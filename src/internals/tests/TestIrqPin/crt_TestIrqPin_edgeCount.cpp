// by Marius Versteegen, 2025
//
// Test for crt::IrqPin in "edge-count + latched callback" mode.
//
// Behavior:
// - Configure PA9 as EXTI falling-edge input with pull-up.
// - Enable IRQ.
// - Wait until the IrqPin callback sets a CleanRTOS Flag (this should happen only on edgeCount 0->1).
// - Then wait 5 seconds, during which all edges are counted by IrqPin.
// - After 5 seconds, print how many edges were seen (takeEdgeCount), and go back to waiting on the flag.
//
// NOTE:
// This test assumes IrqPin implements:
//   - edge counting only when autoDisableOnFire == false
//   - dispatch() calls the user callback only on edgeCount transition 0 -> 1
//   - takeEdgeCount() atomically reads+clears the counter

#include "crt_TestIrqPin_edgeCount.h"

extern "C" {
    #include "crt_stm_hal.h"
    #include "main.h"
    #include "cmsis_os2.h"
}

#include <cstdint>
#include "crt_CleanRTOS.h"

#include "crt_OutputPin.h"
#include "crt_IrqPin.h"

using namespace crt;

namespace crt_testirqpin_edgecount {

    // LED selection: use CubeMX label if present, otherwise default to PA5.
    #if defined(LD2_GPIO_Port) && defined(LD2_Pin)
        static GPIO_TypeDef* const LED_PORT = LD2_GPIO_Port;
        static const uint16_t      LED_PIN  = LD2_Pin;
    #else
        static GPIO_TypeDef* const LED_PORT = GPIOA;
        static const uint16_t      LED_PIN  = GPIO_PIN_5;
    #endif

    class TestIrqEdgeCountTask : public Task
    {
    private:
        OutputPin led;
        IrqPin    button;
        Flag      irqFlag;

        static void buttonIsr(void* ctx, uint16_t pinMask)
        {
            (void)pinMask;
            static_cast<TestIrqEdgeCountTask*>(ctx)->onButtonIsr();
        }

        void onButtonIsr()
        {
            // IMPORTANT:
            // This test relies on IrqPin calling the callback only once per "batch"
            // (0->1 edgeCount transition). This avoids ISR->RTOS call storms.
            irqFlag.set();
        }

    public:
        TestIrqEdgeCountTask(const char *taskName, osPriority_t taskPriority, unsigned int taskSizeBytes) :
            Task(taskName, taskPriority, taskSizeBytes),
            led(LED_PORT, LED_PIN),
            // PA9 on Seeed LoRa-E5 mini header "D9" (corner pin) in many pinout diagrams.
            // Active-low pushbutton to GND => falling edge with pull-up.
            //
            // autoDisableOnFire MUST be false for edge counting mode.
            button(GPIOA, GPIO_PIN_9, this, &TestIrqEdgeCountTask::buttonIsr,
                   IrqTrigger::ON_FLANK_DOWN, GPIO_PULLUP, 5, 0, false /*autoDisableOnFire*/),
            irqFlag(this)
        {
            start();
        }

    private:
        void main() override
        {
            osDelay(800);

            safe_printf("\r\n-----------------------------\r\n");
            safe_printf("TestIrqPin_EdgeCount started\r\n");
            safe_printf("-----------------------------\r\n");
            safe_printf("PA9 configured as EXTI (falling). IRQ disabled for 2 seconds...\r\n");
            osDelay(2000);

            // Ensure we start from a clean state
            button.clearPending();
            button.clearEdgeCount();

            safe_printf("Enabling IRQ now.\r\n");
            button.enable();

            safe_printf("Press the button to START a 5-second counting window.\r\n");
            safe_printf("All falling edges during that 5 seconds are counted; the first press is included.\r\n");

            while (true)
            {
                // Wait until first edge arrives (callback should be invoked only on edgeCount 0->1).
                wait(irqFlag);

                safe_printf("\r\nWindow started: counting edges for 5 seconds...\r\n");
                osDelay(5000);

                const uint32_t edges = button.takeEdgeCount();
                safe_printf("Edges counted in last 5 seconds: %lu\r\n", (unsigned long)edges);

                // Visible feedback that a window finished
                led.toggle();

                safe_printf("Press the button again to start a new 5-second window.\r\n");
            }
        }
    };

} // namespace crt_testirqpin_edgecount

void testIrqPin_edgeCount_init()
{
    safe_printf("Init IRQ pin edge-count test vanuit C++\r\n");
    static crt_testirqpin_edgecount::TestIrqEdgeCountTask testTask("TestIrqPinEdgeCount", osPriorityNormal, 2000 /*Bytes*/);
}
