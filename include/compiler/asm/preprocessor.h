#ifndef _PREPROCESSOR_H_
#define _PREPROCESSOR_H_

#define ASM_PREPROCESSOR_CAPACITY 64

typedef struct {
    char match[ASM_PREPROCESSOR_CAPACITY];    // the symbolic representation of the defined value
    char replacement[ASM_PREPROCESSOR_CAPACITY];       // the symbolic representation to replace the old one
} DefinitionMap_t;

extern DefinitionMap_t* map;
extern int map_capacity;
extern int map_length;

extern char* assembly_preprocessor_compile(const char* content);

extern char* assembly_preprocessor_compile_from_file(const char* filename);

#endif
