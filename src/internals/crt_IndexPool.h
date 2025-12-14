#pragma once
#include <cstdint>
#include <cassert>

namespace crt
{
	template <int32_t MAX_NOF_INDICES, typename INT = int32_t>
	class IndexPool
	{
	public:
		const INT UNDEFINED = -1;

	private:
		INT m_nSizeFreeListUsed;
		INT m_arFreeList[MAX_NOF_INDICES];
		INT m_arTokenToFreeListIndex[MAX_NOF_INDICES];

	private:
		IndexPool(const IndexPool& other) = delete;
		const IndexPool& operator=(const IndexPool& other) = delete;
		bool operator==(const IndexPool& other) = delete;
		bool operator!=(const IndexPool& other) = delete;

	public:
		IndexPool() : m_nSizeFreeListUsed(0)
		{
			reset();
		}

		void reset()
		{
			for(INT i = 0; i < MAX_NOF_INDICES; i++)
			{
				m_arFreeList[i] = i;
				m_arTokenToFreeListIndex[i] = i;
			}
			m_nSizeFreeListUsed = 0;
		}

		INT getNewIndex()
		{
			if(m_nSizeFreeListUsed >= MAX_NOF_INDICES)
				return UNDEFINED;

			INT nToken = m_arFreeList[m_nSizeFreeListUsed];
			m_nSizeFreeListUsed++;
			return nToken;
		}

		bool isIndexUsed(INT nIndex) const
		{
			return (nIndex < MAX_NOF_INDICES) &&
				   (m_arTokenToFreeListIndex[nIndex] < m_nSizeFreeListUsed);
		}

		bool claimIndex(INT nIndex)
		{
			assert(nIndex < MAX_NOF_INDICES);

			INT nPrevFreeListIndex = m_arTokenToFreeListIndex[nIndex];
			if(nPrevFreeListIndex < m_nSizeFreeListUsed) return false;

			INT nPrevFreeListIndexOfOtherFreeToken = m_nSizeFreeListUsed;
			INT nIndexOfOtherFreeToken = m_arFreeList[nPrevFreeListIndexOfOtherFreeToken];

			m_arFreeList[nPrevFreeListIndex] = nIndexOfOtherFreeToken;
			m_arFreeList[nPrevFreeListIndexOfOtherFreeToken] = nIndex;

			m_arTokenToFreeListIndex[nIndexOfOtherFreeToken] = nPrevFreeListIndex;
			m_arTokenToFreeListIndex[nIndex] = nPrevFreeListIndexOfOtherFreeToken;

			m_nSizeFreeListUsed++;

			return true;
		}

		void releaseIndex(INT nIndex)
		{
			m_nSizeFreeListUsed--;

			INT nTokenThatWasFirstFreeCandidate = m_arFreeList[m_nSizeFreeListUsed];
			INT nOriginalFreeListIndexOfReleasedToken = m_arTokenToFreeListIndex[nIndex];

			m_arFreeList[nOriginalFreeListIndexOfReleasedToken] = nTokenThatWasFirstFreeCandidate;
			m_arFreeList[m_nSizeFreeListUsed] = nIndex;

			m_arTokenToFreeListIndex[nTokenThatWasFirstFreeCandidate] = nOriginalFreeListIndexOfReleasedToken;
			m_arTokenToFreeListIndex[nIndex] = m_nSizeFreeListUsed;
		}

		bool isEmpty() const { return m_nSizeFreeListUsed == 0; }
		bool isFull() const { return m_nSizeFreeListUsed == MAX_NOF_INDICES; }
		INT getNofIndicesInUse() const { return m_nSizeFreeListUsed; }
		INT getMaxNofIndices() const { return MAX_NOF_INDICES; }

		int32_t getMemoryUsage() const
		{
			int32_t nMemoryUsage = 0;
			nMemoryUsage += int32_t(sizeof(int32_t)); // UNDEFINED
			nMemoryUsage += int32_t(sizeof(INT));     // m_nSizeFreeListUsed
			nMemoryUsage += int32_t(sizeof(INT)) * MAX_NOF_INDICES; // m_arFreeList
			nMemoryUsage += int32_t(sizeof(INT)) * MAX_NOF_INDICES; // m_arTokenToFreeListIndex
			return nMemoryUsage;
		}

		INT getFirst(INT& nIterator) const
		{
			INT nToken = UNDEFINED;

			if(m_nSizeFreeListUsed > 0)
			{
				nIterator = m_nSizeFreeListUsed - 1;
				nToken = m_arFreeList[nIterator];
				nIterator = (nIterator == 0) ? UNDEFINED : (nIterator - 1);
			}
			else
			{
				nIterator = UNDEFINED;
			}

			return nToken;
		}

		INT getNext(INT& nIterator) const
		{
			INT nToken = UNDEFINED;

			if(nIterator != UNDEFINED)
			{
				if(nIterator < m_nSizeFreeListUsed)
				{
					nToken = m_arFreeList[nIterator];
					nIterator = (nIterator == 0) ? UNDEFINED : (nIterator - 1);
				}
				else
				{
					if(m_nSizeFreeListUsed > 0)
					{
						nIterator = m_nSizeFreeListUsed - 1;
						if(nIterator >= 0)
						{
							nToken = m_arFreeList[nIterator];
							nIterator = (nIterator == 0) ? UNDEFINED : (nIterator - 1);
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
