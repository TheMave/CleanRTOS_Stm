// by Marius Versteegen, 2025

// Extended test-suite for crt::Timer + LongTimerRelay.
#include "crt_TestTimer.h"

#include <cstdio>

extern "C" {
	#include "crt_stm_hal.h"

    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os.h"
	#include <inttypes.h>
}

// by Marius Versteegen, 2023
#include <crt_CleanRTOS.h>
#include "crt_OutputPin.h"

namespace crt
{
	#define USE_SCOPE_PIN

	class TestTimer : public Task
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
		TestTimer(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes)
		: Task(taskName, taskPriority, taskSizeBytes), periodicTimer(this), sleepTimer(this),
		  tA(this), tB(this), tC(this), tD(this),

		#ifdef USE_SCOPE_PIN
		  scopePin(GPIOA, GPIO_PIN_5)
		#endif

		{
			start();
		}

	    // -------- helpers ------------------------------------------------------

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

	    static void overwriteMaxHwTime(Timer& timer, uint64_t nofBitsHwTimer)
	    {
	    	// Redefine maxHwTime to a much lower value, to allow for shorter tests.
	    	// We can do that because TestTimer is a friend class of Timer.
            timer.maxHwTime = (uint32_t)((((uint64_t)1)<<nofBitsHwTimer)-1);
	    }

	    // ----------- tests -----------------------------------------------------

		void testShortTimings()
		{
			printTitle("testShortTimings");
			osDelay(500);

            uint64_t timeMicros = 0;
            uint64_t timeMicrosNew = 0;
			uint64_t timeMicros2 = 0;
            uint64_t timeMicrosNew2 = 0;


			//while(true)									// Remarkably, this long-shot timers seem more accurate than a short timers.
            {
				printf("Start waiting for periodicTimer\r\n");
				osDelay(500);
				periodicTimer.start_periodic(1500000);          // This starts a periodic timer with a period of 1500000 microseconds, which is 1.5 second.
				timeMicros = now_us();
				wait(periodicTimer);
				timeMicrosNew = now_us();
				periodicTimer.stop();
				printf("Timer fired after microseconds:\r\n");
				printf("%" PRId32 "\r\n", (int32_t)(timeMicrosNew - timeMicros));
				osDelay(500);
				timeMicros = timeMicrosNew; 				// prepare for next measurement

				printf("Starting short sleep\r\n");						// Note: calling getTimeMicroseconds twice costs about 5us.
				osDelay(500);
				timeMicros2 = now_us();         						// sleepTimer.sleep_us(100) is equal to sleepTimer.start(100);wait(sleepTimer);
                sleepTimer.sleep_us(100);								// Test the one-shot timer with a sleep of 100 microseconds.
                timeMicrosNew2 = now_us(); 							    // Note: it is recommended to use osDelay for all sleeps greater than 1ms (that saves on hardware timer resources).
                printf("sleepTimer.sleep_us(100) took microseconds:\r\n");
                printf("%" PRId32 "\r\n", (int32_t)(timeMicrosNew2 - timeMicros2));
                osDelay(500);
			}
		}

		// Long Timings specify an amount of microsecs that exceeds 1<<32
		// (does not fit in uint32_t so cannot be used with hw timer directly).
		// bShorten can be used to shorten the test, using less bits of the hwTimer.
		// The ultimate test is though when not shortening (and using the full original 32 bits).
		void testLongTimingSingleShot(bool bShorten)
		{
			printTitle("testLongTimingSingleShot");
			osDelay(500);

			uint64_t timeSecondsBefore 		= 0;
			uint64_t timeMicrosecondsBefore = 0;
			uint64_t timeSecondsAfter 		= 0;
			uint64_t timeMicrosecondsAfter  = 0;

			//while(true)
			{
				printf("Starting long sleep, single shot\r\n");					// Note: calling getTimeMicroseconds twice costs about 5us.
				osDelay(500);

				timeSecondsBefore = now_s();
                timeMicrosecondsBefore = now_us();
                if(bShorten) { overwriteMaxHwTime(sleepTimer,20); } 	// 20 bits means hwTimer restart about every 1e6 us.
                														// (default, it's 32 bits, or about 4e9 us)
                uint64_t sleepTime = 4500000000;						// 1.5 hours.
                assert(sleepTime > ((uint64_t)1)<<32);
                if(bShorten) { sleepTime /= 1000; }						// 4.5sec

                sleepTimer.sleep_us(5000000000/1000);					// Test the one-shot timer with a sleep of 5G microseconds.(does not fit in uint32_t)

				timeMicrosecondsAfter = now_us();
				timeSecondsAfter = now_s();

				// print achteraf om de timing niet te beinvloeden.

				uint32_t timeDifSeconds = (uint32_t)(timeSecondsAfter-timeSecondsBefore);
				printf("sleep in seconds took : %lu\r\n", timeDifSeconds);
				osDelay(500);

				uint64_t timeDifMicroseconds = timeMicrosecondsAfter - timeMicrosecondsBefore;
				uint32_t hi5 = timeDifMicroseconds >> 32;
				uint32_t lo5 = timeDifMicroseconds & 0xFFFFFFFF;
				printf("sleep in microseconds took: (high byte, low byte) : %lu%010lu\r\n", hi5, lo5);
				osDelay(500);
			}

			overwriteMaxHwTime(sleepTimer,32); // restore sleepTimer
		}

