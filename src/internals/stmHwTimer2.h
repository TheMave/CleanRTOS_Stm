#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


//typedef void (*TimerCallback)(void);
typedef void (*TimerCallback)(void* userData); // niet nodig voor cleanRTOS,
											   // we gebruiken de simpeler en snellere variant.

void timer2_init();
void timer2_set_callback(TimerCallback cb, void* userData);
void timer2_fire_after_us(uint32_t delay_us);

void timer2_pause(void);
void timer2_resume();
void timer2_set_callback(TimerCallback cb, void* userData);
uint8_t timer2_is_running(void);

#ifdef __cplusplus
}
#endif
