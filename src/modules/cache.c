#include <stdlib.h>

#include "utils/Log.h"

#include "modules/cache.h"


Cache_t* cache_create(uint16_t capacity) {
    if (capacity & (capacity - 1)) {
        log_msg(LP_ERROR, "Cache capacity has to be a power of 2 [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }

    Cache_t* cache = calloc(1, sizeof(Cache_t));
    cache->capacity = capacity;
    cache->data = malloc(sizeof(uint8_t) * cache->capacity);
    cache->address = malloc(sizeof(uint16_t) * cache->capacity);
    cache->state = malloc(sizeof(CacheState_t) * cache->capacity);

    if (!cache->data || !cache->address || !cache->state) {
        free(cache->data);
        free(cache->address);
        free(cache->state);
        return NULL;  // Return NULL on failure to allocate memory
    }

    for (int i = 0; i < cache->capacity; i++) {
        cache->address[i] = 0x0000;
        cache->data[i] = 0x00;
        cache->state[i].value = 0x0000;
    }

    return cache;
}

void cache_delete(Cache_t** cache) {
    if (!cache) {return;}
    free((*cache)->data);
    free((*cache)->address);
    free((*cache)->state);
    free(*cache);
    *cache = NULL;
}

int cache_read(Cache_t* cache, uint16_t address, uint8_t* data, int skipRead) {
    if (!cache) return 0;
    if (skipRead) {
        return 0;
    }

    uint16_t cache_address = address & (cache->capacity - 1);

    cache->state[cache_address].age += (cache->state[cache_address].age == 0xff) ? 0 : 1;
    if (!cache->state[cache_address].valid || cache->address[cache_address] != address || cache->state[cache_address].dirty) {
        cache->miss ++;
        return 0;
    }
    *data = cache->data[cache_address];
    cache->state[cache_address].uses += (cache->state[cache_address].uses == 0xff) ? 0 : 1;
    cache->hit ++;
    
    return 1;
}

int cache_write(Cache_t* cache, uint16_t address, uint8_t* data, size_t data_size, int skipWrite) {
    if (!cache) return 0;
    if (skipWrite) {
        return 0;
    }

    uint16_t cache_address = address & (cache->capacity - 1);

    for (size_t byte = 0; byte < data_size; byte++) {
        int index = (cache_address + byte) % cache->capacity;
        cache->address[index] = address + byte;
        cache->data[index] = data[byte];
        cache->state[index].dirty = 0;
        cache->state[index].valid = 1;
        cache->state[index].uses = (byte == 0); // only set use to 1 for the first byte, else 0. Why? I forgor
        cache->state[index].age = 0;
    }

    return 0;
}

int cache_invalidate(Cache_t* cache) {
    if (!cache) return 0;
    for (int i = 0; i < cache->capacity; i++) {
        cache->state[i].valid = 0;
    }
    return 1;
}
