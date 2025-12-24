// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
}
#include <cassert>

#include "stdio.h"
#include "crt_Config.h"
#include "crt_Waitable.h"
#include "crt_std_Stack.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "c_printing.h"

using namespace crt_std;
namespace crt
{
	class Task
	{
	public:
		const char*			taskName;
		const osPriority_t 	taskPriority;
		const uint32_t 		taskStackSizeBytes;

		osThreadId_t 		taskHandle;

		uint32_t nofWaitables;
        uint32_t queuesMask;        // Every bit in this mask belongs to a queue.
        uint32_t flagsMask;         // Every bit in this mask belongs to a flag.
        uint32_t timersMask;        // Every bit in this mask belongs to a timer.

        uint32_t latestResult;
		crt_std::Stack<uint32_t, MAX_MUTEXNESTING> mutexIdStack;

	private:
		osEventFlagsId_t   hFlags;

	protected:
		UBaseType_t prev_stack_hwm;

	public:
		Task(const char *taskName, osPriority_t taskPriority, uint32_t taskStackSizeBytes)
		    : taskName(taskName), taskPriority(taskPriority),
		      taskStackSizeBytes(taskStackSizeBytes), taskHandle(nullptr),
		      nofWaitables(0), queuesMask(0), flagsMask(0), timersMask(0), latestResult(0),
			  mutexIdStack(0) /* The value 0 is reserved for "empty stack*/, hFlags(nullptr),
			  prev_stack_hwm(0)
		{
		    hFlags = osEventFlagsNew(nullptr);
		    assert(hFlags != nullptr);
		}

        void start()
        {
        	// In ARM FreeRTOS ports, stack_size is soms in StackSize_t = 4 bytes per size. Ik dacht bij blackpill.

        	const osThreadAttr_t  task_attributes({
        			          .name = taskName,
        			          .stack_size = taskStackSizeBytes, // by e5 wel gewoon bytes.. ((taskStackSizeBytes+3)/4),
        			          .priority = taskPriority,
        			      });
        	taskHandle = osThreadNew(staticMain, this, &task_attributes);
        	assert(taskHandle != nullptr);
        }

		virtual void main() = 0;

        uint32_t queryBitNumber(Waitable* pWaitable)
        {
            assert(nofWaitables < 24);
            switch (pWaitable->getType())
            {
            case WaitableType::wt_Queue:
                queuesMask |= (1 << nofWaitables);
                break;
            case WaitableType::wt_Timer:
                timersMask |= (1 << nofWaitables);
                break;
            case WaitableType::wt_Flag:
                flagsMask |= (1 << nofWaitables);
                break;
            default:
                break;
            }
            return nofWaitables++;
        }


        // crt_Task.h/.cpp
//        inline void setEventBits(const uint32_t uxBitsToSet)
//        {
//            // Detecteer ISR context
//            if (__get_IPSR() != 0U) {
//                // FreeRTOS-weg is gegarandeerd ISR-safe en forceert preemption indien nodig
//                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//                EventGroupHandle_t eg = (EventGroupHandle_t)hFlags;
        // onderstaande gebruikt defferred set via FreeRTOS Timer service.
//                BaseType_t ok = xEventGroupSetBitsFromISR(eg, (EventBits_t)uxBitsToSet, &xHigherPriorityTaskWoken);
//                assert(ok != pdFAIL);
//                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//            } else {
//                // Thread-context: reguliere CMSIS-call
//                uint32_t r = osEventFlagsSet(hFlags, uxBitsToSet);
//                assert(r != osFlagsErrorISR);          // zou hier nooit mogen
//                assert((r & osFlagsError) == 0);
//            }
//        }
//
        inline void setEventBits(const uint32_t uxBitsToSet)
        {
        	// Should be safe to call from ISR as well, in CMSIS-2.
        	uint32_t error = osEventFlagsSet(hFlags,uxBitsToSet);
        	assert(error != osFlagsErrorResource);
        	assert(error != osFlagsErrorParameter);
        	assert((error & osFlagsError)==0); // Make sure the main bit indicating an error is not set.

//			// In sommige CMSIS builds is osEventFlagsSet-from-ISR uitgeschakeld.
//			// Detecteer ISR en gebruik dan direct de FreeRTOS API.
//			if (__get_IPSR() != 0U) { // we zitten in een ISR
//				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//				EventGroupHandle_t eg = (EventGroupHandle_t)hFlags;
//				BaseType_t ok = xEventGroupSetBitsFromISR(eg, (EventBits_t)uxBitsToSet, &xHigherPriorityTaskWoken);
//				assert(ok != pdFAIL);
//				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//			} else {
//				uint32_t r = osEventFlagsSet(hFlags, uxBitsToSet);
//				assert((r & osFlagsError) == 0);
//			}
//
//
//#define osFlagsError          0x80000000U ///< Error indicator.
//#define osFlagsErrorUnknown   0xFFFFFFFFU ///< osError (-1).
//#define osFlagsErrorTimeout   0xFFFFFFFEU ///< osErrorTimeout (-2).
//#define osFlagsErrorResource  0xFFFFFFFDU ///< osErrorResource (-3).
//#define osFlagsErrorParameter 0xFFFFFFFCU ///< osErrorParameter (-4).
//#define osFlagsErrorISR       0xFFFFFFFAU ///< osErrorISR (-6).
//
//        	typedef enum {
//        	  osOK                      =  0,         ///< Operation completed successfully.
//        	  osError                   = -1,         ///< Unspecified RTOS error: run-time error but no other error message fits.
//        	  osErrorTimeout            = -2,         ///< Operation not completed within the timeout period.
//        	  osErrorResource           = -3,         ///< Resource not available.
//        	  osErrorParameter          = -4,         ///< Parameter error.
//        	  osErrorNoMemory           = -5,         ///< System is out of memory: it was impossible to allocate or reserve memory for the operation.
//        	  osErrorISR                = -6,         ///< Not allowed in ISR context: the function cannot be called from interrupt service routines.
//        	  osStatusReserved          = 0x7FFFFFFF  ///< Prevents enum down-size compiler optimization.
//        	} osStatus_t;
        }

