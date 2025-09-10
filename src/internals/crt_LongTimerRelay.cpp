extern "C" {
	#include "crt_stm_hal.h"
	#include "cmsis_os.h"
}

#include <cstdint>
#include <cassert>

#include "crt_LongTimerRelay.h"
#include "crt_Timer.h"
#include "crt_Task.h"

// Implementation in cpp file to make mutual dependency of Timer and LongTimerRelay build with
// forward declaration only (without additional interface).

namespace crt
{
	// Note: bWaitWriteTillQueueFill must be false in this case, as requestRearm is called from an ISR,
    // and osMessageQueuePut in cmsis2 is only ISR safe is timeout is set to 0.
	// NB: nullptr is passed to getType instead of this.
	// That makes sure that setEventBits is not used, and thus the FreeRTOS timer service is bypassed.
	// (activation of FreeRTOS timer service from ISR can cause overload of the service, in some cases.
    //  see comments at cpu_load_jitter test of crt_TestTimer.cpp)
	LongTimerRelay::LongTimerRelay(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
	Task(taskName, taskPriority, taskSizeBytes), queueLongTimersThatDesireRearm(nullptr)
	{
		instance(this); // initialize the static _pInstance variable in the function instance().

		start();
	}

	/*(static)*/ LongTimerRelay* LongTimerRelay::instance(LongTimerRelay* instance)
	{
		// I prefer function static variable over class static variable.
		// (no implementation outside class needed).
		static LongTimerRelay* _pInstance = nullptr;
		if(_pInstance != nullptr)
		{
			assert(instance == nullptr); // initialisation of this Task object should only happen once  (like singleton).
			// Typically, it should be done in the init function of CleanRTOS.
		}
		else
		{
			_pInstance = instance; // initialisation.
		}
		return _pInstance;
	}

	// Assumption for safe behaviour: Timer should not be destroyed before
	// being stopped or having finally fired. (pTimer must remain valid while Queued).
	// This is automatically achieved when always preallocating all required timers
	// and not destroying them - which is the advised way of embedded programming.
	/*(static)*/ void LongTimerRelay::requestRearm(const LongTimerRelayInfo& longTimerRelayInfo)
	{
		instance()->queueLongTimersThatDesireRearm.write(longTimerRelayInfo);
	}

	void LongTimerRelay::main()
	{
		osDelay(1);
	    while (true) {
	    	dumpStackHighWaterMarkIfIncreased();

	        LongTimerRelayInfo info{nullptr,0,RelayAction::Rearm};
	        queueLongTimersThatDesireRearm.read(info);

	        if (!info.pTimer) {osDelay(0); continue;}

	        switch (info.action) {
	            case RelayAction::Rearm:
	                info.pTimer->rearmToContinueLongTiming(info.longTimerRunId);
	                break;

	                // no longer used: (see requestDeliver in header file)
	                // reden: prevent Queue overload bij te snelle periodic timers.
	            case RelayAction::DeliverOnly:
	                // Bezorg eventbit, maar alleen als het nog dezelfde run is
	                if (info.longTimerRunId == info.pTimer->getLongTimerRunId()) {
	                    // task-context: veilig
	                    info.pTimer->getOwnerTask()->setEventBits(info.pTimer->getBitMask());
	                }
	                break;
	        }
	        osDelay(0);
	    }
	}
//
//	void LongTimerRelay::requestDeliver(Timer* t, uint32_t runId) {
//		if (!t) return;
//		// Alleen bezorgen als het nog dezelfde run is (was ook zo in de relay-loop):
//		if (runId == t->getLongTimerRunId()) {
//			// Voorkeur: wrapper op Task die FromISR doet (zie Stap 2).
//			t->getOwnerTask()->setEventBits(t->getBitMask());
//		}
//	}
}; // end namespace crt
