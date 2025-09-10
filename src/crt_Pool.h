// by Marius Versteegen, 2025

// Pools can be used to protect access to shared data without
// explicitly worrying about mutexes. The class of the shared data should provide a copy constructor.
// Internally, a SimpleMutex is used to avoid concurrent access to the encapsulated data.
// (see the Pool example in the examples folder)

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os.h"
}

#include "internals/crt_SimpleMutexSection.h"

namespace crt
{
	// This class allows automatic Mutex release using the RAI-pattern.
	template <class T> class Pool
	{
	private:
        T data;
		SimpleMutex simpleMutex;	// Unlike with Mutex, SimpleMutex does not offer deadlock protection,
	public:                         // but that is no problem because the Pool inheritely poses no deadlock thread. (no case of multiple mutexes that can have different lock orders).
		Pool()
		{}

		void write (T item)
		{
			SimpleMutexSection simpleMutexSection(simpleMutex);
			data = item;
		}

		void read (T& item)
		{
			SimpleMutexSection simpleMutexSection(simpleMutex);
			item = data;
		}
	};
};

