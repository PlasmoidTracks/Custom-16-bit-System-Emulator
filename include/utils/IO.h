#ifndef _IO_H_
#define _IO_H_

#include <stdlib.h>

// reads a files content in binary mode and returns the content which is allocated in the heap and NULL terminated
// filesize is optional. If it is not NULL then the filesize of the file will be writting to filesize
extern char* read_file(const char* const filename, long* filesize);

extern char* read_file64(const char* const filename, long long int* filesize);

extern char* read_file_partial(const char* const filename, long bytes, long offset, long* filesize);

extern char* read_file_partial64(const char* const filename, long long int bytes, long long int offset, long long int* filesize);

extern int data_export(const char* const filename, const void* data, size_t size);

extern int data_import(const char* const filename, void* data, size_t size);

extern int data_append(const char* const filename, const void* data, size_t size);

extern int append_file_format(const char* const filename, const char* format, ...);

extern int file_exists(const char* const filename);

extern long file_size(const char *filename);

#endif
