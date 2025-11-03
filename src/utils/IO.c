#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "utils/IO.h"

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#define _FILE_OFFSET_BITS 64

char* read_file(const char* const filename, long* filesize) {
    if (!filename) {printf("\tERROR: Null pointer\n"); return NULL;}
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    if (fsize == 0) {
        printf("\tERROR: The size returned is 0 Bytes\n");
        printf("\tINFO: Perhaps the file is too large. In that case try using read_file64(const char* const, long long int*)\n");
    }
    if (filesize) {*filesize = fsize;}
    fseek(file, 0, SEEK_SET);

    char* data = malloc(fsize + 1);
    if (!data) {
        printf("\tERROR: Memory allocation failure\n");
        fclose(file);
        return NULL;
    }

    fread(data, 1, fsize, file);
    fclose(file);
    data[fsize] = '\0';
    return data;
}

#include <stdio.h>
#include <stdlib.h>

char* read_file64(const char* const filename, long long int* filesize) {
    if (!filename) {
        printf("\tERROR: Null pointer\n");
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return NULL;
    }

    // Use fseeko and ftello instead of Windows-specific _fseeki64/_ftelli64
    if (fseek(file, 0, SEEK_END) != 0) {
        printf("\tERROR: Unable to seek file\n");
        fclose(file);
        return NULL;
    }

    long long int fsize = (long long int)ftell(file);
    if (fsize < 0) {
        printf("\tERROR: Unable to determine file size\n");
        fclose(file);
        return NULL;
    }

    if (filesize) {
        *filesize = fsize;
    }

    rewind(file);  // Equivalent to fseeko(file, 0, SEEK_SET)

    char* data = malloc(fsize + 1);
    if (!data) {
        printf("\tERROR: Memory allocation failure\n");
        fclose(file);
        return NULL;
    }

    fread(data, 1, fsize, file);
    fclose(file);
    data[fsize] = '\0';

    return data;
}


char* read_file_partial(const char* const filename, long bytes, long offset, long* filesize) {
    if (!filename) {
        printf("\tERROR: Null pointer\n");
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    if (fsize == 0) {
        printf("\tERROR: The size returned is 0 Bytes\n");
        printf("\tINFO: Perhaps the file is too large. In that case try using read_file_partial64(const char* const, long long int, long long int, long long int*)\n");
    }
    if (filesize) {
        *filesize = (fsize < offset + bytes ? bytes : fsize - offset);
    }

    if (offset >= fsize) {
        printf("\tERROR: Offset beyond end of file\n");
        fclose(file);
        return NULL;
    }

    if (offset + bytes > fsize) {
        bytes = fsize - offset;
    }

    fseek(file, offset, SEEK_SET);

    char* data = malloc(bytes + 1);
    if (!data) {
        printf("\tERROR: Memory allocation failure\n");
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(data, 1, bytes, file);
    fclose(file);

    data[read_bytes] = '\0';

    return data;
}

char* read_file_partial64(const char* const filename, long long int bytes, long long int offset, long long int* filesize) {
    if (!filename) {
        printf("\tERROR: Null pointer\n");
        return NULL;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long long int fsize = ftell(file);
    if (filesize) {
        *filesize = (fsize >= offset + bytes ? bytes : fsize - offset);
    }

    if (offset >= fsize) {
        printf("\tERROR: Offset beyond end of file\n");
        fclose(file);
        return NULL;
    }

    if (offset + bytes > fsize) {
        bytes = fsize - offset;
    }

    fseek(file, offset, SEEK_SET);

    char* data = malloc(bytes + 1);
    if (!data) {
        printf("\tERROR: Memory allocation failure\n");
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(data, 1, bytes, file);
    fclose(file);

    data[read_bytes] = '\0';

    return data;
}

int data_export(const char* const filename, const void* data, size_t size) {
    if (!filename) {printf("\tERROR: Null pointer\n"); return 0;}
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return 0;
    }
    fwrite(data, 1, size, file);
    fclose(file);
    return 1;
}

int data_append(const char* const filename, const void* data, size_t size) {
    if (!filename) {printf("\tERROR: Null pointer exception\n"); return 0;}
    FILE* file = fopen(filename, "ab");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename); 
        return 0;
    }
    fwrite(data, 1, size, file);
    fclose(file);
    return 1;
}

int data_import(const char* const filename, void* data, size_t size) {
    if (!filename) {printf("\tERROR: Null pointer\n"); return 0;}
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("\tERROR: Unable to open file \"%s\"\n", filename);
        return 0;
    }
    size_t bytes_read = fread(data, 1, size, file);
    if (bytes_read < size) {
        printf("\tERROR: Only %zu bytes read out of %zu\n", bytes_read, size);
    }
    fclose(file);
    return 1;
}

int append_file_format(const char* const filename, const char* format, ...) {
    if (!filename || !format) {printf("\tERROR: Null pointer exception\n"); return 0;}

    FILE* file = fopen(filename, "a");  // Open the file in append mode
    if (!file) {printf("\tERROR: Unable to open file \"%s\"\n", filename); return 0;}

    va_list args;
    va_start(args, format);
    int result = vfprintf(file, format, args);  // Use vfprintf to append the formatted string to the file
    va_end(args);

    fclose(file);

    return result >= 0 ? 1 : 0;  // Check vfprintf result and return success or failure
}

int file_exists(const char* const filename) {
    FILE *file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return 1;
    }
    return 0;
}
