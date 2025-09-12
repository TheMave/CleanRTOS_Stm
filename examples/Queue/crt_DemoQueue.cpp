// by Marius Versteegen, 2023

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_DemoQueue.h"

extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

// put c++ includes here
#include <cstdio>
#include "crt_CleanRTOS.h"


// This file contains the code of a task that sends numbers to another task, which displays them.
// The numbers are transferred via the Queue synchronization mechanism.

using namespace crt;

namespace crt_demoqueue
{
	class NumberDisplayTask : public Task
	{
	private:
		Queue<int32_t, 10> queueNumbers;

	public:
		NumberDisplayTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes), queueNumbers(this)
		{
			start();
		}

    // The function below is called by an other thread, thus passing on the numbers that is wants to get displayed.
	void displayNumber(int32_t number)
	{
        bool bResult = queueNumbers.write(number);
        if (!bResult)
        {
        	printf("NumberDisplayTask: OOPS! queueNumber was already full!\r\n"); vTaskDelay(1);
        }
	}

	private:
		void main() override
		{
			int32_t number=0;
			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				wait(queueNumbers);
                queueNumbers.read(number);
                printf("NumberDisplayTask: received number = %" PRIi32 "\r\n", number);
			}
		}
	}; // end class NumberDisplayTask

	class NumberSendTask : public Task
	{
	private:
		NumberDisplayTask& numberDisplayTask;

	public:
		NumberSendTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes, NumberDisplayTask& numberDisplayTask) :
			Task(taskName, taskPriority, taskSizeBytes), numberDisplayTask(numberDisplayTask)
		{
			start();
		}

	private:
		void main() override
		{
			osDelay(1000); // wait for other threads to have started up as well.
			printf("\r\n-----------------\r\n");
			printf("DemoQueue_started\r\n");
			printf("-----------------\r\n");
			osDelay(300);

			int32_t i = 1;

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();

				// Send a burst of 5 numbers to the NumerDisplayTask.
				for(int n=0;n<5;n++)
				{
					numberDisplayTask.displayNumber(i++);
				}

				osDelay(1000);
			}
		}
	}; // end class NumberSendTask
};// end namespace crt_demoqueue

extern "C" {
	void demoQueue_init()
	{
		static crt_demoqueue::NumberDisplayTask numberDisplayTask("NumberDisplayTask", osPriorityNormal /*priority*/, 1000 /*stackBytes*/); // Don't forget to call its start() memeber during setup().
		static crt_demoqueue::NumberSendTask    numberSendTask   ("NumberSendTask"   , osPriorityNormal /*priority*/, 1000 /*stackBytes*/, numberDisplayTask);
	}
}
