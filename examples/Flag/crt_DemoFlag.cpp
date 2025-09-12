// by Marius Versteegen, 2025

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_DemoFlag.h"

extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
}

// put c++ includes here
#include <cstdio>
#include "crt_CleanRTOS.h"

using namespace crt;

namespace crt_demoflag
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
			osDelay(800); // wait for other threads to have started up as well.
			printf("\r\n----------------\r\n");
			printf("DemoFlag_started\r\n");
			printf("----------------\r\n");
			osDelay(100);

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				printf("FlagListener: Waiting for flagHi\r\n"); osDelay(1);
				wait(flagHi);								// Wait for flag to be set.
				printf("FlagListener: flagHi was set\r\n"); osDelay(1);
				printf("FlagListener: Another thread said Hi to this thread\r\n"); osDelay(1);
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
			osDelay(1000); // wait for other threads to have started up as well.

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();
				printf("FlagSetter: Saying Hi to FlagListener\r\n"); osDelay(1);
				flagListener.sayHi();
				osDelay(1000);
			}
		}
	}; // end class FlagSetter
};// end namespace crt_demoflag

extern "C" {
	void demoFlag_init()
	{
		static crt_demoflag::FlagListener   flagListener("FlagListener", osPriorityNormal /*priority*/, 800 /*stackBytes*/);
		static crt_demoflag::FlagSetter     flagSetter  ("FlagSetter",   osPriorityNormal /*priority*/, 800 /*stackBytes*/, flagListener);
	}
}
