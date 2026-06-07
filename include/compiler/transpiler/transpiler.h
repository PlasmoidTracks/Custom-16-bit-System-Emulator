#ifndef _TRANSPILER_H_
#define _TRANSPILER_H_

#include <stdint.h>

char* transpile(uint8_t* bin, long binary_size, long* file_size);

char* transpile_from_file(const char* const filename, long* filesize);


#endif // _TRANSPILER_H_
