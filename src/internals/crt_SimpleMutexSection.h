// by Marius Versteegen, 2025

// SimpleMutex Sections can be used to avoid concurrent resource usage
// within the scope they exist.

// It is only safe to use when the protected scope is and will be protected by
// a single mutex only.

// If the protected scope may be protected (in the future) by multiple mutexes,
// MutexSection should be used instead, to avoid deadlock risks.

// Always use SimpleMutexSections in stead of locking SimpleMutexes directly.
// That way, it is ensured that every lock is always paired with a
// matching unlock().

// (see the SimpleMutexSection example in the examples folder)

#pragma once
#include "crt_SimpleMutex.h"

namespace crt
{
	// This class allows automatic Mutex release using the RAI-pattern.
	class SimpleMutexSection
	{
	private:
		SimpleMutex& simpleMutex;
	public:
		SimpleMutexSection(SimpleMutex& simpleMutex) : simpleMutex(simpleMutex)
		{
			simpleMutex.lock();
		}
		~SimpleMutexSection()
		{
			simpleMutex.unlock();
		}
	};
};
