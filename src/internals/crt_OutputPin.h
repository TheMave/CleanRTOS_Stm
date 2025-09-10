#pragma once

extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"           // bevat vaak GPIO-definities (LED_PIN, enz.)
}

namespace crt
{
	class OutputPin {
	public:
		// Constructor for default output pin
		OutputPin(GPIO_TypeDef* port, uint16_t pin)
		: _port(port), _pin(pin) {
			enableClock();

			GPIO_InitTypeDef cfg = {0};
			cfg.Pin = _pin;
			cfg.Mode = GPIO_MODE_OUTPUT_PP;
			cfg.Pull = GPIO_NOPULL;
			cfg.Speed = GPIO_SPEED_FREQ_LOW;
			HAL_GPIO_Init(_port, &cfg);
		}

		// Constructor for fully customized (output) pin
		OutputPin(GPIO_TypeDef* port, uint16_t pin, uint32_t mode, uint32_t pull, uint32_t speed, uint32_t altenate)
		: _port(port), _pin(pin) {
			enableClock();

			GPIO_InitTypeDef cfg = {0};
			cfg.Pin = _pin;
			cfg.Mode = mode;
			cfg.Pull = pull;
			cfg.Speed = speed;
			HAL_GPIO_Init(_port, &cfg);
		}

		inline void set()   { HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_SET); }
		inline void clear() { HAL_GPIO_WritePin(_port, _pin, GPIO_PIN_RESET); }
		inline void toggle(){ HAL_GPIO_TogglePin(_port, _pin); }

	private:
		// Only stored for debug purposes:
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
