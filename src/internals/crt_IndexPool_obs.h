// The Index Pool is used to manage a collection of unique, reusable
// "index tokens": a preallocated range of integer numbers starting from 0.

// TODO: maak ook een variatie op basis van std arrays, en vergelijk de performance.
// (voor overzichtelijker code en auto-bounds checks)

#pragma once
#include <cstdint>
#include "crt_IndexPool.h" // Zorg dat je IndexPool template beschikbaar is

namespace crt
{
	template <typename T, int32_t NOF_ITEMS, typename INT = int32_t>
	struct CustomHeap
	{
		T arItems[NOF_ITEMS];                         // Array van items
		crt::IndexPool<NOF_ITEMS, INT> indexPool;     // IndexPool voor beheer van vrije indices
	};
};
