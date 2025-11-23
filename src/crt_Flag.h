// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"

	#include "cmsis_os2.h"
}

#include "crt_Waitable.h"
#include "crt_Task.h"

namespace crt
{
    // A Flag is a waitable.It is meant for inter task communications.
    // The task that owns the flag should wait till another task sets the flag.
    // (see the Flag example in the examples folder)

    class Flag : public Waitable
    {
    private:
        Task* pTask;

    public:
        Flag(Task* pTask) : Waitable(WaitableType::wt_Flag), pTask(pTask)
        {
            init(pTask);
        }

        Flag() : Waitable(WaitableType::wt_Flag), pTask(nullptr)
        {
            // init is postponed, when this constructor is used.
            // that allows instantiation of flags in arrays.
        }

        void init(Task* pTask)
        {
            assert(pTask!=nullptr);
            assert(!isInitialized()); // this init function should only be called once.
            Waitable::init(pTask->queryBitNumber(this));
        }

        bool isInitialized()
        {
            return (Waitable::getBitMask() != 0);
        }

        //osMutexAttr_t attr = {}; // wie gebruikt dit? laten we het eens uitcommentarieren.
        void set()
        {
            assert(pTask!=nullptr);
            pTask->setEventBits(Waitable::getBitMask());
        }

		void clear()
		{
            assert(pTask!=nullptr);
			pTask->clearEventBits(Waitable::getBitMask());
		}
    };
};

