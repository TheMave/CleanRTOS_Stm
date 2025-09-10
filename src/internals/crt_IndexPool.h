// The Index Pool is used to manage a collection of unique, reusable
// "index tokens": a preallocated range of integer numbers starting from 0.

// TODO: maak ook een variatie op basis van std arrays, en vergelijk de performance.
// (voor overzichtelijker code en auto-bounds checks)

#pragma once
#include <cstdint>

namespace crt
{
	template <int32_t MAX_NOF_INDICES>
	class IndexPool
	{
	public:
		const int32_t UNDEFINED = -1;               // not static, because IndexPool<3>::UNDEFINED is
													// uglier than indexPool.UNDEFINED.

	private:
		int32_t m_nSizeFreeListUsed;				// Amount of used IndexTokens, which equals
													// the index of the first indexToken in the
													// freelist of an unused indexToken.

		int32_t m_arFreeList[MAX_NOF_INDICES];
													// In the interval [0.. m_nSizeFreeListUsed-1],
													// it contains the collection of used pool-indices.
													// In the interval [m_nSizeFreeListUsed.. m_nSizeArrayReserved-1]
													// it contains the collection of unused pool-indices.
		int32_t m_arTokenToFreeListIndex[MAX_NOF_INDICES];
													// For every poolindex, this array gives the corresponding
													// entry of m_arFreeList.

	private:
		// Unsupported:
		IndexPool(const IndexPool& other) 					= delete;
		const IndexPool& operator= (const IndexPool& other) = delete;
		bool operator== (const IndexPool& other)			= delete;
		bool operator!= (const IndexPool& other)			= delete;

	public:
		// For the outside world, it's an index pool.
		// But in inside, it stores stuff in arrays, and uses local indices for that.
		// So inside the code I use the word Token to give a name to the "Index" that is
		// managed for the end user.
		// I did not name the class TokenPool though, because, the word Index implies a
		// continuous range starting from zero.

		IndexPool() : m_nSizeFreeListUsed(0)
		{
			reset();
		}

		void reset()
		{
			// Fill it with useful content.
			for(int32_t i=0; i<MAX_NOF_INDICES; i++)
			{
				m_arFreeList[i]=i;
				m_arTokenToFreeListIndex[i]=i;
			}

			m_nSizeFreeListUsed = 0;
		}

		int32_t getNewIndex()
		{
			if(m_nSizeFreeListUsed >= MAX_NOF_INDICES)
				return UNDEFINED;  // Returns UNDEFINED if MAX_NOF_INDICES already in use.

			int32_t nToken = m_arFreeList[m_nSizeFreeListUsed];

			m_nSizeFreeListUsed++;

			return nToken;
		}

		bool isIndexUsed(int32_t nIndex)
		{
			bool bIsUsed = (nIndex<MAX_NOF_INDICES) &&
						   (m_arTokenToFreeListIndex[nIndex] < m_nSizeFreeListUsed);
			return bIsUsed;
		}

		bool claimIndex(int32_t nIndex)
		{
			//assert((m_nSizeFreeListUsed<MAX_NOF_INDICES)&&(nIndex<MAX_NOF_INDICES));
			assert(nIndex<MAX_NOF_INDICES);

			// Make sure that the token at nIndex moved from the freepart to the in-use part of the freelist,
			// That is, to location m_nSizeFreeListUsed.
			// The token that was referred to by that free list postition previously is swapped with it.
			int32_t nPrevFreeListIndex							= m_arTokenToFreeListIndex[nIndex];

			// Next must hold, otherwise it is an token that is already being used:
			// assert(nPrevFreeListIndex>=m_nSizeFreeListUsed);
			if(nPrevFreeListIndex<m_nSizeFreeListUsed) return false;

			int32_t nPrevFreeListIndexOfOtherFreeToken			= m_nSizeFreeListUsed;
			int32_t nIndexOfOtherFreeToken						= m_arFreeList[nPrevFreeListIndexOfOtherFreeToken];

			// Swap them:
			m_arFreeList[nPrevFreeListIndex]					= nIndexOfOtherFreeToken;
			m_arFreeList[nPrevFreeListIndexOfOtherFreeToken]	= nIndex;

			// A change in the freelist has occurred, so m_arTokenToFreeListIndex must
			// be updated accordingly:
			m_arTokenToFreeListIndex[nIndexOfOtherFreeToken]	= nPrevFreeListIndex;
			m_arTokenToFreeListIndex[nIndex]					= nPrevFreeListIndexOfOtherFreeToken;

			m_nSizeFreeListUsed++;

			return true;
		}

