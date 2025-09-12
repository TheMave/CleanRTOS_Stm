#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
}

#include "crt_IndexPool.h"
#include <array>
#include <stdint.h>
#include <assert.h>
#include <crt_Time.h>
#include "stmHwTimer2.h"

// Precondition o use of this class: Time Object was instantianted.

namespace crt
{
	//typedef int32_t TimerHandle;
	using TimerHandle = int32_t;

	// Uses timer2 (a 32 bits timer) of the stm chip, via stmHwTimer2.
	// Extern (in CleanRTOS.h), a typedef renames the templatespecialisation to the name Timers, like this:
	// (to avoid the need of passing the template parameter around).
	// typedef crt::Timers_template<MAX_NOF_TIMERS> Timers;
	template <int32_t MAX_NOF_TIMERS>
	class Timers_template
	{
		typedef void (*TimerArgsCallback)(void*);  // the void* parameter is the userArg.

	private:
		struct HwTimer
		{
			const char* name;
			TimerArgsCallback callback;
			void* userArg;
			uint32_t sleepTime_us; // equals periodic time if bPerioc==true.
			uint64_t wakeTime_us;
			HwTimer* pNext;
			bool bPeriodic;
			bool bRunning;
			TimerHandle hTimer; // it's own entry in arTimers.

			void reset()
			{
				name = nullptr;
				userArg = nullptr;
				pNext = nullptr;
				hTimer = -1;
			}
		};

		IndexPool<MAX_NOF_TIMERS> 			  _indexPoolTimerCreation   = {};
		::std::array<HwTimer,MAX_NOF_TIMERS> _arTimers    = {};  // geprealloceerde timers, tegelijk listitems.

		HwTimer* _pHead;	     // List of "active timers" (for which the "alarm" has been set).
		TimerHandle _hTimerHardwareActivatedFor;

	public:
		constexpr static TimerHandle TimerHandle_None = -1;
		constexpr static uint64_t minimumWaitTimeUs   = 100; // Dependent on clock settings - what is still possible and feasible.

	private:
		struct FiredList { HwTimer* head=nullptr; HwTimer* tail=nullptr; };

		void collectDueTimers(uint64_t now_us, FiredList& out) {
		    while (_pHead && _pHead->wakeTime_us <= now_us) {
		        HwTimer* fired = _pHead;
		        _pHead = _pHead->pNext;
		        fired->pNext = nullptr;
		        if (out.tail) out.tail->pNext = fired; else out.head = fired;
		        out.tail = fired;
		    }
		}

		void runCallbacks(FiredList& fired) {
		    for (HwTimer* t = fired.head; t; t = t->pNext)
		        if (t->callback) t->callback(t->userArg);
		}

		void reschedulePeriodicsAndRearm(FiredList& fired, uint64_t now_us) {
		    bool headChanged = false;
		    for (HwTimer* t = fired.head; t; t = t->pNext) {
		        if (t->bPeriodic && t->bRunning) {
		            // t->wakeTime_us += (uint64_t)t->sleepTime_us;
		        	// Nee, beter onderstaande.
		        	// Liever gespreid een vertraging dan onregelmatiger perioden.
		        	t->wakeTime_us = now_us + (uint64_t)t->sleepTime_us;
		            headChanged |= insertTimerAtWakeUpTimeInList(*t);
		        } else {
		            t->bRunning = false;
		        }
		    }
		    reassignHardwareTimerInterruptToFirstInList(now_us);
		}


		// Precondition: critical section opened.
		void removeFromList(TimerHandle hTimer) {
		    HwTimer* curr = _pHead;
		    HwTimer* prev = nullptr;
		    while (curr) {
		        if (curr->hTimer == hTimer) {
		            if (prev) {prev->pNext = curr->pNext;}
		            else {_pHead = curr->pNext;} // head verwijderen. ..dit verandert de head ook..
		            curr -> pNext=nullptr; // tbv debug-overzicht
		            return;
		        }
		        prev = curr;
		        curr = curr->pNext;
		    }
		}

