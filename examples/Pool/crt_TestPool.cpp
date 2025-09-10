// by Marius Versteegen, 2025

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_TestPool.h"

#include <cstdio>
extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os.h"
	#include <inttypes.h>
}

#include <crt_CleanRTOS.h>

namespace crt
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
	extern Pool<TwoNumbers> poolTwoNumbersA;
	extern TwoNumbers       twoNumbersB;

	class SharedNumberIncreaser : public Task
	{
	public:
		SharedNumberIncreaser(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
     		start();
		}

	private:
		void main() override
		{
			vTaskDelay(1000); // wait for other threads to have started up as well.

			uint64_t count = 0;

			TwoNumbers twoNumbers;							// temp variable to copy to and from the pool.

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				// Create a new clause to allow MutexSection to be used in a RAII fashion.
				for(int i=0; i<20; i++)
				{
					// increase all numbers by 2.
					// The numbers in the pool protected TwoNumber object are expected to remain in sync.
				    poolTwoNumbersA.read(twoNumbers);
					twoNumbers.number1 += 2;
					twoNumbers.number2 += 2;
					poolTwoNumbersA.write(twoNumbers);

					// The numbers in the unprotected TwoNumber object may get out of sync because of
					// concurrent access.
					twoNumbersB.number1 += 2;
					twoNumbersB.number2 += 2;
				}
				count+=1;
				if ((count % 100) == 1)
				{
				    printf("[%s] Current numbers, the first two should remain synced:\r\n", taskName);
				    poolTwoNumbersA.read(twoNumbers);
				    printf("TwoNumbersA: %" PRIi32 "\r\n", twoNumbers.number1);
				    printf("TwoNumbersA: %" PRIi32 "\r\n", twoNumbers.number2);

				    printf("TwoNumbersB: %" PRIi32 "\r\n", twoNumbersB.number1);
				    printf("TwoNumbersB: %" PRIi32 "\r\n", twoNumbersB.number2);
				}
				vTaskDelay(10);  // wait long enough - giving each task a chance - to avoid the chance
				                 // of the watchdog timer kicking in for one of them.
			}
		}
	}; // end class SharedNumberIncreaser

	Pool<crt::TwoNumbers> poolTwoNumbersA;						    // Protected..
	TwoNumbers twoNumbersB;				   				            // Unprotected..
};// end namespace crt

extern "C" {
	void testPool_init()
	{
		printf("Starting Test"); vTaskDelay(1000);
		static crt::SharedNumberIncreaser sharedNumberIncreaserA("SharedNumberIncreaser A", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
		static crt::SharedNumberIncreaser sharedNumberIncreaserB("SharedNumberIncreaser B", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
		static crt::SharedNumberIncreaser sharedNumberIncreaserC("SharedNumberIncreaser C", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
	}
}
