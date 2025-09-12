// by Marius Versteegen, 2025

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_DemoPool.h"

extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

// put c++ includes here
#include <cstdio>
#include <stdlib.h>
#include <crt_CleanRTOS.h>

using namespace crt;

// Separate namespace to prevent ODR-violation risk.
namespace crt_demopool
{
	// Rule for the struct below: both numbers should stay equal.
	// If not, the data is compromised.

	// For instance imagine a hotelroom, the first number is the room number,
	// and the second numer is the key number, which should match the room number.

	struct TwoNumbers
	{
		TwoNumbers():number1(0),number2(0){}

		int32_t number1;
		int32_t number2;
	};

	// Shared resources.
	extern Pool<TwoNumbers>* pPoolTwoNumbersA;
	extern TwoNumbers        twoNumbersB;
	extern Pool<uint32_t>*	 pPoolInitialDelay; // Different initial delays prevent multiple
										        // tasks to printf at the same time / interleaved.

	class SharedNumberIncreaser : public Task
	{
	private:
		static void addToDelay(uint32_t& delay, uint32_t additive) {delay += additive;}

	public:
		SharedNumberIncreaser(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)//, initialDelay(initialDelay)
		{
			start();
		}

	private:
		void main() override
		{
			uint32_t initialDelay = 0;
			pPoolInitialDelay->readAtomicUpdate(initialDelay, &addToDelay, static_cast<uint32_t>(300));

			osDelay(initialDelay); // wait for other threads to have started up as well.
			printf("\r\n----------------\r\n");
			printf("DemoPool_started\r\n");
			printf("----------------\r\n");
			osDelay(100);

			uint64_t count = 0;

			TwoNumbers twoNumbers;							// temp variable to copy to and from the pool.

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				// Create a new clause to allow MutexSection to be used in a RAII fashion.
				for(int i=0; i<5; i++)
				{
					// increase all numbers by 2.
					// The numbers in the pool protected TwoNumber object are expected to remain in sync.
					pPoolTwoNumbersA->read(twoNumbers);
					twoNumbers.number1 += 2;
					twoNumbers.number2 += 2;
					pPoolTwoNumbersA->write(twoNumbers);

					// The numbers in the unprotected TwoNumber object may get out of sync because of
					// concurrent access.
					twoNumbersB.number1 += 2;
					twoNumbersB.number2 += 2;
				}
				count+=1;
				if ((count % 30) == 1)
				{
					printf("[%s] Current numbers, the first two should remain synced:\r\n", taskName);
					pPoolTwoNumbersA->read(twoNumbers);
					printf("TwoNumbersA: %" PRIi32 "\r\n", twoNumbers.number1);
					printf("TwoNumbersA: %" PRIi32 "\r\n", twoNumbers.number2);

					printf("TwoNumbersB: %" PRIi32 "\r\n", twoNumbersB.number1);
					printf("TwoNumbersB: %" PRIi32 "\r\n", twoNumbersB.number2);
				}
				osDelay(10);  // wait long enough - giving each task a chance - to avoid the chance
								 // of the watchdog timer kicking in for one of them.
			}
		}
	}; // end class SharedNumberIncreaser

	// The implementations of the shared resources
	Pool<TwoNumbers>* pPoolTwoNumbersA = nullptr;				// Protected..
	TwoNumbers twoNumbersB;				   				        // Unprotected.
	Pool<uint32_t>* pPoolInitialDelay = nullptr;
}; // end namespace crt_demopool

extern "C" {
	void demoPool_init()
	{
		// Pools gebruiken intern freertos semaphores.
		// Die kunnen alleen geldig wordenr gecreëerd nadat osKernelInitialize() is gedaan
		// (of vanuit een taak nadat osKernelStart() draait):
		// Normaal zijn Pools onderdelen van taken, en gaat dat automatisch goed.
		// Maar voor deze arcane test gebruiken we een pool los, accessible in global space,
		// vandaar deze delayed initialisatie van de pointer.
		static crt::Pool<crt_demopool::TwoNumbers> poolStorage;
		crt_demopool::pPoolTwoNumbersA = &poolStorage;

		static crt::Pool<uint32_t> poolInitialDelay(1000);
		crt_demopool::pPoolInitialDelay = &poolInitialDelay;

		static crt_demopool::SharedNumberIncreaser sharedNumberIncreaserA
			("SharedNumberIncreaser A", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
		static crt_demopool::SharedNumberIncreaser sharedNumberIncreaserB
			("SharedNumberIncreaser B", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
		static crt_demopool::SharedNumberIncreaser sharedNumberIncreaserC
			("SharedNumberIncreaser C", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
	}
}
