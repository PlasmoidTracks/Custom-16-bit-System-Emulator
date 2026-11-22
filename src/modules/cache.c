#include <stdlib.h>

#include "utils/Log.h"

#include "modules/cache.h"


Cache_t* cache_create(uint16_t capacity) {
    if (capacity & (capacity - 1)) {
        log_msg(LP_ERROR, "Cache: Capacity has to be a power of 2 [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }

    Cache_t* cache = calloc(1, sizeof(Cache_t));
    cache->capacity = capacity;
    cache->data = malloc(sizeof(uint8_t) * cache->capacity);
    cache->address = malloc(sizeof(uint16_t) * cache->capacity);
    cache->state = malloc(sizeof(CacheState_t) * cache->capacity);

    if (!cache->data || !cache->address || !cache->state) {
        log_msg(LP_ERROR, "Cache: Memory allocation failure [%s:%d]", __FILE__, __LINE__);
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
    if (!*cache) {return;}
    free((*cache)->data);
    free((*cache)->address);
    free((*cache)->state);
    free(*cache);
    *cache = NULL;
}

uint16_t cache_resolve_address(Cache_t* cache, uint16_t address) {
    return address & (cache->capacity - 1);
}

int cache_read(Cache_t* cache, uint16_t address, uint8_t* data) {
    if (!cache) return 0;

    uint16_t cache_address = cache_resolve_address(cache, address);

    //cache->state[cache_address].age += (cache->state[cache_address].age == 0xff) ? 0 : 1;
    if (!cache->state[cache_address].valid || cache->address[cache_address] != address /*|| cache->state[cache_address].dirty*/) {
        cache->miss ++;
        return 0;
    }
    *data = cache->data[cache_address];
    //cache->state[cache_address].uses += (cache->state[cache_address].uses == 0xff) ? 0 : 1;
    cache->hit ++;
    
    return 1;
}

int cache_write(Cache_t* cache, uint16_t address, uint8_t* data, size_t data_size, int dirty) {
    if (!cache) return 0;

    uint16_t cache_address = cache_resolve_address(cache, address);

    for (size_t byte = 0; byte < data_size; byte++) {
        int index = (cache_address + byte) % cache->capacity;
        cache->address[index] = address + byte;
        cache->data[index] = data[byte];
        cache->state[index].dirty = dirty;
        cache->state[index].valid = 1;
        //cache->state[index].uses = (byte == 0); // only set use to 1 for the first byte, else 0. Why? I forgor
        //cache->state[index].age = 0;
    }

    return 0;
}

int cache_slot_is_dirty(Cache_t* cache, uint16_t cache_address) {
    if (!cache) return 0;
    if (cache_address > cache->capacity) return 0;
    return cache->state[cache_address].dirty;
}

int cache_address_is_dirty(Cache_t* cache, uint16_t address) {
    if (!cache) return 0;
    uint16_t cache_address = cache_resolve_address(cache, address);
    if (cache->address[cache_address] != cache_address) return 0;
    return cache->state[cache_address].dirty;
}

int cache_set_dirty_value(Cache_t* cache, uint16_t address, int dirty) {
    if (!cache) return 0;
    uint16_t cache_address = cache_resolve_address(cache, address);
    if (cache->address[cache_address] != cache_address) return 0;
    cache->state[cache_address].dirty = dirty;
    return 1;
}

int cache_slot_set_dirty_value(Cache_t* cache, uint16_t cache_address, int dirty) {
    if (!cache) return 0;
    if (cache_address > cache->capacity) return 0;
    cache->state[cache_address].dirty = dirty;
    return 1;
}

int cache_invalidate(Cache_t* cache, uint16_t address) {
    if (!cache) return 0;
    uint16_t cache_address = cache_resolve_address(cache, address);
    cache->state[cache_address].valid = 0;
    return 1;
}

int cache_invalidate_all(Cache_t* cache) {
    if (!cache) return 0;
    for (int i = 0; i < cache->capacity; i++) {
        cache->state[i].valid = 0;
    }
    return 1;
}

// returns 1 if the cache has a value associated with the given address (dirty or not)
int cache_contains_address(Cache_t* cache, uint16_t address) {
    if (!cache) return 0;
    uint16_t cache_address = cache_resolve_address(cache, address);
    if (cache->address[cache_address] != address) return 0;
    return cache->state[cache_address].valid;
}

uint16_t cache_get_occupied_address(Cache_t* cache, uint16_t address) {
    if (!cache) return 0;
    uint16_t cache_address = cache_resolve_address(cache, address);
    return cache->address[cache_address];
}


/*

OP; in cache; NC; evict

READ; 0; 0; 0 -> read from ram, fill cache
READ; 1; 0; 0 -> read from cache
READ; 0; 1; 0 -> read from ram, DONT fill cache
READ; 1; 1; 0 -> read from ram, DONT fill cache, invalidate read byte in cache


*/
