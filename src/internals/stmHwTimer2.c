
#include "stmHwTimer2.h"

#include "crt_stm_hal.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include <assert.h>

//// in Clock config, check radio button use PLL -> then "press ook on "fix .." -> results in 84MHZ ABP1 Clock for hardware timers.
//// Note: only two custom timers: timer 2 and timer 5 have 32 bit counters.
//// The code below is for timer 2. Replace all 2 by 5 to change it towards timer 5.

//// in main.c, pass next task instead of
//// StartDefaultTask (that is af function at the bottom of this file)

//// Currently, when specifying 1us, you get 5us from the hardware timer.
//// So 4us is lost to overhead. That will grow when it will be shared
//// to accomodate all timer objects of all tasks.

#include "core_cm4.h"  // of core_cm3.h / core_cm7.h afhankelijk van je MCU

//// Cycle counting
//// Wanneer moet je oppassen?
//// Als je al een debugger of trace-tool gebruikt die de DWT nodig heeft (zoals ITM via SWO), dan kunnen conflicten ontstaan.
////
//// Op sommige goedkope STM32-klonen of in low-power-modi kan DWT->CYCCNT niet betrouwbaar werken (of wordt het gepauzeerd).
////
//// Overflow na 2³² cycli (~51 ms bij 84 MHz) — dus niet bruikbaar voor lange tijdmetingen zonder overloopafhandeling.
//void startCycleCount(void) {
//    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable tracing
//    DWT->CYCCNT = 0;                                 // Reset counter
//    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // Enable counter
//}
//
//uint32_t getCycleCount(void) {
//    return DWT->CYCCNT;
//}

static TIM_HandleTypeDef htim2; // static: alleen bekend binnen deze .c file

// Timer2 en Timer5 (beide 32 bits timers) zitten op de APB1 bus.
static uint32_t getPreScalerFor1MhzTimerTick_APB1()
{
	uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();


	RCC_ClkInitTypeDef clkconfig;
	uint32_t flashLatency;

	HAL_RCC_GetClockConfig(&clkconfig, &flashLatency);

	if (clkconfig.APB1CLKDivider != RCC_HCLK_DIV1) {
		timer_clock *= 2;
	}

	uint32_t prescaler = (timer_clock / 1000000) - 1;
	return prescaler;
}

void timer2_init() {
    // Check of TIM2 al geactiveerd is door iets anders
    // blackpill code: if (RCC->APB1ENR & RCC_APB1ENR_TIM2EN) {
    if (__HAL_RCC_TIM2_IS_CLK_ENABLED()) {
        // TIM2-klok is al aan — dus mogelijk elders in gebruik
        assert(0);  // Stop hier als TIM2 al bezet is

        // Bijvoorbeeld valkuil: voorkom expliciete instantiatie van StmTimers.
        // Die wordt automatisch lazy-instantiated als singleton wanneer
        // hij voor het eerst gebruikt wordt.
    }

    __HAL_RCC_TIM2_CLK_ENABLE();
    // Activeer klok voor TIM2 zodat hardware registers toegankelijk zijn
    // Dit moet vóór elke toegang tot TIM2-configuratie

    htim2.Instance = TIM2;
    // Koppel de hardware-timer aan onze HAL-handle
    // Alle HAL-functies gebruiken deze handle als context

    htim2.Init.Prescaler = getPreScalerFor1MhzTimerTick_APB1();
    //htim2.Init.Prescaler = 84 - 1; // Gebruik voorkennis van configuratie ABP1 Timer clock staat ingesteld op 84Mz.
    // Configureer prescaler zodat de timer op 1 MHz loopt
    // → 1 tick = 1 µs

    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    // Laat TIM2 omhoog tellen vanaf 0
    // Standaard telrichting voor vertragingen

    htim2.Init.Period = 0xFFFFFFFF;
    // Maximaal bereik; we gebruiken zelf een kleinere grens (autoreload)
    // Geeft ruimte voor vertragingen tot 4294 sec

    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    // Geen extra deling — nauwkeurige timing gewenst

    HAL_TIM_Base_Init(&htim2);
    // Zet alles in de TIM2 hardware registers via HAL
    // Zonder deze stap gebeurt er niks

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    // Zorg dat er geen oude interrupt pending is
    // Belangrijk als dit niet de eerste keer is

    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
    // Activeer de update-interrupt
    // Wordt gebruikt om te reageren zodra de teller overloopt

    //HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_SetPriority(TIM2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1, 0);
    // Stel prioriteit van interrupt in — 0 is de hoogste (veilige) waarde
    // Belangrijk voor timinggevoelige toepassingen
    // Maar prioriteit moet een niveau lager om portYIELD_FROM_ISR te laten werken
    // in ISR callback. (zie comments bij de declaratie van configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)

    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    // Activeer de interrupt in het Nested Vector Interrupt Controller (NVIC)
    // Nodig om `TIM2_IRQHandler()` aan te roepen
}

//static uint8_t timer2_is_paused = 0;

inline void timer2_fire_after_us(uint32_t delay_us) {

    __HAL_TIM_SET_AUTORELOAD(&htim2, delay_us);
    // auto-reload zorgt ervoor dat na vuren de counter automatisch reset naar 0.
    // Geef aan wanneer de update-interrupt moet vuren
    // Bijvoorbeeld delay_us = 2 → interrupt na 2 µs

    __HAL_TIM_SET_COUNTER(&htim2, 0);
    // Zet de teller op 0 zodat we precies weten waar we starten
    // Maakt de delay voorspelbaar, ongeacht vorige waarde

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    // Wis eerder afgevuurde interrupts zodat we niet per ongeluk retriggeren
    // Verplicht bij hergebruik van dezelfde timer

    HAL_TIM_Base_Start_IT(&htim2);
}

uint8_t timer2_is_running(void) {
    return (htim2.Instance->CR1 & TIM_CR1_CEN) ? 1 : 0;
}

//int bGlobal = 0;

static TimerCallback timer2_callback = NULL;
static void* timer2_userData = NULL;  // argument hebben we niet nodig voor CleanRTOS

inline void timer2_set_callback(TimerCallback cb, void* userData) {
	timer2_callback = cb;
	timer2_userData = userData; // indien je met argument werkt
}

void timer2_pause(void) {
	HAL_TIM_Base_Stop_IT(&htim2);  // Stop zonder reset
}

void timer2_resume() {
    HAL_TIM_Base_Start_IT(&htim2);
}

void TIM2_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) &&
        __HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE)) {
        // Controleer of TIM2 daadwerkelijk een update interrupt heeft gegenereerd
        // Dit filtert ruis van andere bronnen uit

        __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
        // Wis de interruptflag zodat deze niet blijft vuren
        // Noodzakelijk bij one-shot gebruik

        HAL_TIM_Base_Stop_IT(&htim2);  // 🧠 cruciaal voor one-shot!
        							   //.. comment out voor periodiek.

        // Note: doe eigenlijk nooit in een hardware interrupt.
                // Die moeten zo snel mogelijk afgehandeld worden!
                // Maar allee.. nu even voor de test..
        // bGlobal=1;
        if(timer2_callback != NULL)
        {
        	timer2_callback(timer2_userData);
        }
    }
}

