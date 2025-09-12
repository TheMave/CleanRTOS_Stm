// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"
	#include "cmsis_os2.h"
	#include <inttypes.h>
}

#include <cstdio>
#include "crt_CleanRTOS.h"
#include "crt_IndexPool.h"

namespace crt
{
	class TestIndexPoolTask : public Task
	{

	public:
		TestIndexPoolTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
			Task(taskName, taskPriority, taskSizeBytes)
		{
			start();
		}

	public:
		// Note: the static tests below can be called as well without instantiating a TestIndexPoolTask Object.

		// === Interface ===

		// Call testLifeCycleFunctions_human_readable to show some
		// human readable examples.
		static void testLifeCycleFunctions_human_readable() {testLifeCycleFunctions_human_readable_impl();}

		// Call testLifeCycleFunctions_auto for automatic testing.
		static void testLifeCycleFunctions_auto() 			{testLifeCycleFunctions_auto_impl();};

		// Call doAutoTests() to do all automatic tests.
		static void doAutoTests()							{doAutoTests_impl();}

	private:
		// === Implementations ===

		static void testLifeCycleFunctions_human_readable_impl()
		{
			printf("Constructing IndexPool<3> indexPool" "\r\n");
			IndexPool<3> indexPool; // with max 3 indices.
			int32_t nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);

			int32_t nMaxNofIndices = indexPool.getMaxNofIndices();
			printf("nMaxNofIndices: %" PRIi32 "\r\n", nMaxNofIndices);

			int32_t nMemoryUsage = indexPool.getMemoryUsage();
			printf("nMemoryUsage: %" PRIi32 "\r\n", nMemoryUsage);

			bool bResult = indexPool.isEmpty();
			printf("isEmpty bool result: %" PRIi32 "\r\n", int32_t(bResult));

			int32_t nIndex0 = indexPool.getNewIndex();
			printf("GetNewIndex yields idx0: %" PRIi32 "\r\n", nIndex0);

			bResult = indexPool.isEmpty();
			printf("isEmpty bool result: %" PRIi32 "\r\n", int32_t(bResult));

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);

