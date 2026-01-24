// by Marius Versteegen, 2025

#pragma once

extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"
}

namespace crt
{
	class InputPin {
	public:
		// Constructor for input pin with configurable pull-up/down and speed
		InputPin(GPIO_TypeDef* port, uint16_t pin, bool pullup, bool highSpeed)
		: _port(port), _pin(pin) {
			enableClock();

			GPIO_InitTypeDef cfg = {0};
			cfg.Pin = _pin;
			cfg.Mode = GPIO_MODE_INPUT;
			cfg.Pull = pullup ? GPIO_PULLUP : GPIO_NOPULL;
			cfg.Speed = highSpeed ? GPIO_SPEED_FREQ_HIGH : GPIO_SPEED_FREQ_LOW;
			HAL_GPIO_Init(_port, &cfg);
		}

		inline bool isHigh() { return HAL_GPIO_ReadPin(_port, _pin) == GPIO_PIN_SET; }
		inline bool isLow()  { return HAL_GPIO_ReadPin(_port, _pin) == GPIO_PIN_RESET; }

	private:
		GPIO_TypeDef* 	_port;
		uint16_t 		_pin;

		inline void enableClock() {
			if (_port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
			else if (_port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
			else if (_port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
#ifdef GPIOD
			else if (_port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
#endif
#ifdef GPIOE
			else if (_port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
#endif
#ifdef GPIOF
			else if (_port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
#ifdef GPIOG
			else if (_port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
#endif
#ifdef GPIOH
			else if (_port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
#endif
		}
	};
} // end namespace crt