		// returnvalue: headchanged
		bool insertTimerAtWakeUpTimeInList(HwTimer& timer) {
		    if (_pHead == nullptr) { _pHead = &timer; timer.pNext = nullptr; return true; }

		    if (timer.wakeTime_us < _pHead->wakeTime_us) {
		        timer.pNext = _pHead;
		        _pHead = &timer;
		        return true;
		    }
		    HwTimer* prev = _pHead;
		    HwTimer* curr = _pHead->pNext;
		    while (curr && !(timer.wakeTime_us < curr->wakeTime_us)) {
		        prev = curr;
		        curr = curr->pNext;
		    }
		    prev->pNext = &timer;
		    timer.pNext = curr;
		    return false;
		}


		// Precondition: critical section opened.
		void reassignHardwareTimerInterruptToFirstInList(uint64_t now_us)
		{
		    if (_pHead == nullptr) {
		        timer2_pause();
		        _hTimerHardwareActivatedFor = TimerHandle_None;
		        return;
		    }
		    _hTimerHardwareActivatedFor = _pHead->hTimer;

		    uint64_t delta64 = (_pHead->wakeTime_us > now_us)
		                     ? (_pHead->wakeTime_us - now_us)
		                     : 1;
		    uint32_t time_us = (delta64 > UINT32_MAX) ? UINT32_MAX : (uint32_t)delta64;
		    timer2_fire_after_us(time_us);
		}


	public:
		Timers_template():_pHead(nullptr),_hTimerHardwareActivatedFor(TimerHandle_None)
		{
			timer2_init();
			timer2_set_callback(timerCallback, this);

			for (int hTimer=0; hTimer<MAX_NOF_TIMERS; hTimer++)
			{
				HwTimer& timer = _arTimers[hTimer];
				timer.pNext = nullptr;
				timer.hTimer = hTimer;
				timer.bRunning = false;
			}
		}

		// callback for hw timer2:
		inline static void timerCallback(void* userData)
		{
			Timers_template* pStmTimers = (Timers_template*)userData; // TODO gebruik Timers::instance, dan heeft deze timercallback geen argument meer nodig
			assert(pStmTimers!=nullptr);
			pStmTimers->handleHwTimerInterrupt();
		}
	
		inline static Timers_template& instance()
		{
			static Timers_template timers;
			return timers;
		}

		// The static - impl separation allows the user to use Timers::createTimer instead of
		// Timers::instance.createTimer
		inline static TimerHandle createTimer(const char* name, TimerArgsCallback callback, void* userArg)
		{
			return Timers_template::instance().createTimer_impl(name,callback,userArg);
		}


		inline static void destroyTimer(TimerHandle hTimer)
		{
			Timers_template::instance().destroyTimer_impl(hTimer);
		}


		inline static bool isTimerRunning(TimerHandle hTimer)
		{
			return Timers_template::instance().isTimerRunning_impl(hTimer);
		}


		inline static void stopTimer(TimerHandle hTimer)
		{
			Timers_template::instance().stopTimer_impl(hTimer);
		}

		inline static void startTimer(TimerHandle hTimer, uint32_t duration_us, bool bPeriodic)
		{
			Timers_template::instance().startTimer_impl(hTimer, duration_us, bPeriodic, true /*bHandlePendingWakeups*/);
		}

		inline static uint32_t getMemUsageBytes()
		{
			return Timers_template::instance().getMemUsageBytes_impl();
		}

		inline static uint32_t getMaxNofTimers()
		{
			return Timers_template::instance().getMaxNofTimers_impl();
		}

		inline static bool isValidTimerHandle(TimerHandle hTimer)
		{
			return Timers_template::instance().isValidTimerHandle_impl(hTimer);
		}

		inline static uint32_t getNofTimersInUse()
		{
			return Timers_template::instance().getNofTimersInUse_impl();
		}

	private:
		[[nodiscard]] TimerHandle createTimer_impl(const char* name, TimerArgsCallback callback, void* userArg)
		{
			taskENTER_CRITICAL(); // korte instructie. kan wel in kritieke sectie.
			TimerHandle hTimer = _indexPoolTimerCreation.getNewIndex();
			taskEXIT_CRITICAL();

			if(hTimer==_indexPoolTimerCreation.UNDEFINED)
			{
				return TimerHandle_None; // Means: failed to create handle.
			}

			HwTimer& timer = _arTimers[hTimer];

			timer.name 		= name;
			timer.callback 	= callback;
			timer.userArg   = userArg;
			timer.bPeriodic = false; // is passed and set at startTimer(), rather than at construction time.
			timer.bRunning	= false;
			timer.sleepTime_us = 0;
			timer.wakeTime_us = 0;
			timer.pNext = nullptr;

			return hTimer;
		}

