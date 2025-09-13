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
	// Deze versie (itt de obs versie) maakt gebruik van een sequence counter
    // ipv taskEnterCritical, omdat dat sneller zou moeten zijn.
	// Dat blijkt echter niet
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

			startCycleCount(); // Start cyclecount op nul, net als total is.

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
				// Typically, it should be done in the init function of CleanRTOS.
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

		static inline uint64_t getTimeMicroseconds()
		{
			return Time::instance()->getTimeMicroseconds_impl();
		}


	private:

		// Bij 86Mhz clockspeed en compiler-optimization UIT, kost
		// onderstaande functie ongeveer 5us op een 401 blackpill.
		inline uint64_t getTimeMicroseconds_impl()
		{
			return getTotalCycleCount_impl() * 1000000 / uint64_t(SystemCoreClock);
		}

		inline uint64_t getTimeSeconds_impl()
		{
			return (getTotalCycleCount_impl() / SystemCoreClock);
		}

		inline void updateCycleCount_impl()
		{
			seq++; // seq becomes odd
			total += getCycleCount(); 	// aggregate content of cyclecount to total.
			resetCycleCount();			// reset cyclecount naar 0.
			seq++; // seq becomes even again.
		}

		inline uint64_t getTotalCycleCount_impl()
		{
		    for (;;)
		    {
		        uint32_t startSeq = seq;                 // 1) lees seq
		        if (startSeq & 1u) continue;             //    odd = update bezig → opnieuw proberen

		        uint64_t totalCycleCount = total + getCycleCount();    // 2) sample de teller

		        if (seq == startSeq)                     // 3) consistent? (niet veranderd én even)
		            return totalCycleCount;                            //    ja → klaar, anders opnieuw
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
