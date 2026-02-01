// crt_Time.cpp - C wrapper implementation for Time class
// This file ensures the C-callable wrapper function gets compiled into the binary

#include "crt_Time.h"

// Force the extern "C" function to be emitted (the inline definition is in the header)
// By having this translation unit include the header, the inline function gets compiled here
extern "C" void crt_Time_addSleepCompensationMs(uint32_t sleepMs)
{
    crt::Time::addSleepCompensationMs(sleepMs);
}
