// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os.h"
}

#include "crt_Waitable.h"
#include "crt_Task.h"
//#include "crt_ILogger.h"
namespace crt
{
    //extern ILogger& logger;

	template<typename TYPE, uint32_t COUNT> class Queue : public Waitable
	{
	private:
		osMessageQueueId_t qh;
        Task* pTask;
        uint32_t writeDelay;
		TYPE dummy;

	public:
		Queue(Task* pTask,bool bWriteWaitIfQueueFull=false)
		: Waitable(WaitableType::wt_Queue),pTask(pTask),
          writeDelay(bWriteWaitIfQueueFull ? osWaitForever : 0)
		{
			if(pTask!=nullptr)
			{
				Waitable::init(pTask->queryBitNumber(this));
			}
			// else bitnr blijft 0, maar hoort sowieso niet gebruikt te
			// worden (door wait, waitany en waitall) voor deze queue.

			// voordeel van pTask == nullptr: eventbits worden niet
			// gebruikt. Dus wanneer read or write vanuit ISR wordt
			// aangeroepen is er geen FreeRTOS timer service nodig om
			// setEventBits delayed te handelen (perikelen: zie cpu_load_jitter test in crt_TestTimer.cpp)
			// de naturel queue heeft daar geen last van.

            qh = osMessageQueueNew(COUNT, sizeof(TYPE), nullptr);
		}

		void read(TYPE& returnVariable)
		{
			osStatus_t rc = osMessageQueueGet(qh, &returnVariable, nullptr, osWaitForever);
			if (rc != osOK)
			{
				printf("osMessageGet failed\r\n"); vTaskDelay(1);
			}

            if((pTask!=nullptr) && (osMessageQueueGetCount(qh) > 0))
            {
                // The queue is not empty yet,
                // Make sure that the corresponding eventbit gets set again,
                // Such that a wait for the queue will fire.
                pTask->setEventBits(Waitable::getBitMask());
            }
            else
            {
                // Just in case this queue.read is called directly, without preceding
                // waitAny or waitAll (which would have cleared the corresponding bit).
            }
			//assert(rc == pdPASS);
		}

		bool write(const TYPE& variableToCopy)
		{
			osStatus_t rc = osMessageQueuePut(qh, &variableToCopy, 0/*msg_prio*/, writeDelay);

            if (rc != osOK)
            {
                // The queue got full. Note: that cannot happen if bWriteWaitIfQueueFull==true.
            	// TODO: deal with other potential things that may have happened (switch rc..)
            	assert(false);
                return false;
            }
            if(pTask!=nullptr)
            {
            	pTask->setEventBits(Waitable::getBitMask());
            }
            return true;
		}

		int getNofMessagesWaiting()
		{
			return osMessageQueueGetCount(qh);
		}

		void clear()
		{
			while (osMessageQueueGetCount(qh) > 0)
			{
				osMessageQueueGet(qh, &dummy, nullptr, osWaitForever);
			}

			if(pTask!=nullptr)
			{
				pTask->clearEventBits(Waitable::getBitMask());
			}
		}
	};
};
