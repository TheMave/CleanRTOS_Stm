// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"
	#include "cmsis_os2.h"
}
#include <cassert>

namespace crt
{
	// A mutex could be created for each resource that is shared by multiple threads.
	// The mutex can be used to avoid concurrent usage. Instead of locking the Mutex
	// directly, it is better to do it via MutexSection instead
	//	(which uses Mutex internally). That helps to avoid deadlocks due to misaligned
	//	order of resource lockings.


	//	Note: Most of the times, it is neater instead to make sure that each resource
	//	is only directly accessed by a single thread (let's call that thread the
	//	"resource keeper"). Multiple threads could then queue resource - interaction
	//	commands to the "resource keeper".Use of Mutex(-Sections) can be omitted, then.

	// (see the MutexSection example in the examples folder)

	// Each Task has its own mutex stack. That makes sure that within the Task,
	// mutexes are always locked in the order of their mutex priority.

	class Mutex
	{
	public:
		uint32_t mutexID;		// Dus ID = een uint32_t
		osMutexId_t mutexId;    // Dus Id = void*
		osStatus_t status;

	public:
		Mutex(uint32_t mutexID) : mutexID(mutexID)
		{
			assert(mutexID != 0);	// MutexID should not be 0. Zero is reserved (to indicate absence of mutexID)
			mutexId = osMutexNew(nullptr);
			assert(mutexId != nullptr);
		}

		void lock(Task* pTask)
		{
			while (true)
			{
				assert(mutexID > pTask->mutexIdStack.top()); // Error : Potential Deadlock : Within each thread, never try to lock a mutex with lower mutex priority than a mutex that is locked(before it).

				status = osMutexAcquire(mutexId, osWaitForever);
				if (status == osOK)
				{
					assert(pTask->mutexIdStack.push(mutexID));// Assert would mean that either the amount of nested concurrently locked mutexes for this task exceeds the constant MAX_MUTEXNESTING, or the lock and unlock of the mutex are not performed in the same task.
					break;
				}

				osThreadYield();  // WDT vriendelijk
			}
		}

		void unlock(Task* pTask)
		{
			pTask->mutexIdStack.pop();
			status = osMutexRelease(mutexId);
			assert(status == osOK);
		}
	};
};
