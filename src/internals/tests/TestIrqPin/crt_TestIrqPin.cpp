// by Marius Versteegen, 2025
//
// Test for crt::IrqPin (context-callback variant): no file-scope statics for flag/count.
// Uses CleanRTOS Flag to wake the task from the ISR.

#include "crt_TestIrqPin.h"

extern "C" {
    #include "crt_stm_hal.h"
    #include "main.h"
    #include "cmsis_os2.h"
}

// C++ includes
#include <cstdio>
#include "crt_CleanRTOS.h"

#include "crt_OutputPin.h"
#include "crt_IrqPin.h"   // <-- IMPORTANT: include the ctx-enabled IrqPin

using namespace crt;

namespace crt_testirqpin {

    // NOTE:
    // GPIOA/GPIOC macros are typically defined as reinterpret_cast<GPIO_TypeDef*>(BASE),
    // which is NOT a C++ constant expression. Therefore: DO NOT use constexpr here.
    // Use 'static ... const' instead.
    #if defined(LD2_GPIO_Port) && defined(LD2_Pin)
        static GPIO_TypeDef* const LED_PORT = LD2_GPIO_Port;
        static const uint16_t      LED_PIN  = LD2_Pin;
    #else
        static GPIO_TypeDef* const LED_PORT = GPIOA;
        static const uint16_t      LED_PIN  = GPIO_PIN_5;
    #endif

    class TestIrqTask : public Task
    {
    private:
        OutputPin led;
        IrqPin    button;
        Flag      irqFlag;

        volatile uint32_t irqCount;
        uint32_t          lastCount;

        // Static member function is required by the callback type,
        // but we don't need static state because 'ctx' carries 'this'.
        static void buttonIsr(void* ctx, uint16_t pinMask)
        {
            static_cast<TestIrqTask*>(ctx)->onButtonIsr(pinMask);
        }

        void onButtonIsr(uint16_t pinMask)
        {
            (void)pinMask;
            irqCount++;
            irqFlag.set();   // notify task (must be ISR-safe in CleanRTOS)
        }

    public:
        TestIrqTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
            Task(taskName, taskPriority, taskSizeBytes),
            led(LED_PORT, LED_PIN),
            // Typical user-button is active-low => falling edge, with pull-up.
            button(GPIOA, GPIO_PIN_9, this, &TestIrqTask::buttonIsr,
                   IrqTrigger::ON_FLANK_DOWN, GPIO_PULLUP, 5, 0),
            irqFlag(this),
            irqCount(0),
            lastCount(0)
        {
            start();
        }

    private:
        void main() override
        {
            osDelay(800); // match demoFlag style

            safe_printf("\r\n----------------\r\n");
            safe_printf("TestIrqTask started\r\n");
            safe_printf("----------------\r\n");

            safe_printf("IRQ is configured but DISABLED for 2 seconds.\r\n");
            osDelay(2000);

            safe_printf("Enabling IRQ now.\r\n");
            button.enable();

            while (true)
            {
                // Block until ISR sets the flag
                wait(irqFlag);

                // Process all pending interrupts since last wake-up
                uint32_t c = irqCount;
                while (lastCount < c)
                {
                    lastCount++;
                    led.toggle();
                    safe_printf("IRQ event #%lu\r\n", (unsigned long)lastCount);

                    // Example: after 10 presses, disable for 3 seconds, then enable again.
                    if (lastCount == 10)
                    {
                        safe_printf("Disabling IRQ for 3 seconds...\r\n");
                        button.disable();
                        osDelay(3000);
                        safe_printf("Enabling IRQ again.\r\n");
                        button.enable();
                    }
                }
            }
        }
    };

} // namespace crt_testirqpin

void testIrqPin_init()
{
    safe_printf("Init IRQ pin test vanuit C++\r\n");
    static crt_testirqpin::TestIrqTask testTask("TestIrqPin", osPriorityNormal, 2000 /*Bytes*/);
}