        inline void clearEventBits(const uint32_t uxBitsToClear)
        {
            osEventFlagsClear(hFlags, uxBitsToClear);
        }

        // Wait for a single waitable.
        inline void wait(Waitable& waitable)
        {
            waitAll(waitable.getBitMask());
        }

        // WaitAll waits till ALL the specified waitables have fired.
        // It automatically clears the event-bits, apart from the queue bits.
        // So there's no need to check with hasFired.
		inline void waitAll(uint32_t bitsToWaitFor)
		{
			latestResult = osEventFlagsWait(
				hFlags,
				bitsToWaitFor,
				osFlagsWaitAll,
				osWaitForever); // xTicksToWait)

            // Actually, we didn't want to clear the queue bits, so let's repair that:
            setEventBits(queuesMask & latestResult);
		}

		// return value: the bits that were set at the time of firing.
        // Use hasFired() to determine which one.
        // It is possible that multiple event bits were set at the same time.
        // For a clean design, stop after finding the first one using hasFired.
        //
        // Thus, it is advised always to process only the actions on a single event after a waitAny.
		inline void waitAny(uint32_t bitsToWaitFor)
		{
			latestResult = osEventFlagsWait(
				hFlags,
				bitsToWaitFor,
				osFlagsWaitAny | osFlagsNoClear,
				osWaitForever); // xTicksToWait)
		}

		inline bool hasFired(Waitable& waitable)
		{
            uint32_t bitmask = waitable.getBitMask();
			bool result = ((latestResult & bitmask) != 0);
            if (result)
            {
                // "Manually" Clear, except if a queue is involved for a queue, only a read() may consume the event that signals that there's someting in the queue.
                clearEventBits((~queuesMask) & bitmask);
            }
            return result;
		}

		// Peek if the waitable has fired without resetting it and without waiting for it.
        inline bool isSet(Waitable& waitable)
        {
        	return isAllSet(waitable.getBitMask());
        }

        // The function below can be used to peek if the waitables have fired,
        // without resetting them or waiting for them.
        inline bool isAllSet(uint32_t bitsToWaitFor)
        {
			latestResult = osEventFlagsWait(
				hFlags,
				bitsToWaitFor,
				osFlagsWaitAll | osFlagsNoClear,
				0); // xTicksToWait)

			return (bitsToWaitFor == latestResult);
        }

        // The function below can be used to peek if any waitables have fired,
        // without resetting them or waiting for them.
        // You could test which one afterward, using the function hasFired.
        inline bool isAnySet(uint32_t bitsToWaitFor)
        {
			latestResult = osEventFlagsWait(
				hFlags,
				bitsToWaitFor,
				osFlagsWaitAny | osFlagsNoClear,
				0); // xTicksToWait)

			return ((bitsToWaitFor & latestResult) != 0);
        }

		inline const char* getName()
		{
			return taskName;
		}

		// Next construct allows the main thread of the task to be run in a non-static function.
		// That way, we can easily create multiple task objects from the same class.
		static void staticMain(void *pParam)
		{
			Task* pTask = (Task*)pParam;
			pTask->main();
		}


		// Next function gives an indication of whether the task (still) has enough stack and heap
		// memory available.
        inline void dumpStackHighWaterMarkIfIncreased()
        {
#ifdef CRT_HIGH_WATERMARK_INCREASE_LOGGING
            UBaseType_t temp = uxTaskGetStackHighWaterMark(NULL);
            if (!prev_stack_hwm || temp < prev_stack_hwm)
            {
                prev_stack_hwm = temp;

                safe_printf("Task %s has left: %lu stack bytes and %u heap bytes\n",
                       taskName,
                       prev_stack_hwm * sizeof(StackType_t),
                       (unsigned int)xPortGetFreeHeapSize());
            }
#endif
        }
	};
}// end namespace crt
