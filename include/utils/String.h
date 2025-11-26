#ifndef _STRING_H_
#define _STRING_H_

#include "ExtendedTypes.h"

extern int contains(const char character, const char* const matches);

extern char** split(const char* string, const char* const separators, const char* const preserved_separators);

extern void format_float_to_scientific_notation(char* const buffer, float value);

extern int string_is_numeral(const char text[]);

extern int string_is_hex_numeral(const char text[]);

extern int string_is_float(const char text[]);

extern int string_is_immediate(const char text[]);

extern int string_is_asm_immediate(const char text[]);

extern int string_is_string(const char text[]);

extern uint16_t parse_immediate(const char text[]);

#define append_to_output(output, length, text) append_to_output_ext(output, length, text, strlen(text))
extern char* append_to_output_ext(char* output, long* const length, const char* const text, int text_len);

#define append_filename(filename, text) append_filename_ext(filename, text, strlen(text))
extern char* append_filename_ext(char* filename, const char* const text, int text_len);


#endif
