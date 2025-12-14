#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Deze functie kan nu zowel in .c als .cpp bestanden worden aangeroepen
uint32_t c_TimeStamp_now(void);

#ifdef __cplusplus
}
#endif