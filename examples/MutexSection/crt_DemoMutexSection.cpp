// by Marius Versteegen, 2023

// This file contains the code of multiple tasks that run concurrently and notify eachother using flags.
#include "crt_DemoMutexSection.h"

#include <cstdio>
extern "C" {
	// put c includes here
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

// put c++ includes here
#include "crt_CleanRTOS.h"
#include "crt_MutexSection.h" // Must be included separately. (For max code clarity, use Pools and Queues instead, whenever possible).

using namespace crt;

// Separate namespace to prevent ODR-violation risk.
namespace crt_demomutexsection
{
	// Shared resources.
	extern int32_t sharedIntA;
	extern int32_t sharedIntB;
	extern int32_t sharedIntC;

	// The mutexes that guard them.
	extern Mutex mutexSharedIntA;		// Mutex with id 1, which protects sharedIntA
	extern Mutex mutexSharedIntB;		// Mutex with id 2, which protects sharedIntB
	extern Mutex mutexSharedIntC;		// Mutex with id 3, ,, .
										// NOTE: it is forbidden to lock a new mutex with an id
										// that is lower than the highest id of currently locked mutexes.
										// By using MutexSections, that rule is safeguarded.

	class SharedNumberIncreaser : public Task
	{
	private:
		int32_t& sharedInt;
		Mutex&   mutexSharedInt;

	public:
		SharedNumberIncreaser(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes,
							   int32_t& sharedInt, Mutex& mutexSharedInt) :
			Task(taskName, taskPriority, taskSizeBytes),sharedInt(sharedInt),mutexSharedInt(mutexSharedInt)
		{
			start();
		}

	private:
		void main() override
		{
			osDelay(1300); // wait for other threads to have started up as well.
			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				// Create a new clause to allow MutexSection to be used in a RAII fashion.
				{
					MutexSection ms(this,mutexSharedInt);
					sharedInt++;
				}
				osDelay(1);
			}
		}
	}; // end class SharedNumberIncreaser

	class SharedNumbersDisplayer : public Task
	{

	public:
		SharedNumbersDisplayer(const char *taskName, osPriority_t taskPriority, uint32_t taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

	private:
		void main() override
		{
			osDelay(1000); // wait for other threads to have started up as well.
			printf("\r\n------------------------\r\n");
			printf("DemoMutexSection_started\r\n");
			printf("------------------------\r\n");
			osDelay(100);

			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();

				printf("[SharedNumbersDisplayer] Sleeping 1000ms, during which other tasks can update the numbers\r\n");
				osDelay(1000);
				{
					printf("[SharedNumbersDisplayer] Waiting to obtain the mutexes\r\n");
					MutexSection msA(this,mutexSharedIntA);
					MutexSection msB(this,mutexSharedIntB);
					MutexSection msC(this,mutexSharedIntC);

					printf("[SharedNumbersDisplayer] Mutexes obtained. no one else can alter the ints now\r\n");
					printf("[sharedIntA] %" PRIi32 "\r\n", sharedIntA);
					printf("[sharedIntB] %" PRIi32 "\r\n", sharedIntB);
					printf("[sharedIntC] %" PRIi32 "\r\n", sharedIntC);
					printf("[SharedNumbersDisplayer] Sleeping 1000ms, but in spite of that no other task can change the ints\r\n");
					osDelay(1000);
					printf("[SharedNumbersDisplayer] Next three values should thus be the same as the previous three:\r\n");
					printf("[sharedIntA] %" PRIi32 "\r\n", sharedIntA);
					printf("[sharedIntB] %" PRIi32 "\r\n", sharedIntB);
					printf("[sharedIntC] %" PRIi32 "\r\n", sharedIntC);

					printf("[SharedNumbersDisplayer] Releasing the mutexes\r\n");
				}
			}
		}
	}; // end class SharedNumbersDisplayer

	// Implementation of shared resources
	int32_t sharedIntA;
	int32_t sharedIntB;
	int32_t sharedIntC;

	// The mutexes that guard them.

	// Note: this file is actually part of the main.cpp (or .ino) file, as this file is included from that.
	// It is a good idea to define all mutexes that your application needs in this file.
	// That way, you can make sure that every mutex gets a unique id, and that the
	// order of the ids is correct (when multiple mutexes are locked concurrently,
	// Within each thread (separately), MUTEXES WITH SMALLER ID MUST BE LOCKED BEFORE MUTEXES WITH LARGER ID.

	Mutex mutexSharedIntA(1);		// Mutex with id 1, which protects sharedIntA
	Mutex mutexSharedIntB(2);		// Mutex with id 2, which protects sharedIntB
	Mutex mutexSharedIntC(3);		// Mutex with id 3, ,, .
									// NOTE: it is forbidden to lock a new mutex with an id
									// that is lower than the highest id of currently locked mutexes.
									// By using MutexSections, that rule is safeguarded.
}; // end namespace crt_demomutexsection

extern "C" {
	void demoMutexSection_init()
	{
		static crt_demomutexsection::SharedNumberIncreaser sharedNumberIncreaserA
			("SharedNumberIncreaser A", osPriorityNormal /*priority*/, 1000 /*stackBytes*/,
			   crt_demomutexsection::sharedIntA, crt_demomutexsection::mutexSharedIntA);
		static crt_demomutexsection::SharedNumberIncreaser sharedNumberIncreaserB
			("SharedNumberIncreaser B", osPriorityNormal /*priority*/, 1000 /*stackBytes*/,
			   crt_demomutexsection::sharedIntB, crt_demomutexsection::mutexSharedIntB);
		static crt_demomutexsection::SharedNumberIncreaser sharedNumberIncreaserC
			("SharedNumberIncreaser C", osPriorityNormal /*priority*/, 1000 /*stackBytes*/,
			   crt_demomutexsection::sharedIntC, crt_demomutexsection::mutexSharedIntC);
		static crt_demomutexsection::SharedNumbersDisplayer sharedNumbersDisplayer
			("SharedNumbersDisplayer", osPriorityNormal /*priority*/, 1000 /*stackBytes*/);
	}
}
