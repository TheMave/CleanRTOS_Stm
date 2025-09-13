// by Marius Versteegen, 2025
//
// This file is included by CleanRTOS.h
// Normally, you don't need to change the settings below.

#pragma once
//#define CRT_DEBUG_LOGGING - ook voor stm?
#define CRT_HIGH_WATERMARK_INCREASE_LOGGING

namespace crt
{
	const uint32_t MAX_MUTEXNESTING = 20;

	// below, the mutexIDs directly involved in this test can be found.
	const uint32_t MutexID_Logger = (1 << 30);	// High ID, so can be nested very deeply.
};
