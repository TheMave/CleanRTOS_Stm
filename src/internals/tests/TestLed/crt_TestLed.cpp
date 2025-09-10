#include "crt_TestLed.h"

#include <cstdio>
extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"  // bevat vaak GPIO-definities
	#include "cmsis_os.h"
}

#include "crt_OutputPin.h"
#include "crt_Task.h"

// link naar de huart2 instance in main.c
// (it has been put there by the cubeide configurator after activating USART2 there)
extern UART_HandleTypeDef huart2;

namespace crt
{
	class LedTask : public Task
	{
	private:
		OutputPin ledPin;

	public:
		LedTask(const char *taskName, osPriority_t taskPriority,
	             uint32_t taskStackSizeBytes)
		: Task(taskName, taskPriority, taskStackSizeBytes),
		  ledPin(GPIOA, GPIO_PIN_5)
		{
			start();
		}

		void testToggle()
		{
			for(;;)
			{
				ledPin.toggle();
				printf("Toggled!\r\n");
				osDelay(500);
			}
		}

		void testManualToggle()
		{
			for(;;)
			{
				ledPin.set();
				printf("set!\r\n");
				osDelay(2000);
				ledPin.clear();
				printf("clear!\r\n");
				osDelay(2000);
			}
		}

		void main() override
		{
			//testToggle();
			testManualToggle();
		}
	};
}// end namespace crt

void testLed_init()
{
	printf("Init vanuit C++\n");
	static crt::LedTask ledTask("LedTask", osPriorityNormal, 2000/*Bytes*/);
}
