#pragma once

// The mumbo-jumbo below is needed because this header file is included both by
// a .cpp file and by a .c file (main.c). To make sure the compiler generates
// only one function name (without C++ name mangling), so the linker does not
// get confused, we make sure the function name always is the "c function name".

#ifdef __cplusplus
extern "C" {
#endif

void testTimeOverflow_init();

#ifdef __cplusplus
}
#endif
