// by Marius Versteegen, 2023

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_TestFlag.h"

#include <cstdio>
extern "C" {
	#include "crt_stm_hal.h"

    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os.h"
}

#include "crt_CleanRTOS.h"

namespace crt
{
	class FlagListener : public Task
	{
	private:
        Flag flagHi;

	public:
		FlagListener(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes), flagHi(this)
		{
			start();
		}

	// Simply Notifying another task should be done via a member function like the
	// one below, which sets a Flag.
    void sayHi()
    {
        flagHi.set();
    }

	private:
		void main() override
		{
			vTaskDelay(1000); // wait for other threads to have started up as well.

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				printf("FlagListener: Waiting for flagHi\r\n"); vTaskDelay(1);
				wait(flagHi);								// Wait for flag to be set.
				printf("FlagListener: flagHi was set\r\n"); vTaskDelay(1);
				printf("FlagListener: Another thread said Hi to this thread\r\n"); vTaskDelay(1);
			}
		}
	}; // end class FlagListener

	class FlagSetter : public Task
	{
	private:
		FlagListener& flagListener;

	public:
		FlagSetter(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes, FlagListener& flagListener) :
			Task(taskName, taskPriority, taskSizeBytes), flagListener(flagListener)
		{
			start();
		}

	private:
		void main() override
		{
			vTaskDelay(1000); // wait for other threads to have started up as well.

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();
				printf("FlagSetter: Saying Hi to FlagListener\r\n"); vTaskDelay(1);
				flagListener.sayHi();
				vTaskDelay(1000);
			}
		}
	}; // end class FlagSetter
};// end namespace crt

extern "C" {
	void testFlag_init()
	{
		static crt::FlagListener   flagListener("FlagListener", osPriorityNormal /*priority*/, 800 /*stackBytes*/);
		static crt::FlagSetter     flagSetter  ("FlagSetter",   osPriorityNormal /*priority*/, 800 /*stackBytes*/, flagListener);
	}
}
