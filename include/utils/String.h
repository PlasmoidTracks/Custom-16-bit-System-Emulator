#ifndef _STRING_H_
#define _STRING_H_

#include "ExtendedTypes.h"

extern int contains(char character, char* matches);

extern char** split(const char* string, char* separators, char* preserved_separators);

extern int string_is_numeral(char text[]);

extern int string_is_hex_numeral(char text[]);

extern int string_is_float(char text[]);

extern int string_is_immediate(char text[]);

extern int string_is_asm_immediate(char text[]);

extern int string_is_string(char text[]);

extern uint16_t parse_immediate(const char text[]);

#define append_to_output(output, length, text) append_to_output_ext(output, length, text, strlen(text))
extern char* append_to_output_ext(char* output, long* length, const char* text, int text_len);

#define append_filename(filename, text) append_filename_ext(filename, text, strlen(text))
extern char* append_filename_ext(char* filename, const char* text, int text_len);


#endif
