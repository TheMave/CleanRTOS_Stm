// by Marius Versteegen, 2025

#pragma once
#include "crt_CleanRTOS.h"   // defines crt::MAX_NOF_TIMERS and using Timers = ...
#include "crt_LongTimerRelay.h"

// A Timer is a microsecond timer. It can be fire once (20 us or more) or periodic(50 us or more).
// The timer is also a waitable. It can be waited for by the task that owns it.
// If waits of longer than 1000 us are needed, it is better to use vTaskDelay instead (to avoid
// running out of hardware timers).

// Assumption for safe behaviour: Timer should not be destroyed before
// being stopped or having finally fired.
// This is automatically achieved when always preallocating all required timers
// and not destroying them - which is the advised way of embedded programming.

namespace crt
{
	class Timer;
	struct TimerCallBackInfo
	{
		Timer* pTimer;
		uint32_t bitMask;

		// bitmask is set by calling timer.setBitMask from Waitables during Task initialisation.
		TimerCallBackInfo() : pTimer(nullptr), bitMask(0)
		{}
		void init(Timer* pTimer, uint32_t bitMask)
		{
			this->pTimer = pTimer;
			this->bitMask = bitMask;
		}
	};

	class Timer : public Waitable
	{
		friend class TestTimer;

	private:
		TimerHandle hTimer;

		TimerCallBackInfo timerCallBackInfo;
        Task* pTask;

		bool bPeriodic;				// set to true if this Timer should auto-restart after firing.
													   // Hw timer2 (which we are using now) is 32 bit.
													   // For times larger than 1<<32 in us, chopup is needed.
													   // you could temporarily set this to a lower value to test chopup
													   // without waiting very long.
		uint32_t maxHwTime = (uint32_t)((((uint64_t)1)<<32/*bits of used timer, timer2*/)-1);

		// Everything used from both freertos task and ISR needs to be volatile.
		volatile bool bLongTimeChoppingActive;	// In such a case, this bool is true.
		volatile uint64_t totalLongTime_us;	    // .. and totalLongTime_us reflects the total time to wait.
		volatile uint64_t currentlyWaiting_us;	// .. while currentlyWaiting_us reflects the time that the currently
												// activated hardware timer is waiting.
												// In case timer2 (32 bits is used), that is typically ((1<<32)-1).
		volatile uint64_t waitTimeFiredSoFar_us; // Is increased by currentlyWaiting_us, as soon as hardware timer fires.
										// Theoretically, a user could issue a new timer command after
										// a rearm request was send to the relay. And before the relay could
										// handle it. In that case, when the relay calls back, it can
										// verify from longTimerRunId whether the current rearm request is
										// not yet obsolete and can be discarded.
		volatile uint32_t longTimerRunId; // id that separates subsequent longtimer sessions from one another.
		                                  // It helps to indicate if a firing ISR should still be served.
		                                  // (for which it needs the original member variables, so no new start or stops after starting the initial long timer start)
		// obsolete:
		//uint64_t chopUpCounter;
		//uint64_t chopUpCount;		// large times, that don't fit in a uint32, need to be chopped up.
									// In that case, chopUpCount > 1, which implies that the hwCounter
									// is operated like a periodical counter. After counting chopUpCount
									// periodical hw-fires, this Timer calls its client.
									// If this Timer is non-periodical, it issues a stop to the hwtimer.
	public:
        Timer(Task* pTask):Waitable(WaitableType::wt_Timer), hTimer(Timers::TimerHandle_None), pTask(pTask),
		bPeriodic(false), bLongTimeChoppingActive(false), totalLongTime_us(0), currentlyWaiting_us(0),
		waitTimeFiredSoFar_us(0), longTimerRunId(0)
		{
            Waitable::init(pTask->queryBitNumber(this));	// This will cause the bitmask of Waitable to be set properly.
            timerCallBackInfo.init(this, Waitable::getBitMask());
        }

        void createIfNeeded()
        {
        	if(crt::Timers::isValidTimerHandle(hTimer)) return; // already created
            hTimer = crt::Timers::createTimer("myTimer", static_timer_callback, &timerCallBackInfo);
            assert(crt::Timers::isValidTimerHandle(hTimer));
        }

        // If sleeps of longer than 1000 us are needed, you could use osDelay instead.
        inline void sleep_us(uint64_t duration_us)
        {
            start(duration_us);
            pTask->wait(*this);
        }

        inline void stop()
        {
        	longTimerRunId++;
        	crt::Timers::stopTimer(hTimer);
        	pTask->clearEventBits(bitMask);
        	totalLongTime_us = 0;
        	currentlyWaiting_us = 0;
        	bLongTimeChoppingActive = false;
        }

        // For quick tests of LongTimerRelay: ability to override maxHwTime with lower value.
        inline void setMaxHwTime (uint32_t maxHwTime){this->maxHwTime = maxHwTime;}
        inline uint32_t getMaxHwTime(){return maxHwTime;}
        inline TimerHandle getTimerHandle(){return hTimer;}

