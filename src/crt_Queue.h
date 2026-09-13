// by Marius Versteegen, 2025

#pragma once

// Alleen activeren in geval van test om te zien of het 
// zonder de faciliteiten die tryRead biedt inderdaad niet veilig is.
// Spoiler: dat is dus niet veilig, dus onderstaande define 
// normaal gesproken NIET gebruiken!
// #define CRT_QUEUE_WIDEN_DOORBELL_RACE

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
	#include "c_printing.h"
}

#include "crt_Waitable.h"
#include "crt_Task.h"
//#include "crt_ILogger.h"
namespace crt
{
    //extern ILogger& logger;

	// A Queue is a waitable. It is meant for inter task communications.
	// The task that owns the queue waits for another task to put something into it.
	//
	// Contract:
	//   * wait(queue) with nothing else to wait for  -> read()     (blocking)
	//   * after waitAny()/hasFired() on this queue   -> tryRead()  (never read())
	// See the comments at read() and tryRead() below for the reason.
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

		// Blokkeert tot er iets in de queue staat. Prima als de taak niets anders
		// te doen heeft dan wachten op deze queue (zoals LongTimerRelay, of
		// wait(queue) gevolgd door read() in examples/Queue/crt_DemoQueue.cpp).
		//
		// LET OP: gebruik deze functie NIET na een waitAny()/hasFired() op deze
		// queue - gebruik dan tryRead(). Het event-bit is namelijk een belletje
		// dat los van de queue wordt bijgehouden, en het kan aan blijven staan
		// terwijl de queue al leeg is (zie write()). Wie dat belletje gelooft en
		// hier binnenloopt, blijft voorgoed staan wachten op iets dat er niet
		// meer is - en dat ziet er van buitenaf uit als een taak die zomaar dood
		// is gegaan.
		void read(TYPE& returnVariable)
		{
			osStatus_t rc = osMessageQueueGet(qh, &returnVariable, nullptr, osWaitForever);
			if (rc != osOK)
			{
				printf("osMessageGet failed\r\n"); vTaskDelay(1);
			}
			syncEventBit();
		}

		// Leest zonder ooit te blokkeren; geeft false als de queue leeg was.
		// Dit is de veilige tegenhanger van read() voor een taak die met
		// waitAny() op meerdere dingen tegelijk wacht: het event-bit is dan een
		// aanwijzing, geen belofte, en de queue zelf is de enige waarheid.
		// Alleen aanroepen vanuit de eigenaar-taak (niet vanuit een ISR).
		bool tryRead(TYPE& returnVariable)
		{
			// Timeout 0: een lege queue geeft osErrorResource terug, geen blokkade.
			bool bGotOne = (osMessageQueueGet(qh, &returnVariable, nullptr, 0) == osOK);
			syncEventBit();		// ook als er niets stond: dan stond het bit ten onrechte aan.
			return bGotOne;
		}

		[[nodiscard]] bool write(const TYPE& variableToCopy)
		{
			osStatus_t rc = osMessageQueuePut(qh, &variableToCopy, 0/*msg_prio*/, writeDelay);

            if (rc != osOK)
            {
                // The queue got full. Note: that cannot happen if bWriteWaitIfQueueFull==true.
            	// TODO: deal with other potential things that may have happened (switch rc..)
            	safe_printf("crt::Queue::write failed (queue full or rc=%d)\r\n", (int)rc);
                return false;
            }
            if(pTask!=nullptr)
            {
#ifdef CRT_QUEUE_WIDEN_DOORBELL_RACE
            	// ALLEEN VOOR TESTS (projectsymbool, zie tests/TestQueueDoorbell) - geen fix.
            	// Zet het venster open dat hieronder sowieso al bestaat: het item staat na
            	// osMessageQueuePut al in de queue, maar de lezer weet dat pas na
            	// setEventBits. Wordt de schrijver daar precies tussenin weggedrukt en
            	// leegt de lezer de queue intussen helemaal, dan zet de regel hieronder
            	// alsnog een bit waar niets meer achter zit. Een lezer die daarna read()
            	// doet, blijft op een lege queue hangen; tryRead() niet.
            	// osThreadYield geeft de CPU aan gelijk-geprioriteerde taken: precies de
            	// lezer. (Niet in de pTask==nullptr-tak: die queue wordt vanuit een ISR
            	// beschreven, waar yield zinloos is.)
            	osThreadYield();
#endif
            	pTask->setEventBits(Waitable::getBitMask());
            }
            return true;
		}

		int getNofMessagesWaiting()
		{
			return osMessageQueueGetCount(qh);
		}

		bool isFull()
		{
			return (getNofMessagesWaiting()==COUNT);
		}

		bool isEmpty()
		{
			return (getNofMessagesWaiting()==0);
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

	private:
		// Brengt het event-bit in overeenstemming met wat er werkelijk in de
		// queue staat. Alleen de eigenaar-taak roept dit aan, vanuit read() /
		// tryRead(). (Vanuit een ISR gaat osEventFlagsClear via de FreeRTOS
		// timer-daemon en kan dan pas later - en in andere volgorde - landen.)
		//
		// De dubbele controle is geen overdaad. Tussen het vaststellen dat de
		// queue leeg is en het wissen van het bit kan een schrijver er alsnog
		// iets in zetten. Die schrijver zet daarna zelf ook het bit, maar dat kan
		// net voor ons wissen gebeurd zijn - dan zouden we zijn belletje
		// wegpoetsen en zou zijn commando blijven liggen tot er toevallig een
		// volgende schrijver langskomt. Na het wissen dus nog een keer kijken:
		// wat er dan staat, is van na het wissen, en daar hoort het bit bij.
		void syncEventBit()
		{
			if (pTask == nullptr) return;	// queue zonder event-bit (bijv. LongTimerRelay)

			if (osMessageQueueGetCount(qh) > 0)
			{
				pTask->setEventBits(Waitable::getBitMask());
				return;
			}

#ifdef CRT_QUEUE_WIDEN_LOST_DOORBELL_RACE
			// ALLEEN VOOR TESTS (projectsymbool, zie tests/TestQueueDoorbell) - geen fix.
			// Vergroot het venster tussen "queue is leeg" en het wissen van het bit,
			// zodat een schrijver er tussen kan komen. Zonder de tweede controle
			// hieronder zou diens belletje worden gewist en zijn item blijven liggen.
			// Een delay en geen yield: een yield geeft alleen de beurt aan een taak
			// die op dat moment al klaarstaat, en een schrijver zit meestal nog in
			// een osDelay. 20 ms geeft hem gegarandeerd de kans.
			osDelay(20);
#endif
			pTask->clearEventBits(Waitable::getBitMask());

			if (osMessageQueueGetCount(qh) > 0)
			{
				pTask->setEventBits(Waitable::getBitMask());
			}
		}
	};
};