		inline void startTimer_impl(TimerHandle hTimer, uint32_t duration_us_in, bool bPeriodic, bool bHandleWakeups)
		{
			assert(_indexPoolTimerCreation.isIndexUsed(hTimer));

			// zonder gcc en g++ compiler optimizations (O0):
			// uint32_t estimated_overhead_us = uint64_t(SystemCoreClock)/4400000; // Bij 84Mhz clock: 84M/1M = 84, maar delay is 19, dus nog eens delen door factor 84/19=4.4

			// met gcc en g++ compiler optimizations (O2):
			//uint32_t estimated_overhead_us = uint64_t(SystemCoreClock)/10000000;

			// Automatisch aanpassen:
			/* In een header of bronbestand dat zowel met gcc als g++ wordt gecompileerd, wordt die OPTIMIZE gezet als niet O0 */
			#if defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__)
			  /* -O1, -O2 of -O3 */
			  #define EST_OVERHEAD_DIV 10000000ULL
			#else
			  /* -O0 */
			  #define EST_OVERHEAD_DIV 4400000ULL
			#endif

			// Bij 16MHZ moet EST_OVERHEAD_DIV weer opnieuw getweakt worden.
			// Vooral bij O0 is dan de overhead enorm (150us oid).

			uint32_t estimated_overhead_us = 0;//(uint32_t)((uint64_t)SystemCoreClock / EST_OVERHEAD_DIV);

			assert(duration_us_in>estimated_overhead_us);
			uint32_t duration_us = duration_us_in - estimated_overhead_us;

			HwTimer& timer = _arTimers[hTimer];

			if(timer.bRunning)
			{
				stopTimer_impl(hTimer); // Remove current wakeup time for this timer.
			}

			timer.bPeriodic   = bPeriodic;

			uint64_t now_us   = Time::instance()->getTimeMicroseconds();
			timer.wakeTime_us = now_us + int64_t(duration_us);
			timer.sleepTime_us = duration_us;
			timer.bRunning = true;

			//bool bNewlyHwTimerStartNeeded = false;

			timer2_pause();
			taskENTER_CRITICAL();
			bool headChanged = insertTimerAtWakeUpTimeInList(timer);

			FiredList fired;
			if (bHandleWakeups) {
			    collectDueTimers(now_us, fired);
			}
			if (headChanged || fired.head) {
			    reassignHardwareTimerInterruptToFirstInList(now_us);
			}
			bool needResume = (_hTimerHardwareActivatedFor != TimerHandle_None);
			taskEXIT_CRITICAL();

			runCallbacks(fired);

			taskENTER_CRITICAL();
			reschedulePeriodicsAndRearm(fired, now_us);
			needResume = (_hTimerHardwareActivatedFor != TimerHandle_None);
			taskEXIT_CRITICAL();

			if (needResume) timer2_resume();
		}

		inline void stopTimer_impl(TimerHandle hTimer)
		{
			timer2_pause();
			taskENTER_CRITICAL();
			assert(_indexPoolTimerCreation.isIndexUsed(hTimer));

			// invariance: running timer <-> in the list.
			_arTimers[hTimer].bRunning = false; // Unmark it as running
			removeFromList(hTimer); // Kan langer duren bij veel timers.

			uint64_t now_us = Time::instance()->getTimeMicroseconds();
			FiredList fired;
			collectDueTimers(now_us, fired);
			taskEXIT_CRITICAL();

			runCallbacks(fired); // volgens cgpt veiligst om niet binnen task crit aan
			                     // te roepen, ivm mogelijke problemen als non-isr qualified
			                     // functies in een callback zou worden aangeroepen.
								 // zou poteniele latency ook beperken.

			taskENTER_CRITICAL();
			//bool bWakeupHandled = handleWakeups(now_us);
			reschedulePeriodicsAndRearm(fired, now_us);

			bool needResume = (_hTimerHardwareActivatedFor != TimerHandle_None);
			taskEXIT_CRITICAL();

			//runCallbacksAndReschedule(fired, now_us);

			if (needResume) timer2_resume();
		}

