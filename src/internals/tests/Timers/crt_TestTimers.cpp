// by Marius Versteegen, 2025

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include <tests/Timers/crt_TestTimers.h>
#include <tests/Timers/crt_TestTimersTask.h>

extern "C" {
	#include "cmsis_os2.h"
}

#include "crt_CleanRTOS.h"

extern "C" {
	void testTimers_init()
	{
		crt::cleanRTOS_init();
		static crt::TestTimersTask testTimersTask("TestTimersTask", osPriorityNormal /*priority*/, 1500 /*stackBytes*/); // Don't forget to call its start() member during setup().
	}
}
