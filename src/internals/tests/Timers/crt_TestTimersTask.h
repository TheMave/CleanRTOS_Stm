// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

#include <cstdio>
#include "crt_CleanRTOS.h"
#include <crt_Timers.h>
#include <crt_Time.h>

#include "crt_PrintTools.h"

namespace crt
{
	static constexpr auto& print_u64 = PrintTools::print_u64; // short hand fp as alias.

	class TestTimersTask : public Task
	{

	private:
		// Belangrijk: variabelen die door ISR veranderd worden en in normale thread gelezen, volatile
		// maken. Anders kunnen ze (met compile optimization O2 bijvoorbeeld) naar registers
		// worden geoptimaliseerd, waardoor de brug vervalt.
		static uint32_t myUserArg; // Deze wordt niet in de ISR veranderd, dus niet volatile.
		static volatile uint32_t myUserArgReceived;
		static volatile uint64_t us1;
		static volatile uint64_t diff1;
		static volatile uint64_t us2;
		static volatile uint64_t diff2;
		static volatile uint64_t us3;
		static volatile uint64_t diff3;
		static volatile int32_t myPeriodicTimerCount;

	public:
		TestTimersTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

	public:
		// Note: the static tests below can be called as well without instantiating a TestTimersTask Object.

		// === Interface ===

		// Call testTimers_human_readable to show some
		// human readable examples.
		static void testTimers_human_readable()  {testTimers_human_readable_impl();}

		// Call testLifeCycleFunctions_auto for automatic testing.
		static void testTimers_auto() 			{testTimers_auto_impl();};

		// Call doAutoTests() to do all automatic tests.
		static void doAutoTests()					{doAutoTests_impl();}

	private:
		// === Implementations ===
		static void testTimers_human_readable_impl()
		{
			printf("\r\n------testTimers_human_readable----------\r\n");
			printf("\r\n testing 3 concurrently running timers.. \r\n");

			printf("Max Nof Timers set as template parameter in CleanRTOS: %" PRIu32 "\r\n",
				   crt::Timers::getMaxNofTimers());

			printf("Amount of corresponding size in bytes that Timers takes: %" PRIu32 "\r\n",
				   crt::Timers::getMemUsageBytes());

			crt::TimerHandle hTimer1 = crt::Timers::createTimer("myTimer", myCallback1, &myUserArg);
			crt::Timers::isValidTimerHandle(hTimer1);

			crt::TimerHandle hTimer2 = crt::Timers::createTimer("myTime2", myCallback2, &myUserArg);
			crt::Timers::isValidTimerHandle(hTimer2);

			crt::TimerHandle hTimer3 = crt::Timers::createTimer("myTime3", myCallback3, &myUserArg);
			crt::Timers::isValidTimerHandle(hTimer3);

			us1 = Time::getTimeMicroseconds();
			crt::Timers::startTimer(hTimer1, 671/*duration_us*/, false/*bPeriodic*/);
			assert(crt::Timers::isTimerRunning(hTimer1) || (diff1!=0)); // in latter case: already fired.
			us2 = Time::getTimeMicroseconds();
			crt::Timers::startTimer(hTimer2, 2000/*duration_us*/, false/*bPeriodic*/);
			assert(crt::Timers::isTimerRunning(hTimer2) || (diff2!=0)); // in latter case: already fired.
			us3 = Time::getTimeMicroseconds();
			crt::Timers::startTimer(hTimer3, 444/*duration_us*/, true/*bPeriodic*/);
			assert(crt::Timers::isTimerRunning(hTimer3) || (diff3!=0)); // in latter case: already fired.
			// busy wait:
			uint32_t i=0; // count loops
			while(true)
			{
				i++;
				if((diff1!=0)and(diff2!=0)and(diff3!=0))
				{
					break;
				}
			}
			// stop periodic timer
			assert(crt::Timers::isTimerRunning(hTimer3));
			Timers::stopTimer(hTimer3);
			assert(!crt::Timers::isTimerRunning(hTimer3));

			uint64_t diff4 = Time::getTimeMicroseconds()-us1;

			print_u64("Single shot 671us timer took: ", diff1);
			print_u64("Single shot 2000us timer took: ", diff2);
			print_u64("20 periods of 444us timer took: ", diff3);
			print_u64("Total amount microseconds spent: ", diff4);

			printf("Amount of busy wait loops: %" PRIu32 "\r\n", i);

			diff1 = 0;
			diff2 = 0; // prepare for next measurement.
			diff3 = 0;
			myPeriodicTimerCount = 0;

			assert(crt::Timers::getNofTimersInUse()==3);
			crt::Timers::destroyTimer(hTimer1);
			assert(crt::Timers::getNofTimersInUse()==2);
			crt::Timers::destroyTimer(hTimer2);
			assert(crt::Timers::getNofTimersInUse()==1);
			crt::Timers::destroyTimer(hTimer3);
			assert(crt::Timers::getNofTimersInUse()==0);

			assert(!crt::Timers::isValidTimerHandle(hTimer1));
			assert(!crt::Timers::isValidTimerHandle(hTimer2));
			assert(!crt::Timers::isValidTimerHandle(hTimer3));

			printf("-------------------------------------------\r\n");
		}

		static void myCallback1(void* userArg)
		{
			myUserArgReceived = *((uint32_t*)userArg);
			diff1 = Time::getTimeMicroseconds()-us1;
		}

		static void myCallback2(void* userArg)
		{
			myUserArgReceived = *((uint32_t*)userArg);
			diff2 = Time::getTimeMicroseconds()-us2;
		}

		static void myCallback3(void* userArg)
		{
			myPeriodicTimerCount++;
			if(myPeriodicTimerCount==20)
			{
				// signal end.
				diff3 = Time::getTimeMicroseconds()-us3;
				// dont call stoptimer here, from within the hw interrupt!
			}
			assert(myPeriodicTimerCount<=50); // otherwise stopping above has failed.
		}

		static void testTimers_auto_impl()
		{
			printf("Auto - testing Timers" "\r\n");
		}

		static void doAutoTests_impl()
		{
			testTimers_auto();
		}

		void main() override
		{
			vTaskDelay(10); // wait for init of dependencies.
			uint32_t iTestRun = 0;
			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				printf("Test run: %" PRIu32 "\r\n", iTestRun++);
				testTimers_human_readable();
				doAutoTests();

				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				osDelay(5000);
			}
		}
	};
};// end namespace crt
