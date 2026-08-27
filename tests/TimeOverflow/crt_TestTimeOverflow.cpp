// Regression test for the uint64 overflow in crt::Time::getTimeMicroseconds().
//
// Background
// ----------
// getTimeMicroseconds_impl() used to compute the microseconds as
//
//     getTotalCycleCount_impl() * 1000000 / uint64_t(SystemCoreClock)
//
// That multiplication happens in uint64 and therefore overflows as soon as
//
//     total >= 2^64 / 1e6 = 18,446,744,073,710 cycles
//
// At 48 MHz that is after only 4.45 days of uptime. The clock thus became a
// sawtooth with an unreachable ceiling W = (2^64 - 1) / SystemCoreClock us.
//
// Consequence: crt_Timers.h computes an absolute deadline (wakeTime_us = now_us
// + duration_us) and fires on wakeTime_us <= now_us. A timer that is armed just
// below the ceiling gets a wakeTime ABOVE W, and that value is never reached -
// not even in a later sawtooth. The waiting task then hangs permanently, because
// crt_Task.h waits with osWaitForever, without a timeout.
//
// This is what kept an AppRunner stuck for 15 days on 2026-08-12.
//
// The clock is advanced here through the existing public
// Time::addSleepCompensationMs(), so that the Time class itself does not need to
// be modified for this test.

#include "crt_TestTimeOverflow.h"

#include <cstdio>

