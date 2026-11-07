// by Marius Versteegen, 2025

//#include <cstdio>
#include "crt_CleanRTOS.h"
#include <crt_Time.h>
#include "crt_LongTimerRelay.h"

extern "C" {
	#include "cmsis_os2.h"
}

void crt::cleanRTOS_init()
{
	// crt::Time is the "watch", used to measured passed time (while not sleeping).
	static crt::Time timeTask("stmTimeTask", osPriorityNormal /*priority*/, 900 /*stackBytes - measured peak ~480 B incl. stack-HWM printf; was 2200*/);
	// From hereon, the StmTime singleton can be accessed via its static StmTime::instance() function.

	// crt::LongTimerRelay helps to time-chunk-wise restart Timer objects to wait longer than otherwise possible.
	static crt::LongTimerRelay longTimerRelayTask("longTimerRelayTask", osPriorityNormal /*priority*/, 900 /*stackBytes - measured peak ~520 B incl. stack-HWM printf; was 2200*/);
}

// Default report of a new stack high-water mark (see crt_Task.h). Weak, so an
// application can replace it by defining crt_onStackHighWaterMark itself.
extern "C" __attribute__((weak)) void crt_onStackHighWaterMark(const char* taskName, uint32_t stackBytesLeft, uint32_t heapBytesFree)
{
	safe_printf("Task %s has left: %lu stack bytes and %u heap bytes\n",
	            taskName, (unsigned long)stackBytesLeft, (unsigned int)heapBytesFree);
}

// C-callable wrapper for use from C code (e.g., c_Stop2Sleep.c)
// Call after STOP2 wake to process any timers that became due during sleep.
extern "C" void crt_Timers_handleWakeupsAfterSleep(void)
{
	crt::Timers::handleWakeupsAfterSleep();
}
