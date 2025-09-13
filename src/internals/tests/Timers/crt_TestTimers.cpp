// by Marius Versteegen, 2025

extern "C" {
	// put c includes here
	#include "cmsis_os2.h"
}

// put c++ includes here
#include "crt_TestTimers.h"
#include "crt_TestTimersTask.h"
#include "crt_CleanRTOS.h"

extern "C" {
	void testTimers_init()
	{
		crt::cleanRTOS_init();
		static crt_testtimers::TestTimersTask testTimersTask("TestTimersTask", osPriorityNormal /*priority*/, 1500 /*stackBytes*/); // Don't forget to call its start() member during setup().
	}
}
