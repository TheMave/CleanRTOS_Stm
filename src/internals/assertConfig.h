// assertConfig.h
#pragma once
#include <stdio.h>

// Belangrijk: vooraf <assert> dan wel <cassert> includen,
// afhankelijk van of je c of cpp gebruikt.
// .. en uiteraard TEST_MODE definen, bij tests.

#ifdef TEST_MODE

    // Assert in test mode: print en return optioneel
    #define ASSERT_RET(cond, retval)                      \
        do {                                              \
            if (!(cond)) {                                \
                printf("ASSERT FAILED: %s (%s:%d)\r\n",   \
                       #cond, __FILE__, __LINE__);        \
                return (retval);                          \
            }                                             \
        } while (0)

    // Als je niks wil returnen
    #define ASSERT(cond) ASSERT_RET(cond, )

#else

    // Assert in release / embedded: harde fout
    #define ASSERT(cond) assert(cond)
    #define ASSERT_RET(cond, retval) assert(cond)

#endif
