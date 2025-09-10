#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os.h"
}

#include <cstdint>
#include <cassert>

#include "crt_Task.h"
#include "crt_Queue.h"

namespace crt
{
   // Long timers need to rearm hw timers multiple times, before
   // being able to fire to the client.
   // For safety and reducing ISR service time,
   // rearmament is done by this task, rather than
   // (in a complex ISR proof way) in the ISR itself.

   // Assumption for safe behaviour:

	class Timer; // forward declaration, to avoid mutual depency of header file inclusion.

	enum class RelayAction : uint8_t { Rearm, DeliverOnly };

	struct LongTimerRelayInfo
	{
	    Timer*    pTimer;
	    uint32_t  longTimerRunId;
	    RelayAction action;

	    LongTimerRelayInfo() : pTimer(nullptr), longTimerRunId(0), action(RelayAction::Rearm) {}
	    LongTimerRelayInfo(Timer* p, uint32_t runId, RelayAction a)
	    : pTimer(p), longTimerRunId(runId), action(a) {}
	};

	class LongTimerRelay : public Task
	{
		Queue<LongTimerRelayInfo, 10> queueLongTimersThatDesireRearm;

	public:
		LongTimerRelay(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes);

		// The function instance can be used to both initialize
		// and to query it.
		// The approach below allows an explicitly constructed Time object to be passed.
		// That helps to group task creations and maintain oversight.
		static LongTimerRelay* instance(LongTimerRelay* instance = nullptr);

		// Assumption for safe behaviour: Timer should not be destroyed before
		// being stopped or having finally fired. (pTimer must remain valid while Queued).
		// This is automatically achieved when always preallocating all required timers
		// and not destroying them - which is the advised way of embedded programming.
		static void requestRearm(const LongTimerRelayInfo& pLongTimerRelayInfo);

	    static inline void requestDeliver(Timer* t, uint32_t runId) {
	        requestRearm(LongTimerRelayInfo{t, runId, RelayAction::DeliverOnly});
	    }
		// Lever meteen vanuit ISR. Geen relay-queue meer voor DeliverOnly.
//		static void requestDeliver(Timer* t, uint32_t runId);

	    static inline void requestRearmTimer(Timer* t, uint32_t runId) {
	        requestRearm(LongTimerRelayInfo{t, runId, RelayAction::Rearm});
	    }

	private:
		void main() override;
	}; // end class LongTimerRelay
}; // end namespace crt
