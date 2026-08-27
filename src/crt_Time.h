// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
}

#include <cstdint>
#include <cassert>

#include "stmCycleCounter.h"
#include "crt_Task.h"

namespace crt
{
	// This version (unlike the obs version) uses a sequence counter
    // instead of taskEnterCritical, because that should be faster.
	// In practice however, it turns out it is not.
	class Time : public Task
	{
	private:
		volatile uint64_t total;
		volatile uint32_t msPerCountOverflowCheck;

		volatile uint32_t seq;

	public:
		Time(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
		Task(taskName, taskPriority, taskSizeBytes), total(0),msPerCountOverflowCheck(0),seq(0)
		{
			assert(configTICK_RATE_HZ == 1000); // in FreeRTOSConfig.h. Makes sure that osDelay(1) = 1ms.

			uint64_t overflow_time_seconds = (((uint64_t)1) << 31) / SystemCoreClock; // time per overflow of the uint32_t counter.

			msPerCountOverflowCheck = uint32_t(overflow_time_seconds) * 1000 / 2;	// 2 times less than theoretical - to be on the safe side.

			startCycleCount(); // Start cyclecount at zero, just like total is.

			instance(this); // initialize the static _pInstance variable in the function instance().

			start();
		}

		// The function instance can be used to both initialize
		// and to query it.
		// The approach below allows an explicitly constructed Time object to be passed.
		// That helps to group task creations and maintain oversight.
		static Time* instance(Time* instance = nullptr)
		{
			// I prefer function static variable over class static variable.
			// (no implementation outside class needed).
			static Time* _pInstance = nullptr;
			if(_pInstance != nullptr)
			{
				assert(instance == nullptr); // initialisation of this Task object should only happen once  (like singleton).
				// Typically, it should be done in the init function of CleanRTOS,
				// calling crt::cleanRTOS_init() before you instantiate any other tasks.
			}
			else
			{
				_pInstance = instance; // initialisation.
			}
			return _pInstance;
		}

		static inline uint64_t getTotalCycleCount()
		{
			return Time::instance()->getTotalCycleCount_impl();
		}

		static inline void updateCycleCount()
		{
			Time::instance()->updateCycleCount_impl();
		}

		static inline uint64_t getTimeSeconds()
		{
			return Time::instance()->getTimeSeconds_impl();
		}

		static inline uint64_t getTimeMilliseconds()
		{
			return Time::instance()->getTimeMilliseconds_impl();
		}
		
		static inline uint64_t getTimeMicroseconds()
		{
			return Time::instance()->getTimeMicroseconds_impl();
		}

		// Call this after waking from STOP2 to compensate for time spent sleeping.
		// The DWT cycle counter freezes during STOP2, so we need to manually add
		// the sleep duration to our accumulated cycle count.
		static inline void addSleepCompensationMs(uint32_t sleepMs)
		{
			Time::instance()->addSleepCompensationMs_impl(sleepMs);
		}

	private:
		inline void addSleepCompensationMs_impl(uint32_t sleepMs)
		{
			// Convert milliseconds to cycles: cycles = ms * clockHz / 1000
			uint64_t sleepCycles = (uint64_t)sleepMs * SystemCoreClock / 1000;

			seq++; // seq becomes odd (signals update in progress)
			total += sleepCycles;
			seq++; // seq becomes even again
		}

		// At 86MHz clockspeed and with compiler-optimization OFF, the
		// function below takes about 5us on a 401 blackpill.
		inline uint64_t getTimeMicroseconds_impl()
		{
			// Divide first, then multiply. The other way around, (cycles * 1000000)
			// overflows uint64 as soon as cycles >= 2^64/1e6, which at 48MHz happens
			// after only 4.45 days of uptime. That turned the clock into a sawtooth
			// with an unreachable ceiling, after which crt_Timers could compute an
			// absolute deadline above that ceiling which was never reached again:
			// sleep_us then permanently failed to return.
			// The remainder term is < hz, so it fits easily in uint64 after *1000000.
			// The problem now only returns after 12178 years, at 48MHz.
			// This construction stays safe as long as SystemCoreClock > 1 MHz.
			// At exactly 1 MHz, (cycles/hz) * 1e6 would again approach 2^64.
			// At 48 MHz there is a factor 48 of margin - but should CleanRTOS ever
			// run on a 32 kHz-like clock, this is the place to revisit.
			const uint64_t cycles = getTotalCycleCount_impl();
			const uint64_t hz     = uint64_t(SystemCoreClock);
			return (cycles / hz) * 1000000u + ((cycles % hz) * 1000000u) / hz;
		}

		inline uint64_t getTimeMilliseconds_impl()
		{
			// Same pattern as above. This variant would only overflow after ~12 years
			// of uptime, but is kept consistent to prevent the mistake from recurring.
			const uint64_t cycles = getTotalCycleCount_impl();
			const uint64_t hz     = uint64_t(SystemCoreClock);
			return (cycles / hz) * 1000u + ((cycles % hz) * 1000u) / hz;
		}

		inline uint64_t getTimeSeconds_impl()
		{
			return (getTotalCycleCount_impl() / SystemCoreClock);
		}

		inline void updateCycleCount_impl()
		{
			seq++; // seq becomes odd
			total += getCycleCount(); 	// aggregate content of cyclecount to total.
			resetCycleCount();			// reset cyclecount to 0.
			seq++; // seq becomes even again.
		}

		inline uint64_t getTotalCycleCount_impl()
		{
		    for (;;)
		    {
		        uint32_t startSeq = seq;                 // 1) read seq
		        if (startSeq & 1u) continue;             //    odd = update in progress -> retry

		        uint64_t totalCycleCount = total + getCycleCount();    // 2) sample the counter

		        if (seq == startSeq)                     // 3) consistent? (unchanged and even)
		            return totalCycleCount;                            //    yes -> done, otherwise retry
		    }
		}

		void main() override
		{
			osDelay(100);
			while (true)
			{
				dumpStackHighWaterMarkIfIncreased();

				updateCycleCount();
				//osDelay(1500); // for testing (below takes too long for effective testing).
				osDelay(msPerCountOverflowCheck);
			}
		}
	}; // end class StmTimers
}; // end namespace crt

// C-callable wrapper declaration for use from C code (e.g., c_Stop2Sleep.c)
// Implementation is in crt_Time.cpp
extern "C" void crt_Time_addSleepCompensationMs(uint32_t sleepMs);
