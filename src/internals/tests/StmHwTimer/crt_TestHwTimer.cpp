// by Marius Versteegen, 2023

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include <internals/examples/StmHwTimer/crt_TestHwTimer.h>
#include <cstdio>
extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os.h"
	#include <inttypes.h>
}

#include "crt_CleanRTOS.h"

#include "stmHwTimer2.h"
#include "stmCycleCounter.h"

// This file contains the code of a task that sends numbers to another task, which displays them.
// The numbers are transferred via the Queue synchronization mechanism.

// To see the test output in the serial monitor, just press the button that is assigned
// to the logger.

namespace crt
{
	//extern crt::ILogger& logger;

	class TestHwTimerTask : public Task
	{
	public:
		TestHwTimerTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

	static volatile uint32_t bGlobal; // initialised below

	static void timerCallback(void* userData)
	{
		bGlobal = 1;
		assert(*((int32_t*)userData) == 42);
	}

	private:
		void main() override
		{
			int32_t testUserData = 42;

			printf("timer init\n\r");
			osDelay(100);
			timer2_init();
			timer2_set_callback(TestHwTimerTask::timerCallback, &testUserData);

			printf("PCLK1: %lu\n", HAL_RCC_GetPCLK1Freq()); // check CLK1 freq
			osDelay(100);

			// ergens later...
			uint32_t timerTime_us=1;

			startCycleCount();
			uint32_t nCount =0;
			uint32_t nCycles=0;
			uint32_t nPrevCycleCount=0;
			uint32_t nCountFires=0;
			timer2_fire_after_us(timerTime_us); // start een one-shot timer van 2 µs

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();// This function call takes about 0.25ms! It should be called while debugging only.

				for(;;)
				{
					nCountFires = 0;
					nPrevCycleCount = getCycleCount();
					while(nCountFires<100) // measure clockticks of 100 consequtive timer sets and fires.
					{
						if(bGlobal!=0) { bGlobal=0;timer2_fire_after_us(timerTime_us);nCountFires++;}
					}
					nCycles = getCycleCount()-nPrevCycleCount;
					//osThreadYield();
					//osDelay(1);
					nCount+=1;
					if (nCount==2) {
						printf("count: %lu\r\n", nCount);
						printf("cycles: %lu\r\n", nCycles);
						break;
					}
				}

				dumpStackHighWaterMarkIfIncreased();

				osDelay(50000);
			}
		}
	}; // end class TestHwTimerTask
};// end namespace crt


// initialisation. moet volatile zijn - want communicatiepunt voor elders.
// anders kan compiler dit bij optimalisaties weg-optimaliseren. (de compiler weet niet veel van implementatis elders).
volatile uint32_t crt::TestHwTimerTask::bGlobal = 0; // De compiler wil dit buiten de namespace hebben..

extern "C" {
	void testHwTimer_init()
	{
		// kernal not started yet: printf werkt hier nog niet..
		static crt::TestHwTimerTask testHwTimerTask("TestHwTimerTask", osPriorityNormal /*priority*/, 2000 /*stackBytes*/); // Don't forget to call its start() member during setup().
	}
}

