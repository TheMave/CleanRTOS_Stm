// by Marius Versteegen, 2023

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_TestIndexPool.h"
#include "crt_TestIndexPoolTask.h"

extern "C" {
	#include "cmsis_os.h"
}

#include "crt_CleanRTOS.h"

extern "C" {
	void testIndexPool_init()
	{
		static crt::TestIndexPoolTask testIndexPoolTask("TestIndexPoolTask", osPriorityNormal /*priority*/, 1000 /*stackBytes*/); // Don't forget to call its start() member during setup().
	}
}
