#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdlib.h>
#include <stdint.h>


typedef union CacheState_t {
    union {
        uint16_t value;  // Full status
        struct {
            uint16_t valid : 1;          // cache is holding actual data and has a valid address associated
            uint16_t dirty : 1;          // data in RAM is newer than in CACHE [when written to RAM but CACHE not updated]
            uint16_t modified : 1;       // data in CACHE is newer than in RAM [when written to CACHE but RAM not updated] [UNUSED]
            uint16_t unused : 5;
            uint16_t uses : 8;           // how often it has been read from [alternatively "decay"?]
            uint16_t age : 8;           // how often it has been read from [alternatively "decay"?]
        };
    };
} CacheState_t;

typedef struct Cache_t {
    uint16_t capacity;
    uint8_t* data;              // holds the data (read only). Should be invalidated at write to corresponding address
    uint16_t* address;          // holds the representative address in ram
    uint64_t hit, miss;
    CacheState_t *state;
} Cache_t;


extern Cache_t* cache_create(uint16_t capacity);

extern void cache_delete(Cache_t* cache);

extern int cache_read(Cache_t* cache, uint16_t address, uint8_t* data, int skipRead);

// cache_write returns 1 if a dirty write has happened, else 0
extern int cache_write(Cache_t* cache, uint16_t address, uint8_t* data, size_t data_size, int skipWrite);

extern int cache_invalidate(Cache_t* cache);

#endif