			int32_t nIndex1 = indexPool.getNewIndex();
			printf("GetNewIndex yields idx1: %" PRIi32 "\r\n", nIndex1);

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);

			indexPool.releaseIndex(nIndex0);
			printf("Released: %" PRIi32 "\r\n", nIndex0);

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);

			int32_t nIndex0_b = indexPool.getNewIndex();
			printf("GetNewIndex yields idx0_b: %" PRIi32 "\r\n", nIndex0_b);

			bResult = indexPool.isIndexUsed(2);
			printf("isIndexUsed(2) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			int32_t nIndex2 = indexPool.getNewIndex();
			printf("GetNewIndex yields idx2: %" PRIi32 "\r\n", nIndex2);

			bResult = indexPool.isIndexUsed(2);
			printf("isIndexUsed(2) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			// next should assert, because max 3 indices.
			// int32_t nIndex3 = indexPool.getNewIndex();
			// printf("GetNewIndex yields idx3: %" PRIi32 "\r\n", nIndex3);

			bResult = indexPool.claimIndex(2);
			printf("ClaimIndex(2) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			// next should assert, because max 3 indices.
			//int32_t nIndex3_claimed = indexPool.claimIndex(3);
			//printf("ClaimIndex(3) yields idx3_claimed: %" PRIi32 "\r\n", nIndex3_claimed);

			printf("releaseIndex(2)" "\r\n");
			indexPool.releaseIndex(2);

			bResult = indexPool.isIndexUsed(2);
			printf("isIndexUsed(2) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			int32_t nIndex2_claimed = indexPool.claimIndex(2);
			printf("ClaimIndex(2) yields bool result: %" PRIi32 "\r\n", nIndex2_claimed);


			printf("iterate through all indices and print" "\r\n");

			int32_t iterator = indexPool.UNDEFINED;
			int32_t index = indexPool.getFirst(iterator);
			printf("index = getFirst(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);

			while (iterator!=indexPool.UNDEFINED)
			{
				index = indexPool.getNext(iterator);
				printf("index = getNext(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);
			}

			printf("iterate through all indices, remove part and print" "\r\n");
			printf("(should work, because it is explicitly supported)" "\r\n");
			iterator = indexPool.UNDEFINED;
			index = indexPool.getFirst(iterator);
			printf("index = getFirst(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);

			while (iterator!=indexPool.UNDEFINED)
			{
				if(index==0)
				{
					indexPool.releaseIndex(index);
					printf("indexPool.releaseIndex(index); with index: %" PRIi32 "\r\n", index);
				}

				index = indexPool.getNext(iterator);
				printf("index = getNext(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);
			}

			bResult = indexPool.isEmpty();
			printf("isEmpty bool result: %" PRIi32 "\r\n", int32_t(bResult));

			bResult = indexPool.isIndexUsed(0);
			printf("isIndexUsed(0) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);

			printf("indexPool.reset()" "\r\n");
			indexPool.reset();

			bResult = indexPool.isEmpty();
			printf("isEmpty bool result: %" PRIi32 "\r\n", int32_t(bResult));

			bResult = indexPool.isIndexUsed(2);
			printf("isIndexUsed(2) yields bool result: %" PRIi32 "\r\n", int32_t(bResult));

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			printf("nNofIndicesInUse: %" PRIi32 "\r\n", nNofIndicesInUse);


			printf("refill by 3x getNewIndex" "\r\n");
			nIndex0 = indexPool.getNewIndex();
			nIndex1 = indexPool.getNewIndex();
			nIndex2 = indexPool.getNewIndex();

			printf("loop through contents once more" "\r\n");
			iterator = indexPool.UNDEFINED;
			index = indexPool.getFirst(iterator);
			printf("index = getFirst(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);

			while (iterator!=indexPool.UNDEFINED)
			{
				index = indexPool.getNext(iterator);
				printf("index = getNext(iterator): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);
			}

			index = indexPool.getNext(iterator);
			printf("index = getNext(iterator==-1): %" PRIi32 " iterator: %" PRIi32 "\r\n", index, iterator);
		}

		static void testLifeCycleFunctions_auto_impl()
		{
			printf("testLifeCycleFunctions_automatic()" "\r\n");
			IndexPool<3> indexPool; // with max 3 indices.
			int32_t nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==0);

			int32_t nMaxNofIndices = indexPool.getMaxNofIndices();
			assert(nMaxNofIndices==3);

			int32_t nMemoryUsage = indexPool.getMemoryUsage();
			assert(nMemoryUsage==32);

			bool bResult = indexPool.isEmpty();
			assert(bResult);

			int32_t nIndex0 = indexPool.getNewIndex();
			assert(nIndex0==0); // pool: 0

			bResult = indexPool.isEmpty();
			assert(!bResult);

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==1);

			int32_t nIndex1 = indexPool.getNewIndex();
			assert(nIndex1==1); // pool: 0, 1

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==2);

			indexPool.releaseIndex(nIndex0); // pool: 1

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==1);

			int32_t nIndex0_b = indexPool.getNewIndex();
			assert(nIndex0_b==0); // pool: 1, 0

			bResult = indexPool.isIndexUsed(2);
			assert(!bResult);

			int32_t nIndex2 = indexPool.getNewIndex(); // pool: 1, 0, 2
			assert(nIndex2==2);

			bResult = indexPool.isIndexUsed(2);
			assert(bResult);

			// next should assert, because max 3 indices.
			// int32_t nIndex3 = indexPool.getNewIndex();
			// printf("GetNewIndex yields idx3: %" PRIi32 "\r\n", nIndex3);

			bResult = indexPool.claimIndex(2);
			assert(!bResult); // already claimed via getNewIndex.

			// next should assert, because max 3 indices.
			//int32_t nIndex3_claimed = indexPool.claimIndex(3);
			//printf("ClaimIndex(3) yields idx3_claimed: %" PRIi32 "\r\n", nIndex3_claimed);

			indexPool.releaseIndex(2); // pool: 1, 0

			bResult = indexPool.isIndexUsed(2);
			assert(!bResult); // already claimed via getNewIndex.

			bResult = indexPool.claimIndex(2); // pool: 1, 0, 2
			assert(bResult);

			//Iterate through all indices and print
			int32_t iterator = indexPool.UNDEFINED;
			int32_t index = indexPool.getFirst(iterator);
			assert(index==2);
			assert(iterator==1);

			index = indexPool.getNext(iterator);
			assert(index==0);
			assert(iterator==0);

			index = indexPool.getNext(iterator);
			assert(index==1);
			assert(iterator==-1);

			// Iterate through all indices, remove part and print
			//(should work, because it is explicitly supported)
			iterator = indexPool.UNDEFINED;
			index = indexPool.getFirst(iterator);
			assert(index==2);
			assert(iterator==1);

			index = indexPool.getNext(iterator);
			assert(index==0);
			assert(iterator==0);

			indexPool.releaseIndex(0);	// pool: 1, 2
			index = indexPool.getNext(iterator);
			assert(index==1);
			assert(iterator==-1);

			bResult = indexPool.isEmpty();
			assert(!bResult);

			bResult = indexPool.isIndexUsed(0);
			assert(!bResult);

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==2);

			indexPool.reset();

			bResult = indexPool.isEmpty();
			assert(bResult);

			bResult = indexPool.isIndexUsed(2);
			assert(!bResult);

			nNofIndicesInUse = indexPool.getNofIndicesInUse();
			assert(nNofIndicesInUse==0);

			// Refill by 3x getNewIndex
			nIndex0 = indexPool.getNewIndex();
			nIndex1 = indexPool.getNewIndex();
			nIndex2 = indexPool.getNewIndex();

			//loop through contents once more
			iterator = indexPool.UNDEFINED;
			index = indexPool.getFirst(iterator);
			assert(index==2);
			assert(iterator==1);

			index = indexPool.getNext(iterator);
			assert(index==1);
			assert(iterator==0);

			index = indexPool.getNext(iterator);
			assert(index==0);
			assert(iterator==-1);

			index = indexPool.getNext(iterator); // while iterator is INVALID..
			assert(iterator==-1);
			assert(index==-1);
			printf("testLifeCycleFunctions_automatic succesful" "\r\n");
		}

		static void doAutoTests_impl()
		{
			testLifeCycleFunctions_auto();
		}

		void main() override
		{
			while (true)
			{
				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				testLifeCycleFunctions_human_readable();
				doAutoTests();

				dumpStackHighWaterMarkIfIncreased(); 		// This function call takes about 0.25ms! It should be called while debugging only.

				vTaskDelay(500000);
			}
		}
	};
};// end namespace crt
