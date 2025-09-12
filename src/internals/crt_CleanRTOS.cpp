
// by Marius Versteegen, 2023

//#include <cstdio>
extern "C" {
//    #include "crt_stm_hal.h"
//    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os2.h"
//	#include <inttypes.h>
}

#include "crt_CleanRTOS.h"
#include <crt_Time.h>
#include "crt_LongTimerRelay.h"

void crt::cleanRTOS_init()
{
	// crt::Time is the "watch", used to measured passed time (while not sleeping).
	static crt::Time timeTask("stmTimeTask", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
	// From hereon, the StmTime singleton can be accessed via its static StmTime::instance() function.

	// crt::LongTimerRelay helps to time-chunk-wise restart Timer objects to wait longer than otherwise possible.
	static crt::LongTimerRelay longTimerRelayTask("longTimerRelayTask", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
}
