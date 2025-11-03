#include "utils/IO.h"
#include "asm/macro_code_expansion.h"

char* macro_code_expand_from_file(char* filename) {
    char* content = read_file(filename, NULL);
    return content;
}

