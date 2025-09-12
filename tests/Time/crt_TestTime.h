#pragma once

// Onderstaande mumbo - jumbo is nodig, omdat deze header file
// zowel door een .cpp file als een .c file (main.c) wordt geinclude.
// Om te zorgen dat de compiler maar 1 functienaam (zonder name cpp name mangling)
// aanmaakt, en de linker daardoor niet in de war raakt, zorgen we ervoor
// dat de functie-naam altijd de "c functie naam" heeft.

#ifdef __cplusplus
extern "C" {
#endif

void testTime_init();

#ifdef __cplusplus
}
#endif
