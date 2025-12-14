#pragma once
#include <cassert>
#include <cstdint>
#include "crt_CustomHeap.h"
#include "internals/crt_SimpleMutexSection.h"

namespace crt
{
// Een bijna Thread-safe variant van CustomHeap:
// Het is thread-safe zolang meerdere tasks niet objecten met
// dezelfde handle bepotelen (omdat een reference naar het interne
// object wordt geretourneerd).
template <typename T, size_t MAX_NOF_ITEMS, typename INT = int32_t>
class CustomHeapTS
{
private:
    T storage[MAX_NOF_ITEMS];
    IndexPool<MAX_NOF_ITEMS, INT> indexPool;
    SimpleMutex simpleMutex;

public:
    const INT UNDEFINED = indexPool.UNDEFINED;

    CustomHeapTS() {}

    // Allocate a new item, returns handle or UNDEFINED
    INT allocate(const T& item)
    {
        SimpleMutexSection sms(simpleMutex);

        INT handle = indexPool.getNewIndex();
        if (handle == indexPool.UNDEFINED)
            return indexPool.UNDEFINED;

        storage[handle] = item;
        return handle;
    }

    // Release a handle back to the heap
    void release(INT handle)
    {
        SimpleMutexSection sms(simpleMutex);

        assert(indexPool.isIndexUsed(handle));
        indexPool.releaseIndex(handle);
    }

    // Access item by handle (non-const)
    T& get(INT handle)
    {
        SimpleMutexSection sms(simpleMutex);

        assert(indexPool.isIndexUsed(handle));
        return storage[handle];
    }

    // Check if handle is valid
    bool isHandleValid(INT handle)
    {
        SimpleMutexSection sms(simpleMutex);
        return indexPool.isIndexUsed(handle);
    }

    size_t getSizeUsed()
    {
        SimpleMutexSection sms(simpleMutex);
        return indexPool.getNofIndicesInUse();
    }

    size_t getSizeFree()
    {
        SimpleMutexSection sms(simpleMutex);
        return getCapacity() - indexPool.getNofIndicesInUse();
    }

    size_t getCapacity() const
    {
        return MAX_NOF_ITEMS;
    }
};

} // namespace crt