		// -------- 1) one-shot short (≤ UINT32_MAX) -----------------------------
		void test_one_shot_short()
		{
			printTitle("test_one_shot_short");
			osDelay(500);
			// Goal: exactly one wake, no relay activity; error within 56us fudge + 1 tick + measurement noise.
			const uint32_t us = 500'000; // 0.5s

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

		// -------- 2) one-shot long (> UINT32_MAX) ------------------------------
		void test_one_shot_long()
		{
			printTitle("test_one_shot_long");
			osDelay(500);
			// WARNING: A true “long” means > 2^32 - 1 us ≈ 4294.967 s (~71.6 min).
			// Use only when you are OK to wait that long, or temporarily lower nofBitsHwTimer
			// in your Timer to make “long” shorter during validation. :contentReference[oaicite:2]{index=2}

			const uint64_t us = (uint64_t)UINT32_MAX + 12'345; // chunked via relay

			{
				print_u64("[one_shot_long] start us = ", us);
				osDelay(500);

				uint64_t t0s  = now_s();
				uint64_t t0us = now_us();
				pulse_pin();
				sleepTimer.sleep_us(us);      // start + wait combined
				pulse_pin();
				uint64_t t1us = now_us();
				uint64_t t1s  = now_s();

				print_u64("seconds delta: ", t1s - t0s);
				print_us_delta("microseconds delta", t0us, t1us);
				osDelay(500);
			}
		}

		// -------- 3) periodic short -------------------------------------------
		void test_periodic_short()
		{
			printTitle("test_periodic_short");
			osDelay(500);
			// Check stability/jitter for small periods.
			const uint32_t period_us = 1'000; // 1 ms
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

		// -------- 4) periodic long --------------------------------------------
		void test_periodic_long()
		{
			printTitle("test_periodic_long");
			osDelay(500);

			const uint64_t period_us = 4500000000;
			assert(period_us > ((uint64_t)1)<<32);

			print_u64("[periodic_long] start_periodic us = ", period_us);
			osDelay(500);
			uint64_t t2 = 0;
			uint64_t t1 = 0;
			uint64_t t0 = now_us();
			periodicTimer.start_periodic(period_us);

			{
				wait(periodicTimer);
				t1 = now_us();
				wait(periodicTimer);
				t2 = now_us();
				print_u64("[periodic_long] abs time at start:", t0);
				print_u64("[periodic_long] abs time after first period in us:", t1);
				print_u64("[periodic_long] abs time after second period in us:", t2);
				print_us_delta("Total of 2 periods", t0, t2);
				print_u64("average per period: ",(t2-t0)/2);
				osDelay(500);
			}
			periodicTimer.stop();
		}

		// -------- 5) edges around UINT32_MAX -----------------------------------
		void test_edges_uint32_max()
		{
			printTitle("test_edges_uint32_max");
			osDelay(500);
			// Verkort HW-timer naar 20 bits zodat de randwaarden in seconden vallen.
			//overwriteMaxHwTime(sleepTimer, 20); // ~1.048.575 us max
			const uint64_t e1 = (uint64_t)sleepTimer.maxHwTime - 1; // friend: toegestaan
			const uint64_t e2 = (uint64_t)sleepTimer.maxHwTime;
			const uint64_t e3 = (uint64_t)sleepTimer.maxHwTime + 1; // forceert long-pad

			printf("[edges] around maxHwTime: ");
			print_u64("e1=", e1);  print_u64("e2=", e2);  print_u64("e3=", e3);
			osDelay(500);

			for (uint64_t us : { e1, e2, e3 }) {
				uint64_t t0 = now_us();
				sleepTimer.start(us);
				wait(sleepTimer);
				uint64_t t1 = now_us();
				printf("[edges] requested "); print_u64("", us);
				print_u64(" measured ", t1 - t0);
				osDelay(500);
			}
			overwriteMaxHwTime(sleepTimer, 32); // herstel
		}

		// -------- 6) minimal duration & fudge ----------------------------------
		void test_min_duration_fudge()
		{
			printTitle("test_min_duration_fudge");
			osDelay(500);
			// The implementation subtracts ~56 us for short waits (to compensate overhead).
			// With the assert ≥ 100 us this should not underflow; expect ~44 us effective.
			const uint32_t us = 100;

			for (int i=0;i<10;++i) {
				uint64_t t0 = now_us();
				sleepTimer.sleep_us(us);
				uint64_t t1 = now_us();

				printf("[min_fudge] requested %" PRIu32 " us :", us);
				print_u64(" measured: ",t1-t0);
				osDelay(500);
			}
		}

		// -------- 7) zero / too small -----------------------------------------
		void test_zero_or_too_small()
		{
			printTitle("test_zero_or_too_small");
			// Expect assert (if asserts enabled) or graceful guard.
			// Only run deliberately.
			printf("[zero_small] attempting small durations (may assert in debug)\r\n");
			osDelay(500);
			// Uncomment carefully in debug builds:
			// sleepTimer.start(0);
			sleepTimer.start(15);
			// sleepTimer.start_periodic(50);
			osDelay(500);
		}

		// -------- 8) stop during chunk (short variant) -------------------------
		void test_stop_during_chunk()
		{
			printTitle("test_stop_during_chunk");
			osDelay(500);
			// For long timers this validates run-id cancellation; this short variant
			// verifies “stop before fire” never delivers a spurious event.
			const uint32_t us = 2'000'000; // 2s
			const bool bShorten=true;
            if(bShorten) { overwriteMaxHwTime(sleepTimer,20); } 	// 20 bits means hwTimer restart about every 1e6 us.

			for (int i=0;i<5;++i) {
				printf("[stop_during_chunk] start(%lu us) then stop after 10ms\r\n", (unsigned long)us);
				osDelay(500);

				sleepTimer.start(us);
				osDelay(10);     // 10 ms
				sleepTimer.stop();  // should cancel; no event should arrive
				// Wait briefly to catch late events
				osDelay(2500);
				pollAny(sleepTimer);
				if(!hasFired(sleepTimer))
				{
					printf("[stop_during_chunk] no event observed (OK)\r\n");
					osDelay(500);
				}
			}

			overwriteMaxHwTime(sleepTimer,32); // restore sleepTimer
		}

		// -------- 9) restart just before fire (short) --------------------------
		void test_restart_before_fire()
		{
			printTitle("test_restart_before_fire");
			osDelay(500);
			// Start, wait almost to expiry, then start a new short wait. Expect exactly
			// one event matching the *new* start.
			const uint32_t first = 200'000;  // 200 ms
			const uint32_t next  = 50'000;   // 50 ms

			printf("[restart_before_fire] first=%lu us then next=%lu us\r\n",
			       (unsigned long)first, (unsigned long)next);
			osDelay(500);

			sleepTimer.start(first);
			osDelay(first/1000 - 30); // a bit before expiry
			sleepTimer.start(next);

			uint64_t t0 = now_us();
			wait(sleepTimer);
			uint64_t t1 = now_us();

			printf("[restart_before_fire] new=%lu us, ", (unsigned long)next);
			print_u64(" measured: ",t1-t0);
			osDelay(500);
		}

		// -------- 10) stop/start storm -----------------------------------------
		void test_stop_start_storm()
		{
			printTitle("test_stop_start_storm");
			osDelay(500);
			// Rapid sequences to shake out races; should never double-fire nor miss.
			for (int i=0;i<100;++i) {
				uint32_t us = 20'000 + (i%7)*5'000; // 20..50ms range
				sleepTimer.start(us);
				if (i%3==0) sleepTimer.stop();
				if (i%5==0) osDelay(1);
			}
			// Final one to ensure system still behaves
			sleepTimer.start(30'000);
			wait(sleepTimer); //..?
			printf("[storm] completed without asserts/spurious events\r\n");
			osDelay(500);
		}

		// -------- 11) multiple timers simultaneously ---------------------------
		void test_multiple_timers_simultaneous()
		{
			printTitle("test_multiple_timers_simultaneous");

			printf("[multi] timers in use %" PRIu32 " / %" PRIu32 "\r\n",
			       (uint32_t)crt::Timers::getNofTimersInUse(),
			       (uint32_t)crt::Timers::getMaxNofTimers());

			// Mix of durations; verify each timer wakes its own task.
			tA.start(120'000); printf("tA h=%d\r\n", (int)tA.hTimer);
			tB.start(80'000);  printf("tB h=%d\r\n", (int)tB.hTimer);
			tC.start(150'000); printf("tC h=%d\r\n", (int)tC.hTimer);
			tD.start(200'000); printf("tD h=%d (valid=%d)\r\n", (int)tD.hTimer,
			                          crt::Timers::isValidTimerHandle(tD.hTimer));

			wait(tB); printf("[multi] B fired\r\n");
			wait(tA); printf("[multi] A fired\r\n");
			wait(tC); printf("[multi] C fired\r\n");
			wait(tD); printf("[multi] D fired\r\n");
			osDelay(500);
		}

		// -------- 12) relay queue almost full (documentation harness) ----------
		void test_queue_almost_full()
		{
			printTitle("test_queue_almost_full");
			// Realistically filling the relay queue requires many long timers to have
			// chunks completing almost simultaneously (difficult without changing hw width).
			// Strategy:
			// 1) Temporarily reduce hw width (test build) so “long” = seconds.
			// 2) Start ~10 long timers with small skew in start time.
			// 3) Watch for processed re-arms; ensure no starvation or deadlock.
			printf("[queue_full] see comments—requires reduced hw width test build.\r\n");
			osDelay(1);
		}

		// -------- 14) GPIO timing accuracy -------------------------------------
		void test_gpio_timing_accuracy()
		{
			printTitle("test_gpio_timing_accuracy");
			osDelay(500);
			// Pulse around wait to visualize latency on a scope/LA.
			for (int i=0;i<10;++i) {
				sleepTimer.start(100'000);
				pulse_pin();  // before wait
				wait(sleepTimer);
				pulse_pin();  // after callback
				osDelay(10);
			}
			printf("[gpio_timing] pulses emitted around 100ms waits\r\n");
			osDelay(500);
		}

		// -------- 15) CPU load & jitter ----------------------------------------
		class BusyTask : public Task {
		private:
			Flag flagResume;
			Flag flagSuspend;
		public:
			BusyTask()
			: Task("Busy", osPriorityBelowNormal, 512),
			  flagResume(this), flagSuspend(this)
			 { start(); }

			void resume(){flagResume.set();}
			void suspend(){flagSuspend.set();}

		private:
			void main() override {
				for(;;) {
					volatile uint32_t x=0;
					for (uint32_t i=0;i<200000;i++) x+=i; // spin
					osDelay(1);
					pollAny(flagSuspend);
					if(hasFired(flagSuspend))
					{
						flagResume.clear();
						wait(flagResume);
					}
				}
			}
		};

		// The second version below measures more accurately,
		// but I keep this function, as it managed to crash the
		// system (solved by adjusting FreeRTOSConfig.h to:).
		// #define configUSE_TIMERS                     1
		// #define configTIMER_QUEUE_LENGTH             32      // bv. 32 voor 2ms periodiek
		// #define configTIMER_TASK_PRIORITY            (configMAX_PRIORITIES - 1)
		// #define configTIMER_TASK_STACK_DEPTH         (configMINIMAL_STACK_SIZE *
		void test_cpu_load_jitter()
		{
			printTitle("test_cpu_load_jitter");
			osDelay(500);
			static BusyTask busy; // create once
			busy.resume();
			osDelay(1000); // give it time to startup
			const uint32_t period_us = 3'300; // 2 ms
			periodicTimer.start_periodic(period_us);

			uint64_t prev = now_us();
			for (int i=0;i<200;++i) {
				wait(periodicTimer);
				uint64_t now = now_us();
				print_u64("[jitter+load] dt=", now-prev);
				osDelay(500);
				prev = now_us();
			}

			periodicTimer.stop();
			busy.suspend();
		}

		// Deze variant gebruikt geen prints tussendoor, welke de tijdmetingen
		// beinvloeden.
		void test_cpu_load_jitter2()
		{
		    printTitle("test_cpu_load_jitter (batch)");
		    osDelay(500);
		    static BusyTask busy; // create once
		    busy.resume();
		    osDelay(1000); // give it time to startup

		    // Hoge-belasting taak mag aan blijven, dit test de robuustheid onder load.
		    const uint32_t period_us = 2'000;  // 2 ms
		    const int N = 200;
		    static uint32_t dts[N];

		    periodicTimer.start_periodic(period_us);

		    // Warm-up: één fire negeren om koude-start effecten te vermijden
		    wait(periodicTimer);
		    uint64_t t_prev = crt::Time::getTimeMicroseconds();

		    for (int i = 0; i < N; ++i) {
		        wait(periodicTimer);
		        uint64_t t_now = crt::Time::getTimeMicroseconds();
		        uint64_t dt64  = t_now - t_prev;
		        t_prev = t_now;
		        dts[i] = (uint32_t)(dt64 > 0xFFFFFFFFULL ? 0xFFFFFFFFU : dt64);
		    }

		    busy.suspend();

		    // Insertion sort (simpel, snel genoeg voor N=200)
		    for (int i = 1; i < N; ++i) {
		        uint32_t key = dts[i];
		        int j = i - 1;
		        while (j >= 0 && dts[j] > key) {
		            dts[j + 1] = dts[j];
		            --j;
		        }
		        dts[j + 1] = key;
		    }

		    // Statistiek
		    uint64_t sum = 0;
		    uint32_t minv = dts[0];
		    uint32_t maxv = dts[N - 1];
		    for (int i = 0; i < N; ++i) sum += dts[i];

		    // Percentielen (nearest-rank)
		    auto idx = [&](int pct)->int {
		        int k = (pct * N + 99) / 100; // ceil(pct*N/100)
		        if (k < 1) k = 1;
		        if (k > N) k = N;
		        return k - 1;
		    };
		    uint32_t p95 = dts[idx(95)];
		    uint32_t p99 = dts[idx(99)];
		    uint32_t avg = (uint32_t)(sum / N);

		    // Outliers t.o.v. target period (bijv. > 2.5 ms)
		    const uint32_t warn_us = 2'500;
		    int n_over = 0;
		    for (int i = 0; i < N; ++i) if (dts[i] > warn_us) ++n_over;

		    printf("[jitter+load] N=%d, period=%" PRIu32 " us | avg=%" PRIu32
		           " min=%" PRIu32 " p95=%" PRIu32 " p99=%" PRIu32 " max=%" PRIu32
		           " | >%" PRIu32 "us: %d\r\n",
		           N, period_us, avg, minv, p95, p99, maxv, warn_us, n_over);

		    osDelay(500);
		}


		// -------- 18) queue write fail (fault injection placeholder) -----------
		void test_queue_write_fail()
		{
			printTitle("test_queue_write_fail - TODO");
			// To force a write failure, instantiate relay queue with COUNT=1 for a test build
			// and fire many long re-arms quickly. Expect write() -> false and no deadlock.
			printf("[queue_write_fail] requires special test build (COUNT=1) to provoke.\r\n");
			osDelay(1);
		}

	private:
		/*override keyword not supported*/
		void main()
		{
			osDelay(1000); // wait for other threads to have started up as well.

		    print_u64("Test 64 bit uint print", 123456789012345ULL);

		    printf("cfg timers=%d, qlen=%d, prio=%d\n",
		           (int)configUSE_TIMERS, (int)configTIMER_QUEUE_LENGTH,
		           (int)configTIMER_TASK_PRIORITY);

		    osDelay(500);

			// These tests run once each, so can be combined:
	        while(true)
	        {
				// ---- SELECT WHICH TEST TO RUN ------------------------------------
				// Enable exactly one call at a time, or make a tiny menu/CLI if desired.
	        	dumpStackHighWaterMarkIfIncreased();
	            osDelay(1000);// time to finish the dump.
	            osDelay(1000);// time to finish the dump.

				testShortTimings();
				osDelay(1000);
				testLongTimingSingleShot(true/*bShorten*/);
				osDelay(1000);
				test_one_shot_short();
				osDelay(1000);
				test_periodic_short();
				osDelay(1000);
				test_min_duration_fudge();
				osDelay(1000);
				test_stop_during_chunk();
				osDelay(1000);
				test_restart_before_fire();
				osDelay(1000);
				test_stop_start_storm();
				osDelay(1000);
				test_multiple_timers_simultaneous();
				osDelay(1000);
				test_queue_almost_full();
				osDelay(1000);
				test_gpio_timing_accuracy();
				osDelay(1000);
				test_cpu_load_jitter2();
				osDelay(1000);
				test_queue_write_fail();
				osDelay(1000);

			    // -- VERY LONG in real hardware width --
				// test_one_shot_long();

//				test_edges_uint32_max();
//				test_periodic_long();

				//------ asserts if OK ------------
				// test_zero_or_too_small();
	        }
		}
	}; // end class TestTimers
};// end namespace crt


extern "C" {
	void testTimer_init()
	{
		crt::cleanRTOS_init();
		static crt::TestTimer testTimer("TestTimer", osPriorityNormal  /*priority*/, 2000 /*stackBytes*/);
	}
}
