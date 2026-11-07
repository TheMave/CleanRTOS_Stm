// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"
	#include "cmsis_os2.h"
	#include "FreeRTOS.h"  // for configASSERT
}

#include <cassert>
namespace crt
{
	// A SimpleMutex protects a resource from concurrent access by multiple threads.
	// It is a bit simpler to use and faster than the normal Mutex.

	// However, opposed to the normal Mutex, it does not detect deadlock situations.
	// Thus the SimpleMutex should only be used in situations that cannot potentially
	// yield deadlock situations - that is, when a scope is always protected by
    // a single mutex only, like in a Pool.

	// If you use SimpleMutex, make sure to lock and unlock it using SimpleMutexSection.
	// That way, unlocks are guaranteed when leaving the local scope.

	// The FreeRTOS control block of the mutex is part of this object
	// (static allocation, configSUPPORT_STATIC_ALLOCATION = 1), so creating
	// a SimpleMutex takes nothing from the FreeRTOS heap. FreeRTOS keeps a
	// pointer to that control block, so a SimpleMutex can not be copied.

	class SimpleMutex
	{
	private:
		StaticSemaphore_t controlBlock;
		osMutexId_t mutexId;
		osStatus_t status;

	public:
		SimpleMutex()
		{
			configASSERT(osKernelGetState() != osKernelInactive);
			const osMutexAttr_t attributes = { nullptr, 0, &controlBlock, sizeof(controlBlock) };
			mutexId = osMutexNew(&attributes);
			assert(mutexId != nullptr);
		}

		SimpleMutex(const SimpleMutex&) = delete;
		SimpleMutex& operator=(const SimpleMutex&) = delete;

		void lock()
		{
			while (true)
			{
				status = osMutexAcquire(mutexId, osWaitForever);
				if (status == osOK)
					break;

				osThreadYield();  // WDT vriendelijk
			}
		}

		void unlock()
		{
			status = osMutexRelease(mutexId);
			assert(status == osOK);
		}
	};
};
