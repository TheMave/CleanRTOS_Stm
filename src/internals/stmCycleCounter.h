// Fast performance counter. Counts clockticks.
//
//#pragma once
//
//#include <stdint.h>
//
//#ifdef __cplusplus
//extern "C" {
//#endif
//
//void startCycleCount(void);
//void resetCycleCount(void);
//uint32_t getCycleCount(void);
//
//#ifdef __cplusplus
//}
//#endif

// stmCycleCounter.h
#pragma once
#include <stdint.h>
#include "core_cm4.h"  // of core_cm3.h / core_cm7.h afhankelijk van je MCU
					   // tip van chatgpt: gebruik liever hal dan specifieke core, voor compatibility.
                       // maar ik vind dit overzichtelijker.
//#include "stm32f4xx.h"         // Device-specific CMSIS: definieert __FPU_PRESENT, IRQn_Type, etc.
//#include "crt_stm_hal.h"       // HAL (inclusief FreeRTOS.h / assert.h / …)
//#include "stmCycleCounter.h"

#ifdef __cplusplus
extern "C" {
#endif

// obs: Call startCycleCount ONLY ONCE .. it is periodically restarted. use crt_StmTime to measure microseconds instead.
static inline void __attribute__((always_inline)) startCycleCount(void) {
    // CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT  = 0;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t __attribute__((always_inline)) getCycleCount(void) {
    return DWT->CYCCNT;
}

static inline void __attribute__((always_inline)) resetCycleCount(void) {
    DWT->CYCCNT  = 0;
}

#ifdef __cplusplus
}
#endif
