#ifndef _CPU_ADDRESSING_MODES_H_
#define _CPU_ADDRESSING_MODES_H_

#include <stdint.h>

// 5 bits [src]
typedef enum CPU_EXTENDED_ADDRESSING_MODE_t {
    // No Addressing
    ADMX_NONE, 

    // Immediate Values
    ADMX_IMM16, 

    // Register Direct Addressing
    ADMX_R0, ADMX_R1, ADMX_R2, ADMX_R3, ADMX_SP, ADMX_PC, 

    // Indirect Immediate Addressing
    ADMX_IND16, 
    
    // Indirect Register Addressing
    ADMX_IND_R0, ADMX_IND_R1, ADMX_IND_R2, ADMX_IND_R3, ADMX_IND_SP, ADMX_IND_PC, 

    // Indirect Addressing with 16-bit Offset
    ADMX_IND_R0_OFFSET16, ADMX_IND_R1_OFFSET16, ADMX_IND_R2_OFFSET16, ADMX_IND_R3_OFFSET16, 
    ADMX_IND_SP_OFFSET16, ADMX_IND_PC_OFFSET16, 

    // Indirect Addressing with 16-bit Base and Scaled Register Offset
    ADMX_IND16_SCALED8_R0_OFFSET, ADMX_IND16_SCALED8_R1_OFFSET, ADMX_IND16_SCALED8_R2_OFFSET, ADMX_IND16_SCALED8_R3_OFFSET, 
    ADMX_IND16_SCALED8_SP_OFFSET, ADMX_IND16_SCALED8_PC_OFFSET, 

    ADMX_ADDRESSING_MODE_COUNT // Keeps track of the number of addressing modes
} CPU_EXTENDED_ADDRESSING_MODE_t;


// 3 bits [dest] [should never be immedate]
typedef enum CPU_REDUCED_ADDRESSING_MODE_t {
    ADMR_NONE, 
    // Register Direct Addressing
    ADMR_R0, ADMR_R1, ADMR_R2, ADMR_R3, ADMR_SP, 
    // Indirect Immediate Addressing
    ADMR_IND16, 
    // Indirect Register Addressing
    ADMR_IND_R0, 

    ADMR_ADDRESSING_MODE_COUNT // Keeps track of the number of addressing modes
} CPU_REDUCED_ADDRESSING_MODE_t;

typedef enum CPU_ADDRESSING_MODE_CATEGORY_t {
    ADMC_NONE, 
    ADMC_IMM, 
    ADMC_REG, 
    ADMC_IND
} CPU_ADDRESSING_MODE_CATEGORY_t;

extern const char* cpu_extended_addressing_mode_string[32];

extern const char* cpu_reduced_addressing_mode_string[8];

extern const int cpu_extended_addressing_mode_bytes[32];

extern const int cpu_reduced_addressing_mode_bytes[8];

extern const CPU_ADDRESSING_MODE_CATEGORY_t cpu_extended_addressing_mode_category[32];

extern const CPU_ADDRESSING_MODE_CATEGORY_t cpu_reduced_addressing_mode_category[8];


#endif
