#include "utils/IO.h"
#include "utils/Log.h"
#include "asm/macro_code_expansion.h"

char* macro_code_expand_from_file(char* filename) {
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed");
        return NULL;
    }
    return content;
}

