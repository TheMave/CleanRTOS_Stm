// by Marius Versteegen, 2025
// IRQ/EXTI input pin abstraction for STM32 HAL (context callback + optional auto-disable + edge counter)
//
// Features:
// - Configure a GPIO as EXTI interrupt source (rising/falling/both)
// - Provide per-pin enable()/disable() (default: disabled after init)
// - Register ISR callback:
//     preferred: void isr(void* ctx, uint16_t pinMask)
//     legacy   : void isr(uint16_t pinMask)
// - Optional: autoDisableOnFire
//     If true, the EXTI line is masked immediately on first fire (in ISR context),
//     pending is cleared, and it will not fire again until user calls enable().
// - Edge counter mode (only when autoDisableOnFire == false):
//     - Each EXTI fire increments an internal counter.
//     - The user callback is only invoked when the counter transitions 0 -> 1.
//       (So: at most one callback per "batch" until the user clears the counter.)
//     - Clear the counter by calling takeEdgeCount() or clearEdgeCount() from task context,
//       to re-enable calling the ISR callback on the next pin irq event.
//
// Summarised:
// - AutoDisableOnFire==true optimally reduces processor load by letting user explicitly re-enable the irq pin.
// - AutoDisableOnFire==false (edge counter mode) optimally allows to keep track how many irq pin events occurred
//   before you decide to process it, using takeEdgeCount().
//   - Alternatively, you could not wait for ISR callback and instead use peekEdgeCount() to poll how many
//     pin events have occurred since clearEdgeCount() or startup.
//
// Integration note:
// - Many projects already define HAL_GPIO_EXTI_Callback(). To avoid multiple-definition linker errors,
//   this library exports a hook: crt_IrqPin_onHalGpioExti(GPIO_Pin).
//   Call that from your existing HAL_GPIO_EXTI_Callback().

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
        using IsrFn    = void (*)(uint16_t pinMask);                 // legacy
        using IsrCtxFn = void (*)(void* ctx, uint16_t pinMask);      // preferred

        // Preferred: callback with user context pointer.
        // After construction the interrupt is configured but DISABLED.
        IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               void*         ctx,
               IsrCtxFn      isr,
               IrqTrigger    trigger = IrqTrigger::ON_FLANK_DOWN,
               uint32_t      pull    = GPIO_NOPULL,
               uint32_t      nvicPreemptPrio = 5,
               uint32_t      nvicSubPrio     = 0,
               bool          autoDisableOnFire = false);

        // Legacy: callback without context.
        IrqPin(GPIO_TypeDef* port,
               uint16_t      pinMask,
               IsrFn         isr,
               IrqTrigger    trigger = IrqTrigger::ON_FLANK_DOWN,
               uint32_t      pull    = GPIO_NOPULL,
               uint32_t      nvicPreemptPrio = 5,
               uint32_t      nvicSubPrio     = 0,
               bool          autoDisableOnFire = false);

        void enable();           // unmasks EXTI line
        void disable();          // masks EXTI line
        void clearPending();     // clears EXTI pending bit

        uint16_t pinMask() const { return _pinMask; }
        uint8_t  line()    const { return _line; }

        void setAutoDisableOnFire(bool v) { _autoDisableOnFire = v; }
        bool autoDisableOnFire() const    { return _autoDisableOnFire; }

        // Edge counter access (meaningful only when autoDisableOnFire == false)
        uint32_t peekEdgeCount() const;
        uint32_t takeEdgeCount();     // atomically read + clear
        void     clearEdgeCount();

        // called from HAL callback/IRQ glue:
        static void dispatch(uint16_t pinMask);

    private:
        GPIO_TypeDef* _port;
        uint16_t      _pinMask;
        uint8_t       _line;
        IRQn_Type     _irqn;
        bool          _enabled;
        bool          _autoDisableOnFire;

        // Per-EXTI-line callback slots (only 1 owner per line):
        static IsrCtxFn      s_ctxFn[16];
        static void*         s_ctx[16];
        static IsrFn         s_legacyFn[16];
        static IrqPin*       s_ownerObj[16];      // needed for auto-disable + per-line options
        static GPIO_TypeDef* s_ownerPort[16];     // debug / assert double-claim

        // Edge counter per EXTI line (0..15)
        static volatile uint32_t s_edgeCount[16];

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