		// Typically, crt_StmTimers is used by crt::Task to act on behave of crt::HwTimer objects.
		// In that case, the function below should never be called,
		// because all Tasks and associated timers are normally instantiated at device startup and never destroyed.
		void destroyTimer_impl(TimerHandle hTimer)
		{
			assert(_indexPoolTimerCreation.isIndexUsed(hTimer));
			stopTimer_impl(hTimer);

			taskENTER_CRITICAL();

			_indexPoolTimerCreation.releaseIndex(hTimer); // Index can be reused at new createTimer call in the future.
			_arTimers[hTimer].reset();

			FiredList fired;
		    uint64_t now_us = 0;
		    bool needResume = false;

			if(hTimer==_hTimerHardwareActivatedFor)
			{
				_hTimerHardwareActivatedFor = TimerHandle_None;
			}

			// perhaps there are other timers in the list.
			// make sure that they are assigned and handled properly:
			now_us   = Time::instance()->getTimeMicroseconds();

			taskEXIT_CRITICAL();

			//handleWakeups(now_us);
			collectDueTimers(now_us, fired);

			runCallbacks(fired);

			taskENTER_CRITICAL();
			reschedulePeriodicsAndRearm(fired, now_us);

			needResume = (_hTimerHardwareActivatedFor != TimerHandle_None);

			taskEXIT_CRITICAL();

			//runCallbacksAndReschedule(fired, now_us);

			if (needResume) timer2_resume();
		}


//		// return true if one or more were handled.
//		bool handleWakeups(uint64_t now_us) {
//		    bool bWakeupHandled = false;
//		    while (_pHead && _pHead->wakeTime_us <= now_us) {
//		        HwTimer* fired = _pHead;
//		        _pHead = _pHead->pNext; // eerst poppen
//
//		        // callback aanroepen
//		        if (fired->callback) fired->callback(fired->userArg);
//		        // assumed good behaviour of user: only use this HW interrupt callback for instant
//		        // action, like setting an eventbit.
//
//		        bWakeupHandled = true;
//
//		        if (fired->bPeriodic && fired->bRunning) {
//		            fired->wakeTime_us = now_us + (uint64_t)fired->sleepTime_us;
//		            insertTimerAtWakeUpTimeInList(*fired);
//		        } else {
//		            fired->bRunning = false;
//		        }
//		    }
//		    return bWakeupHandled;
//		}

		inline bool isTimerRunning_impl (TimerHandle hTimer)
		{
			assert(_indexPoolTimerCreation.isIndexUsed(hTimer));
			return _arTimers[hTimer].bRunning;
		}

		bool handleWakeups2(uint64_t now_us)
		{
			FiredList fired;
			collectDueTimers(now_us, fired);

			runCallbacks(fired); // volgens cgpt veiligst om niet binnen task crit aan
								 // te roepen, ivm mogelijke problemen als non-isr qualified
								 // functies in een callback zou worden aangeroepen.
								 // zou poteniele latency ook beperken.

			//bool bWakeupHandled = handleWakeups(now_us);
			reschedulePeriodicsAndRearm(fired, now_us);

			bool needResume = (_hTimerHardwareActivatedFor != TimerHandle_None);
			//runCallbacksAndReschedule(fired, now_us);

			if (needResume) timer2_resume();

			return fired.head != nullptr;
		}

		inline void handleHwTimerInterrupt()
		{
			// Handle pending wakeups
			// Set hwTimer to new

			// Note: critical overlap with freertos-threads is avoided above by
			// timer2_pause - resume sections above.
			// critical section not needed here either,
			// because timer2_interrupt priority is higher than any allowed priority for CleanRTOS tasks.
			uint64_t now_us   = Time::instance()->getTimeMicroseconds();

			bool bWakeupHandled = handleWakeups2(now_us);
			if(bWakeupHandled)
			{
				reassignHardwareTimerInterruptToFirstInList(now_us);
			}
		}

		inline uint32_t getMemUsageBytes_impl()
		{
			return sizeof(*this);
		}

		inline uint32_t getMaxNofTimers_impl()
		{
			return (uint32_t)MAX_NOF_TIMERS;
		}

		inline bool isValidTimerHandle_impl(TimerHandle hTimer)
		{
			return _indexPoolTimerCreation.isIndexUsed(hTimer);
		}

		// returns amount of timers that have been created and not yet destroyed.
		inline uint32_t getNofTimersInUse_impl()
		{
			return _indexPoolTimerCreation.getNofIndicesInUse();
		}
	}; // end class Timers
}; // end namespace crt
