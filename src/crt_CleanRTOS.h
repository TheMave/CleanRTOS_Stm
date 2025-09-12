// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
}

// crt_CleanRTOS.h is a convenience include: it includes the most common parts
// of CleanRTOS

#pragma once

// Next define can be used to make other libraries conditionally
// dependent on CleanRTOS.
#define CleanRTOS_INCLUDED

#include "crt_Config.h"
#include "crt_Task.h"

// Internals of CleanRTOS that need to be included first
// (Don't use them directly, or it might break CleanRTOS).
#include <crt_Timers.h> // internal object of CleanRTOS. Don't use it directly
namespace crt
{
	constexpr uint32_t MAX_NOF_TIMERS = 100;
	//typedef crt::Timers_template<MAX_NOF_TIMERS> Timers;
	using Timers = Timers_template<MAX_NOF_TIMERS>;
	void cleanRTOS_init();
}
// end of internals.

#include "crt_Flag.h"
#include "crt_Queue.h"
#include "crt_Time.h"
#include "crt_Timer.h"
#include "crt_Pool.h"

//#include "crt_IHandler.h"
//#include "crt_IHandlerListener.h"

// Internals:



//extern "C" {
	// This function should be called before creating other CleanRTOS Tasks.
//	void cleanRTOS_init()
//	{
//		// StmTime is the "watch", used to measured passed time (while not sleeping).
//		static crt::StmTime stmTimeTask("stmTimeTask", osPriorityNormal /*priority*/, 5000 /*stackBytes*/);
//		// From hereon, the StmTime singleton can be accessed via its static StmTime::instance() function.
//
//		// StmTimers is the hub via where a single hardware timer (timer2) is shared among
//		// a lot of "virtual" timers.
//		constexpr uint32_t MAX_NOF_TIMERS = 100;
//		static crt::StmTimers<MAX_NOF_TIMERS> stmTimers();
//		// From hereon, the StmTimers singleton can be accessed via its static StmTimers::instance() function.
//	}
////}

// crt_Logger, crt_MutexSection, crt_Mutex and crt_Handler are to
// be included separately, if needed.
//
// Reasons:
//    * Logger should never be needed outside main.cpp (or .ino)).
//      ILogger should be used everywhere else instead.
//    * Handler should never be needed outside main.cpp (or .ino))
//      IHandler should be used everywhere else instead.
//    * Mutex should never be needed outside main.cpp (or .ino)),
//      to keep a good overview of the assignment and order of mutex ids.
//		Also, Mutex should not be used stand-alone, as that is error prone.
//      Use MutexSections in conjunction.
//    * Even MutexSections should not be needed often, with a clear design.
//      For example, try to use a Pool instead of a single MutexSection.
