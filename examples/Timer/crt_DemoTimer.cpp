// by Marius Versteegen, 2025

#include "crt_DemoTimer.h"

extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

// put c++ includes here
#include <cstdio>
#include <crt_CleanRTOS.h>
#include "crt_OutputPin.h"

using namespace crt;

namespace crt_demotimer
{
	#define USE_SCOPE_PIN

	class DemoTimer : public Task
	{
	private:
        Timer periodicTimer;
        Timer sleepTimer;

        // Extra timers for multi-timer tests
        Timer tA, tB, tC, tD;

		#ifdef USE_SCOPE_PIN
			OutputPin scopePin;
		#endif

		#ifdef USE_SCOPE_PIN
			#define TOGGLE_SCOPE_PIN scopePin.toggle()
		#else
			#define TOGGLE_SCOPE_PIN ((void)0)          // should result in nop
		#endif

		void printTitle(const char* title)
		{
			printf("--------------------------------------------------\r\n");
			printf("              %s\r\n",title);
			printf("--------------------------------------------------\r\n");
			osDelay(100);// allow some time for it to finish.
		}

	public:
		DemoTimer(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes)
		: Task(taskName, taskPriority, taskSizeBytes), periodicTimer(this), sleepTimer(this),
		  tA(this), tB(this), tC(this), tD(this),

		#ifdef USE_SCOPE_PIN
		  scopePin(GPIOA, GPIO_PIN_5)
		#endif

		{
			start();
		}

	    // -------- shorthands ------------------------------------------------------

	    static inline uint64_t now_us() { return crt::Time::getTimeMicroseconds(); }
	    static inline uint64_t now_s()  { return crt::Time::getTimeSeconds(); }

	    // Toggle a GPIO pin around critical points to observe on a scope/LA.
	    inline void pulse_pin()
	    {
	    	TOGGLE_SCOPE_PIN;
	    }

	    static void print_u64(const char* label, uint64_t v)
	    {
	        char buf[32]; char* p = buf + sizeof(buf); *--p = '\0';
	        if (v == 0) *--p = '0';
	        while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
	        printf("%s: %s\r\n", label, p);
	    }

	    static void print_us_delta(const char* label, uint64_t t0, uint64_t t1)
	    {
	        uint64_t dt = (t1 >= t0) ? (t1 - t0) : 0;

	        print_u64(label, dt);
	    }

		void demoOneShot()
		{
			printTitle("Demo OneShot");
			osDelay(500);

			const uint32_t us = 3000; // 3ms.
			{
				printf("[one_shot_short] start(%" PRIu32 " us)\r\n", us);
				osDelay(500);

				uint64_t t0 = now_us();
				sleepTimer.start(us);

				pulse_pin(); // arm boundary
				wait(sleepTimer);
				pulse_pin(); // callback boundary

				uint64_t t1 = now_us();
				print_us_delta("fired after", t0, t1);
				osDelay(500);
			}
		}

		void demoPeriodic()
		{
			printTitle("Demo Periodic");
			osDelay(500);
			// Check stability/jitter for small periods.
			const uint32_t period_us = 3'000; // 3 ms
			const int N = 3;
			uint64_t min_dt = UINT64_MAX, max_dt = 0, sum_dt = 0;
			periodicTimer.start_periodic(period_us);

			uint64_t prev = now_us();

			for (int i=0; i<N; ++i) {
				wait(periodicTimer);
				uint64_t now = now_us();
				uint64_t dt  = now - prev;
				prev = now;

				if (dt < min_dt) min_dt = dt;
				if (dt > max_dt) max_dt = dt;
				sum_dt += dt;
			}
			periodicTimer.stop(); // else blijft hij flushen en overspoelt ie alles.

			printf("[periodic_short] period=%" PRIu32 " us, ",period_us);
			print_u64("avg= ", sum_dt/N);
			print_u64("min_dt= ", min_dt);
			print_u64("max_dt= ", max_dt);
			osDelay(500);

			periodicTimer.stop();
		}

		void demoMultiTimers_WaitAny()
		{
			printTitle("Demo demoMultiTimer_WaitAny");
			osDelay(500);

			printf("tA.start(3000)\r\n");
			printf("tB.start(4000)\r\n");
			printf("tC.start(5000)\r\n");
			printf("tC.start(6000)\r\n");
			osDelay(500);

			uint64_t tStart = now_us();
			tA.start(3000);
			tB.start(4000);
			tC.start(5000);
			tD.start(6000);

			uint64_t ta_end=0;
			uint64_t tb_end=0;
			uint64_t tc_end=0;
			uint64_t td_end=0;

			uint32_t count = 0;
			while (count<4)
			{
				waitAny(tA+tB+tC+tD);
				if(hasFired(tA))
				{
					ta_end = now_us();
				}
				else if(hasFired(tB))
				{
					tb_end = now_us();
				}
				else if(hasFired(tC))
				{
					tc_end = now_us();
				}
				else if(hasFired(tD))
				{
					td_end = now_us();
				}
				count++;
			}

			print_u64("tA fired after us: ",ta_end-tStart);
			print_u64("tB fired after us: ",tb_end-tStart);
			print_u64("tC fired after us: ",tc_end-tStart);
			print_u64("tD fired after us: ",td_end-tStart);
		}

		void demoMultiTimers_WaitAll()
		{
			printTitle("Demo demoMultiTimer_WaitAll");
			osDelay(500);

			printf("tA.start(3000)\r\n");
			printf("tB.start(4000)\r\n");
			printf("tC.start(5000)\r\n");
			printf("tC.start(6000)\r\n");
			osDelay(500);

			uint64_t tStart = now_us();
			tA.start(3000);
			tB.start(4000);
			tC.start(5000);
			tD.start(6000);

			waitAll(tA+tB+tC+tD);

			print_u64("All timers have finished after us: ",now_us()-tStart);
		}

	private:
		/*override keyword not supported*/
		void main()
		{
			osDelay(800); // wait for other threads to have started up as well.
			printf("\r\n----------------\r\n");
			printf("DemoTimer_started\r\n");
			printf("----------------\r\n");
			osDelay(300);

		    printf("cfg timers=%d, qlen=%d, prio=%d\r\n",
				   (int)configUSE_TIMERS, (int)configTIMER_QUEUE_LENGTH,
				   (int)configTIMER_TASK_PRIORITY);

		    osDelay(500);

			// These tests run once each, so can be combined:
	        while(true)
	        {
	        	dumpStackHighWaterMarkIfIncreased();
	            osDelay(1000);// time to finish the dump.

	            demoOneShot();
	            demoPeriodic();
	            demoMultiTimers_WaitAny();
	            demoMultiTimers_WaitAll();
	        }
		}
	}; // end class TestTimers
};// end namespace crt_demotimer


extern "C" {
	void demoTimer_init()
	{
		cleanRTOS_init();
		static crt_demotimer::DemoTimer testTimer("DemoTimer", osPriorityNormal  /*priority*/, 1200 /*stackBytes*/);
	}
}