		void releaseIndex (int32_t nIndex)
		{
			m_nSizeFreeListUsed--;

			// Copy the entry of freelist at sizeFreeListUsed to the position
			// of the released poolindex.
			int32_t nTokenThatWasFirstFreeCandidate				= m_arFreeList[m_nSizeFreeListUsed];
			int32_t nOriginalFreeListIndexOfReleasedToken		= m_arTokenToFreeListIndex[nIndex];

			m_arFreeList[nOriginalFreeListIndexOfReleasedToken]	= nTokenThatWasFirstFreeCandidate;
			// .. Such that the released poolindex itself can be moved to the start of the unused list.
			m_arFreeList[m_nSizeFreeListUsed]				    = nIndex;

			// Sync m_arTokenToFreeListIndex, by mirroring the last two commands in that array.
			m_arTokenToFreeListIndex[nTokenThatWasFirstFreeCandidate] = nOriginalFreeListIndexOfReleasedToken;
			m_arTokenToFreeListIndex[nIndex]						  = m_nSizeFreeListUsed;
		}

		// Return whether any token from this object is being used.
		bool isEmpty()
		{
			bool bIsEmpty = (m_nSizeFreeListUsed==0);
			return bIsEmpty;
		}

		bool isFull()
		{
			return (m_nSizeFreeListUsed==MAX_NOF_INDICES);
		}

		// Return the amount of objects that is currently used from this
		// TMemoryPoolDICarrier2 object.
		int32_t getNofIndicesInUse()
		{
			return m_nSizeFreeListUsed;
		}

		// Return the amount of objects for which memory has been reserved.
		int32_t getMaxNofIndices()
		{
			return MAX_NOF_INDICES;
		}

		int32_t getMemoryUsage()
		{
			int32_t nMemoryUsage = 0;
			nMemoryUsage += int32_t(sizeof(UNDEFINED));
			nMemoryUsage += int32_t(sizeof(m_nSizeFreeListUsed));
			nMemoryUsage += int32_t(sizeof(int32_t))*int32_t(MAX_NOF_INDICES);
			nMemoryUsage += int32_t(sizeof(int32_t))*int32_t(MAX_NOF_INDICES);
			return nMemoryUsage;
		}

		int32_t getFirst(int32_t& nIterator)
		{
			int32_t nToken = UNDEFINED;

			if(m_nSizeFreeListUsed>0)
			{
				nIterator	= (m_nSizeFreeListUsed-1);
				nToken	= m_arFreeList[nIterator];

				// Construct the iterator for the NEXT object.
				nIterator = (nIterator==0) ? UNDEFINED : (nIterator-1);
			}
			else
			{
				nIterator = UNDEFINED;	// Means: no next object left.
				// (yet the return value NULL for the current value
				//  implies the same).
			}

			return nToken;
		}

		// Note: calls of ReleaseToken() do not invalidate the iterator.
		// So next looping example works perfectly fine:
		// int32_t nIterator = 0;
		// int32_t nToken = tokenPool.GetFirst(nIterator);
		// while(nToken!=UNDEFINED)
		// {
		//	 DoSomethingWith(nToken);
		//	 tokenPool.RemoveAt(nToken);
		//
		//	 nToken = tokenPool.GetNext(nIterator);
		// }

		int32_t getNext(int32_t& nIterator)
		{
			int32_t nToken = UNDEFINED;

			if(nIterator!=UNDEFINED)
			{
				if(nIterator < m_nSizeFreeListUsed)
				{
					nToken = m_arFreeList[nIterator];

					// Prepare iterator for the NEXT object.
					nIterator = (nIterator==0) ? UNDEFINED : (nIterator-1);
				}
				else
				{
					if(m_nSizeFreeListUsed>0)
					{
						// The iterator has been passed by m_nSizeFreeListUsed.
						// That can happen, if some objects have been released.
						// No problem, just readjust to the first resulting
						// position:
						nIterator = m_nSizeFreeListUsed-1;
						if(nIterator>=0)
						{
							nToken		= m_arFreeList[nIterator];

							// Prepare iterator for the NEXT object.
							nIterator = (nIterator==0) ? UNDEFINED : (nIterator-1);
						}
					}
					else
					{
						nIterator = UNDEFINED;
					}
				}
			}

			return nToken;
		}
	};
};
