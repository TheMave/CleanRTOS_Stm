// by Marius Versteegen, 2025
// IRQ/EXTI input pin abstraction for STM32 HAL (with optional context callback)
//
// Design goals:
// - Configure a GPIO as EXTI interrupt source (rising/falling/both)
// - Provide per-pin enable()/disable() (default: disabled after init)
// - Register an ISR callback, optionally with a user context pointer
//   - Context form: void isr(void* ctx, uint16_t pinMask)   (preferred)
//   - Legacy form : void isr(uint16_t pinMask)
// - Callback receives the pin mask (e.g. GPIO_PIN_13)
//
// Notes:
// - STM32 has only one EXTI line (0..15) per pin number; at runtime only one port can own a line.
// - CubeMX usually generates the EXTI IRQ handlers (EXTIxx_IRQHandler) that call HAL_GPIO_EXTI_IRQHandler().
// - HAL_GPIO_EXTI_Callback() is __weak in ST HAL; we provide a strong definition in crt_IrqPin.cpp
//   to dispatch to registered callbacks.

#pragma once

#include <cstdint>

extern "C" {
    #include "crt_stm_hal.h"
    #include "main.h"
}

// --- Assert helper ---------------------------------------------------------
// Uses FreeRTOS configASSERT if available; falls back to <cassert>.
#ifndef CRT_ASSERT
  #if defined(configASSERT)
    #define CRT_ASSERT(x) configASSERT(x)
  #else
    #include <cassert>
    #define CRT_ASSERT(x) assert(x)
  #endif
#endif

namespace crt {

    enum class IrqTrigger : uint8_t {
        ON_FLANK_UP,
        ON_FLANK_DOWN,
        ON_FLANK_BOTH
    };

    class IrqPin {
    public:
        using IsrFn    = void (*)(uint16_t pinMask);            // legacy
        using IsrCtxFn = void (*)(void* ctx, uint16_t pinMask); // preferred

        // Preferred: callback with user context pointer.
        // After construction the interrupt is configured but DISABLED.
        // Call enable() explicitly when you want to start receiving interrupts.
        IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               void*         ctx,
               IsrCtxFn      isr,
               IrqTrigger    trigger = IrqTrigger::ON_FLANK_DOWN,
               uint32_t      pull    = GPIO_NOPULL,
               uint32_t      nvicPreemptPrio = 5,
               uint32_t      nvicSubPrio     = 0);

        // Legacy: callback without context.
        IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               IsrFn         isr,
               IrqTrigger    trigger = IrqTrigger::ON_FLANK_DOWN,
               uint32_t      pull    = GPIO_NOPULL,
               uint32_t      nvicPreemptPrio = 5,
               uint32_t      nvicSubPrio     = 0);

        void enable();
        void disable();
        void clearPending();

        uint16_t pinMask() const { return _pinMask; }
        uint8_t  line()    const { return _line; }

        // Called from HAL callback/IRQ glue:
        static void dispatch(uint16_t pinMask);

    private:
        GPIO_TypeDef* _port;
        uint16_t      _pinMask;
        uint8_t       _line;
        IRQn_Type     _irqn;
        bool          _enabled;

        // Per-EXTI-line callback slots:
        static IsrCtxFn      s_ctxFn[16];
        static void*         s_ctx[16];
        static IsrFn         s_legacyFn[16];

        // Detect invalid double-claim of EXTI line at runtime
        static GPIO_TypeDef* s_ownerPort[16];

        void initCommon(IrqTrigger trigger, uint32_t pull, uint32_t nvicPreemptPrio, uint32_t nvicSubPrio);
        void claimLineOrAssert();

        void enableGpioClock();
        void enableSyscfgClock();
        static uint8_t   pinMaskToLine(uint16_t pinMask);
        static IRQn_Type lineToIrq(uint8_t line);
        static uint32_t  triggerToHalMode(IrqTrigger trigger);

        // Low-level EXTI mask / pending handling
        static void extiEnable(uint16_t pinMask);
        static void extiDisable(uint16_t pinMask);
        static void extiClearPending(uint16_t pinMask);
    };

} // namespace crt

// C-callable hook for projects that already define HAL_GPIO_EXTI_Callback elsewhere.
// Call this from your existing HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin).
extern "C" void crt_IrqPin_onHalGpioExti(uint16_t GPIO_Pin);

