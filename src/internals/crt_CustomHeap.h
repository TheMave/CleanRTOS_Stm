#pragma once
#include <cassert>
#include <cstdint>
#include "crt_CustomHeap.h"  // zorg dat IndexPool beschikbaar is
#include <cstddef>

// Snelle CustomHeap, maar niet threadsafe (gebruik daar CustomHeapTS voor).
// Dus alleen veilig bruikbaar door een enkele thread.
namespace crt 
{
	template <typename T, size_t MAX_NOF_ITEMS, typename INT = int32_t>
	class CustomHeap 
	{
	private:
		T storage[MAX_NOF_ITEMS];
		IndexPool<MAX_NOF_ITEMS, INT> indexPool;

	public:
		CustomHeap() {}

		// Allocate a new item, returns handle (INT) or UNDEFINED if full
		INT allocate(const T& item) {
			INT handle = indexPool.getNewIndex();
			if (handle == indexPool.UNDEFINED) return indexPool.UNDEFINED;
			storage[handle] = item;
			return handle;
		}

		// Release a handle back to the heap
		void release(INT handle) {
			assert(indexPool.isIndexUsed(handle));
			indexPool.releaseIndex(handle);
		}

		// Access item by handle
		T& get(INT handle) {
			assert(indexPool.isIndexUsed(handle));
			return storage[handle];
		}

		const T& get(INT handle) const {
			assert(indexPool.isIndexUsed(handle));
			return storage[handle];
		}

		// Query the heap
		bool isHandleValid(INT handle) const {
			return indexPool.isIndexUsed(handle);
		}

		size_t getSizeUsed() const {
			return indexPool.getNofIndicesInUse();
		}

		size_t getSizeFree() const
		{
			return getCapacity() - indexPool.getNofIndicesInUse();
		}

		size_t getCapacity() const {
			return MAX_NOF_ITEMS;
		}
	};
} // namespace crt