	private:
        inline void handleStart(uint64_t duration_us)
        {
        	pTask->clearEventBits(bitMask); // .. from earlier run.

        	totalLongTime_us = duration_us;

        	currentlyWaiting_us = 0;
        	waitTimeFiredSoFar_us = 0;
            if(duration_us <= (uint64_t)maxHwTime)
            {
            	// Normal behaviour, no chopping required.
            	bLongTimeChoppingActive = false;

            	uint64_t overhead_compensation = 50;// TODO tweak penalty for clock etc. (calibrate using crt_TestTimer)

            	currentlyWaiting_us = (duration_us<=overhead_compensation) ? 1 : duration_us-overhead_compensation;

            	// Periodic hwTimer implies equal wait times, so only when no chopping and periodic.
            	// (it avoids restart of hw timer and is a bit faster, in that case).
            	crt::Timers::startTimer(hTimer, (uint32_t)currentlyWaiting_us, bPeriodic /*periodic*/);
            }
            else
            {
            	// chopping is required
            	bLongTimeChoppingActive = true;
				currentlyWaiting_us =  (uint64_t)maxHwTime; // maximizing wait time per chunk.

				// Theoretically, until the last chunk, it could be hw periodic,
				// saving a few microseconds. But for these long waits, that does not matter,
				// and I prefer to keep the clarity of no auto-restarts for long timers.
				crt::Timers::startTimer(hTimer, (uint32_t)currentlyWaiting_us, false /*hw periodic*/);
            }
        }

	public:
        inline void start(uint64_t duration_us)
        {
        	longTimerRunId++;
            createIfNeeded();
            assert(duration_us >= Timers::minimumWaitTimeUs);                      // assert against bad design.. ah well on 16Mhz and without compiler optimization, it should be even 200us..

            bPeriodic = false;
            handleStart(duration_us);
        }

        inline void start_periodic(uint64_t period_us)
        {
        	longTimerRunId++;
            createIfNeeded();
            assert(period_us >= Timers::minimumWaitTimeUs);                        // assert against bad design

            bPeriodic = true;
            handleStart(period_us);
        }

		static void static_timer_callback(void* arg)
		{
			TimerCallBackInfo* pWCI = (TimerCallBackInfo*)arg;
			pWCI->pTimer->timer_callback(pWCI->bitMask);
		}

		// Actually, it does not rearm if it is finished.
		// Theoretically, that case could be handled in timer_callback,
		// such that the rearm relay is not needed.

		// However, I like the simplicity of the current solution,
		// and for these long waits, the few microseconds of the extra relay
		// does not seem important.
		void rearmToContinueLongTiming(uint32_t longTimerRunId_relayed)
		{
			if(longTimerRunId != longTimerRunId_relayed)
			{
				return; // the long timing that corresponded with this call was interrupted.
			}

			waitTimeFiredSoFar_us += currentlyWaiting_us;
			uint64_t timeLeft = totalLongTime_us - waitTimeFiredSoFar_us;

			if(timeLeft < Timers::minimumWaitTimeUs)
			{
				// Not enough time left for another hw interrupt.
				// Consider it done.
			    waitTimeFiredSoFar_us = totalLongTime_us;
			    timeLeft = 0;
			}

			if(timeLeft == 0)
			{
				// Chopups finished, we may fire below.
				// And we have to restart the process in case of periodic.
				if(bPeriodic)
				{
					// restart again
					start_periodic(totalLongTime_us);
				}
				else
				{
					bLongTimeChoppingActive = false; // oneshot finished.
				}
				// else no need to stop the timer, bLongTimeChoppingActive -> !bHwPeriodic
				pTask->setEventBits(bitMask);
			}
			else
			{
				// Not ready yet.
				if(timeLeft > (uint64_t)maxHwTime)
				{
					assert(currentlyWaiting_us == (uint64_t)maxHwTime);
					// no need to change currentlyWaiting_us.
					// simply restart the timer with current settings.
					crt::Timers::startTimer(hTimer, (uint32_t)currentlyWaiting_us, false/*periodic*/);
				}
				else
				{
					// start timer for the remainder
					currentlyWaiting_us = timeLeft;
					crt::Timers::startTimer(hTimer, (uint32_t)currentlyWaiting_us, false/*periodic*/);
				}
			}
		}

		inline void timer_callback(uint32_t bitMask)
		{
			if(bLongTimeChoppingActive)
			{
				// Lange wacht → rearm via Relay
				LongTimerRelay::requestRearmTimer(this, (uint32_t)longTimerRunId);
			}
			else
			{
				//no chopping. periodic hwTimer will go on. non-periodic hwTimer will stop. no action required.
				// Set the eventbit of the client.

				// korte/gewone fire → afleveren via Relay
				LongTimerRelay::requestDeliver(this, (uint32_t)longTimerRunId);
				//pTask->setEventBits(bitMask);   Geen directe setEventBits meer in ISR!
			}


            // portYIELD_FROM_ISR(pdTRUE); No need to immmediately yield. Perhaps there are more timers that have fired,
            // for threads with even higher priority.

            // Resumingly, this is how it works ( I think :-) ):
            // The timer generates a hardware interrupt.
            // That means: running code is interrupted (program counter and stackpointer are pushed on a special stack).
            // When the timer fires, this callback function is called as interrupt handler.
            // It simply sets an event bit that can be waited on in FreeRTOS by the Task that set the timer.
            //
            // Upon exiting this function, the interrupt ends (program counter and stackpointer of where the code was
            // running prior to the interrupt are popped from the stack and used).

			// Note: if CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD would be enabled (but it is disabled by default),
			// xEventGroupSetBitsFromISR should be used instead.
		}

	private:
		friend class LongTimerRelay;
		Task*     getOwnerTask() const { return pTask; }
		uint32_t  getLongTimerRunId() const { return longTimerRunId; }
	};
};
