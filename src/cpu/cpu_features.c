#include "cpu/cpu_features.h"

const char* cpu_feature_flag_name[16] = {
    "CFF_BASE", 
    "CFF_INT_ARITH", 
    "CFF_INT_SIGNED_SAT_ARITH", 
    "CFF_INT_UNSIGNED_SAT_ARITH", 
    "CFF_INT_ARITH_EXT", 
    "CFF_INT_CARRY_EXT", 
    "CFF_LOGIC_EXT", 
    "CFF_CMOV_EXT", 
    "CFF_BYTE_EXT", 
    "CFF_FLOAT16", 
    "CFF_BFLOAT16", 
    "CFF_FLOAT_CONVERT", 
    "CFF_CACHE_EXT", 
    "CFF_HW_INFO", 
};