extern "C" {
	#include "crt_stm_hal.h"
	#include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

#include <crt_CleanRTOS.h>

using namespace crt;

namespace crt_testtimeoverflow
{
	// Wait time requested by the victim task. Equal to what AppRunner uses in its
	// WaitAbit branch (crt_AppRunner.h), so we reproduce the field scenario exactly.
	static constexpr uint64_t victimSleepUs = 1000000;

	// How far below the ceiling we place the clock for test B. Must be smaller than
	// victimSleepUs, otherwise the deadline still falls within reach and the test
	// demonstrates nothing.
	static constexpr uint64_t marginBeforeCeilingUs = 500000;

	// Maximum time we grant the victim to complete its sleep.
	static constexpr uint32_t victimTimeoutMs = 5000;

	// A separate task that performs a single crt::Timer sleep. Before the fix that
	// sleep hangs permanently, which is why it must not live in the test task itself:
	// that one has to keep running in order to report the result.
	// Waiting for the start signal deliberately uses osDelay (systick) rather than a
	// crt::Timer, so the test does not depend on the mechanism it is investigating.
	class VictimTask : public Task
	{
	private:
		Timer timer;
		volatile bool bGo;
		volatile bool bDone;

	public:
		VictimTask(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes)
		: Task(taskName, taskPriority, taskSizeBytes), timer(this), bGo(false), bDone(false)
		{
			start();
		}

		void go()           { bGo = true;  }
		bool isDone() const { return bDone; }

	private:
		void main() override
		{
			while (!bGo) { osDelay(10); }

			timer.sleep_us(victimSleepUs);   // <-- never returns before the fix
			bDone = true;

			while (true) { osDelay(1000); }
		}
	};

	class TestTimeOverflow : public Task
	{
	private:
		VictimTask& victim;

		void printTitle(const char* title)
		{
			printf("--------------------------------------------------\r\n");
			printf("              %s\r\n", title);
			printf("--------------------------------------------------\r\n");
			osDelay(100);
		}

		static void print_u64(const char* label, uint64_t v)
		{
			char buf[32]; char* p = buf + sizeof(buf); *--p = '\0';
			if (v == 0) *--p = '0';
			while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
			printf("%s: %s\r\n", label, p);
		}

		// Highest value getTimeMicroseconds() can ever return as long as the
		// multiplication happens before the division: floor((2^64 - 1) / SystemCoreClock).
		static uint64_t ceilingUs()
		{
			return UINT64_MAX / (uint64_t)SystemCoreClock;
		}

		// Advance the clock to just below targetUs by repeatedly adding sleep
		// compensation. Repeatedly, because getTotalCycleCount_impl() also includes the
		// running DWT counter, so a single computation would be off.
		static void seedClockTo(uint64_t targetUs)
		{
			for (int attempt = 0; attempt < 16; ++attempt)
			{
				const uint64_t now = Time::getTimeMicroseconds();
				if (now + 1000 >= targetUs) break;      // within 1 ms is accurate enough

				const uint64_t deltaMs = (targetUs - now) / 1000;
				if (deltaMs == 0) break;

				const uint32_t stepMs = (deltaMs > 0xF0000000ull)
				                        ? 0xF0000000u : (uint32_t)deltaMs;
				Time::addSleepCompensationMs(stepMs);
			}
		}

	public:
		TestTimeOverflow(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes,
		                 VictimTask& victim)
		: Task(taskName, taskPriority, taskSizeBytes), victim(victim)
		{
			start();
		}

		// -------- tests -----------------------------------------------------

		// Test B: a timer armed just below the ceiling gets a deadline that is never
		// reachable. Deterministic: we place the clock inside the lethal window
		// ourselves, instead of waiting for that to happen by chance.
		bool test_timer_deadline_reachable()
		{
			printTitle("test_timer_deadline_reachable");

			const uint64_t target = ceilingUs() - marginBeforeCeilingUs;
			seedClockTo(target);

			const uint64_t now = Time::getTimeMicroseconds();
			print_u64("  ceiling W (us)     ", ceilingUs());
			print_u64("  clock set to (us)  ", now);
			print_u64("  wakeTime becomes(us)", now + victimSleepUs);

			victim.go();

			uint32_t waitedMs = 0;
			while (!victim.isDone() && waitedMs < victimTimeoutMs)
			{
				osDelay(100);
				waitedMs += 100;
			}

			const bool pass = victim.isDone();
			if (pass)
			{
				printf("  PASS: sleep_us returned within %lu ms.\r\n", (unsigned long)waitedMs);
			}
			else
			{
				printf("  FAIL: sleep_us did not return within %lu ms.\r\n",
				       (unsigned long)victimTimeoutMs);
				printf("        The deadline lies above the ceiling and is never reached.\r\n");
			}
			osDelay(500);
			return pass;
		}

		// Test A: the clock must stay monotonic when total passes the overflow
		// threshold. Before the fix the returned value snaps back to almost zero.
		bool test_monotonic_across_overflow()
		{
			printTitle("test_monotonic_across_overflow");

			// Test B left the clock just below the ceiling; adding a few more seconds
			// pushes total across the threshold.
			const uint64_t before = Time::getTimeMicroseconds();
			Time::addSleepCompensationMs(5000);
			const uint64_t after = Time::getTimeMicroseconds();

			print_u64("  before threshold (us)", before);
			print_u64("  after  threshold (us)", after);

			const bool pass = (after > before);
			if (pass)
			{
				printf("  PASS: clock kept running across the threshold.\r\n");
			}
			else
			{
				printf("  FAIL: clock ran backwards (sawtooth) - overflow in the us computation.\r\n");
			}
			osDelay(500);
			return pass;
		}

	private:
		void main() override
		{
			osDelay(1000); // wait for other threads to have started up

			printf("\r\n");
			printf("==================================================\r\n");
			printf("        crt::Time overflow regression test\r\n");
			printf("==================================================\r\n");
			print_u64("SystemCoreClock (Hz)", (uint64_t)SystemCoreClock);
			print_u64("Overflow after (s uptime)", UINT64_MAX / 1000000ull / (uint64_t)SystemCoreClock);
			osDelay(500);

			// The order matters: test B moves the clock up to just below the ceiling,
			// after which test A pushes it across the threshold with a few seconds.
			const bool passB = test_timer_deadline_reachable();
			const bool passA = test_monotonic_across_overflow();

			printf("\r\n");
			printf("==================================================\r\n");
			printf("  Test B (timer deadline reachable) : %s\r\n", passB ? "PASS" : "FAIL");
			printf("  Test A (monotonic across thresh.) : %s\r\n", passA ? "PASS" : "FAIL");
			printf("  TOTAL                             : %s\r\n", (passA && passB) ? "PASS" : "FAIL");
			printf("==================================================\r\n");

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();
				osDelay(5000);
			}
		}
	}; // end class TestTimeOverflow
}; // end namespace crt_testtimeoverflow


extern "C" {
	void testTimeOverflow_init()
	{
		crt::cleanRTOS_init();
		static crt_testtimeoverflow::VictimTask victim("TimeOvfVictim", osPriorityNormal, 2000);
		static crt_testtimeoverflow::TestTimeOverflow testTimeOverflow(
			"TestTimeOverflow", osPriorityNormal, 2000, victim);
	}
}
