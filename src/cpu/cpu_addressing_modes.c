#include "cpu/cpu_addressing_modes.h"

const char* cpu_extended_addressing_mode_string[32] = {
    // No Addressing
    "NONE",

    // Immediate Values
    "IMM16",

    // Register Direct Addressing
    "R0", "R1", "R2", "R3", "SP", "PC",

    // Indirect Addressing
    "IND16", "IND_R0", "IND_R1", "IND_R2", "IND_R3", "IND_SP", "IND_PC",

    // Indirect Addressing with 16-bit Offset
    "IND_R0_OFFSET16", "IND_R1_OFFSET16", "IND_R2_OFFSET16", "IND_R3_OFFSET16",
    "IND_SP_OFFSET16", "IND_PC_OFFSET16",

    // Indirect Addressing with 16-bit Base and Scaled Register Offset
    "IND16_SCALED8_R0_OFFSET", "IND16_SCALED8_R1_OFFSET", "IND16_SCALED8_R2_OFFSET", "IND16_SCALED8_R3_OFFSET",
    "IND16_SCALED8_SP_OFFSET", "IND16_SCALED8_PC_OFFSET", 
    
    // Currently unused
    "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", 
};


const char* cpu_reduced_addressing_mode_string[8] = {
    "NONE", 

    // Register Direct Addressing
    "R0", "R1", "R2", "R3", "SP",

    // Indirect Addressing
    "IND16", 
    
    // Indirect Register Addressing
    "IND_R0",
};

const int cpu_extended_addressing_mode_bytes[32] = {
    0, 
    2, 
    0, 0, 0, 0, 0, 0, 
    2, 0, 0, 0, 0, 0, 0, 
    2, 2, 2, 2, 2, 2, 
    3, 3, 3, 3, 3, 3, 

    0, 0, 0, 0, 0, 
};

const int cpu_reduced_addressing_mode_bytes[8] = {
    0, 
    0, 0, 0, 0, 0, 
    2, 
    0, 
};

const CPU_ADDRESSING_MODE_CATEGORY_t cpu_extended_addressing_mode_category[32] = {
    ADMC_NONE, 

    ADMC_IMM, 

    ADMC_REG, ADMC_REG, ADMC_REG, ADMC_REG, ADMC_REG, ADMC_REG, 

    ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, 

    ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, 
    ADMC_IND, ADMC_IND, 

    ADMC_IND, ADMC_IND, ADMC_IND, ADMC_IND, 
    ADMC_IND, ADMC_IND, 

    ADMC_NONE, ADMC_NONE, ADMC_NONE, ADMC_NONE,  ADMC_NONE, 
};

const CPU_ADDRESSING_MODE_CATEGORY_t cpu_reduced_addressing_mode_category[8] = {
    ADMC_NONE, 

    ADMC_REG, ADMC_REG, ADMC_REG, ADMC_REG, ADMC_REG, 

    ADMC_IND, 
    
    ADMC_IND,
};
