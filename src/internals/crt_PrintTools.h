#pragma once

extern "C" {
	#include "crt_stm_hal.h"
    #include "main.h"           // bevat vaak GPIO-definities (LED_PIN, enz.)
}

namespace crt
{
	class PrintTools {
	public:
		static void print_u64(const char* label, uint64_t v)
			    {
			        char buf[32]; char* p = buf + sizeof(buf); *--p = '\0';
			        if (v == 0) *--p = '0';
			        while (v) { *--p = (char)('0' + (v % 10)); v /= 10; }
			        printf("%s: %s\r\n", label, p);
			    }
	};
} // end namespace crt
