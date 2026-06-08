#ifndef _TRANSPILER_H_
#define _TRANSPILER_H_

#include <stdint.h>

char* transpile(char* asm, long* filesize);

char* transpile_from_file(const char* const filename, long* filesize);


#endif // _TRANSPILER_H_
