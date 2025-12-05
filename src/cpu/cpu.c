#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/ExtendedTypes.h"

#include "cache.h"
#include "device.h"

#include "cpu/cpu_instructions.h"
#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu.h"


#define _CPU_DEBUG_
#undef _CPU_DEBUG_

static int halted = 0;


const char* cpu_state_name[CS_COUNT] = {
    "CS_FETCH_INSTRUCTION", 
    "CS_FETCH_ADDRESSING_MODES", 
    "CS_FETCH_ARGUMENT_BYTES", 
    "CS_COMPUTE_ADDRESS",    // computes the absolute address from the addressing mode and arguments
    "CS_FETCH_SOURCE",                // fetches data from the indirect source addressing
    "CS_FETCH_SOURCE_HIGH", 
    "CS_FETCH_DESTINATION",           // fetches data from the indirect destination addressing
    "CS_FETCH_DESTINATION_HIGH", 
    "CS_EXECUTE",                     // processes instruction and saves intermediate result for possible writeback
    "CS_WRITEBACK_LOW",                   // writes result back to memory
    "CS_WRITEBACK_HIGH",                   // writes result back to memory
    "CS_PUSH_LOW",                    // push and pop are unique, thats why
    "CS_PUSH_HIGH", 
    "CS_POP_LOW", 
    "CS_POP_HIGH", 
    "CS_POPSR_LOW", 
    "CS_POPSR_HIGH", 
    "CS_RET_LOW", 
    "CS_RET_HIGH", 

    "CS_INTERRUPT_PUSH_PC_HIGH", 
    "CS_INTERRUPT_PUSH_PC_LOW", 
    "CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW", 
    "CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH", 
    "CS_INTERRUPT_JUMP", 

    "CS_HALT",                    // halts all execution indefinitely

    "CS_EXCEPTION", 
};


CPU_t* cpu_create(void) {
    CPU_t* cpu = malloc(sizeof(CPU_t));
    if (!cpu) return NULL;  // Always check for malloc failure

    cpu->device = device_create(DT_CPU);

    // Zero out the entire CPU structure
    memset(cpu, 0, sizeof(CPU_t));

    // Explicitly reset the status register
    cpu->regs.sr.value = 0x0000;

    cpu->clock = 0ULL;

    cpu->memory_layout = (CpuMemoryLayout_t) {
        .segment_stack = 0x3FFF, 
        .segment_irq_table = 0xEF00, // currently space for 128 interrupt vectors
    };

    //cpu->regs.pc = cpu->memory_layout.segment_code;
    cpu->regs.sp = cpu->memory_layout.segment_stack + 1;        // sp always writes to the byte below, so we need to start one byte higher

    cpu->state = CS_FETCH_INSTRUCTION;

    cpu->feature_flag = 0;
    #ifdef DCFF_CORE_BASE
        cpu->feature_flag |= CFF_BASE;
    #endif
    #ifdef DCFF_INT_ARITH
        cpu->feature_flag |= CFF_INT_ARITH;
    #endif
    #ifdef DCFF_INT_SIGNED_SAT_ARITH
        cpu->feature_flag |= CFF_INT_SIGNED_SAT_ARITH;
    #endif
    #ifdef DCFF_INT_UNSIGNED_SAT_ARITH
        cpu->feature_flag |= CFF_INT_UNSIGNED_SAT_ARITH;
    #endif
    #ifdef DCFF_INT_ARITH_EXT
        cpu->feature_flag |= CFF_INT_ARITH_EXT;
    #endif
    #ifdef DCFF_INT_CARRY_EXT
        cpu->feature_flag |= CFF_INT_CARRY_EXT;
    #endif
    #ifdef DCFF_LOGIC_EXT
        cpu->feature_flag |= CFF_LOGIC_EXT;
    #endif
    #ifdef DCFF_CMOV_EXT
        cpu->feature_flag |= CFF_CMOV_EXT;
    #endif
    #ifdef DCFF_BYTE_EXT
        cpu->feature_flag |= CFF_BYTE_EXT;
    #endif
    #ifdef DCFF_FLOAT16
        cpu->feature_flag |= CFF_FLOAT16;
    #endif
    #ifdef DCFF_BFLOAT16
        cpu->feature_flag |= CFF_BFLOAT16;
    #endif
    #ifdef DCFF_FLOAT_CONVERT
        cpu->feature_flag |= CFF_FLOAT_CONVERT;
    #endif
    #ifdef DCFF_CACHE_EXT
        cpu->feature_flag |= CFF_CACHE_EXT;
    #endif
    #ifdef DCFF_HW_INFO
        cpu->feature_flag |= CFF_HW_INFO;
    #endif

    return cpu;
}


void cpu_delete(CPU_t* cpu) {
    cache_delete(cpu->cache);
    free(cpu);
}

void cpu_mount_cache(CPU_t* cpu, Cache_t* cache) {
    cpu->cache = cache;
}


/* 
Returns 1 if the data has been successfully fetched, else 0. The result will be put in the data pointer
First it looks through cache, if its not there, it sends a request to ram
*/
int cpu_read_memory(CPU_t* cpu, uint16_t address, uint8_t *data) {
    //log_msg(LP_DEBUG, "CPU %d: Attempting to request memory at address 0x%.4x", cpu->clock, address);
    
    //log_msg(LP_DEBUG, "CPU %d: Checking cache first", cpu->clock);
    int success = cache_read(cpu->cache, address, data, cpu->regs.sr.SRC);
    if (success) return 1;
    //log_msg(LP_DEBUG, "CPU %d: \tCache miss", cpu->clock);

    //log_msg(LP_DEBUG, "CPU %d: Checking device response", cpu->clock);
    if (cpu->device.processed) {
        if (cpu->device.address != address) {
            //log_msg(LP_ERROR, "CPU %d: \tThe device responded with the wrong address. Making new request", cpu->clock);
            cpu->device.address = address;
            cpu->device.processed = 0;
            cpu->device.device_state = DS_FETCH;
            return 0;
        }
        //log_msg(LP_DEBUG, "CPU %d: \tDevice responded", cpu->clock);
        uint64_t response = cpu->device.data;
        cpu->device.processed = 0;
        cpu->device.device_state = DS_IDLE;

        //log_msg(LP_DEBUG, "CPU %d: Requesting write to cache", cpu->clock);
        cache_write(cpu->cache, cpu->device.address, (uint8_t*) &response, sizeof(response), cpu->regs.sr.SWC);

        *data = (uint8_t) response;
        //log_msg(LP_DEBUG, "CPU %d: Data %.2x from Address %.4x found in reply in device", cpu->clock, *data, cpu->device.address);
        return 1;
    }

    //log_msg(LP_DEBUG, "CPU %d: \tNo device response. Making request", cpu->clock);
    if (cpu->device.device_state != DS_IDLE) {
        //log_msg(LP_DEBUG, "CPU %d: \t\tDevice is not idle, cannot make request", cpu->clock);
        return 0;
    }
    cpu->device.address = address;
    cpu->device.processed = 0;
    cpu->device.device_state = DS_FETCH;

    return 0;
}


int cpu_write_memory(CPU_t* cpu, uint16_t address, uint8_t data) {
    //log_msg(LP_DEBUG, "CPU %d: Attempting to write data 0x%.2x at address 0x%.4x", cpu->clock, data, address);

    if (cpu->device.processed) {
        //log_msg(LP_DEBUG, "CPU %d: Update went through", cpu->clock);
        cpu->device.processed = 0;
        cpu->device.device_state = DS_IDLE;
        
        return 1;
    }

    //log_msg(LP_DEBUG, "CPU %d: \tNo device response. Making request", cpu->clock);
    if (cpu->device.device_state != DS_IDLE) {
        //log_msg(LP_DEBUG, "CPU %d: \t\tDevice is not idle, cannot make request", cpu->clock);
        return 0;
    }
    cpu->device.address = address;
    cpu->device.data = data;
    cpu->device.processed = 0;
    cpu->device.device_state = DS_STORE;

    int accept_dirty_write = cache_write(cpu->cache, cpu->device.address, (uint8_t*) &data, 1, cpu->regs.sr.SWC);
    return accept_dirty_write;
}


void cpu_update_status_register(CPU_t* cpu, uint16_t result) {
    cpu->regs.sr.Z = (result == 0);
    cpu->regs.sr.L = (result >> 15); // ((int16_t) result < 0)
    cpu->regs.sr.UL = 0;
    cpu->regs.sr.FL = (result >> 15);
    cpu->regs.sr.BL = (result >> 15);
}

void cpu_clock(CPU_t* cpu) {
    //log_msg(LP_DEBUG, "CS %d, DS %d", cpu->state, cpu->device.device_state);

    //log_msg(LP_INFO, "CPU %d: Checking for interrupt", cpu->clock);
    if (cpu->device.device_state == DS_INTERRUPT) {
        #ifdef _CPU_DEBUG_
        log_msg(LP_INFO, "CPU %lld/%lld: PC %.4x - interrupt on CS %d", cpu->clock, cpu->instruction, cpu->regs.pc, cpu->state);
        #endif
        
        if (!cpu->regs.sr.MI && !cpu->regs.sr.MNI) {
            cpu->regs.pc = cpu->intermediate.previous_pc;
            cpu->regs.sp = cpu->intermediate.previous_sp;
            cpu->state = CS_INTERRUPT_PUSH_PC_HIGH;
            cpu->intermediate.irq_id = cpu->device.address;
        }
        cpu->device.device_state = DS_IDLE;
    }

    switch (cpu->state) {

        case CS_FETCH_INSTRUCTION:
            //log_msg(LP_INFO, "CPU %d: Fetching instruction", cpu->clock);
            {
                CS_FETCH_INSTRUCTION:
                cpu->regs.sr.MNI = 0;
                uint16_t address = cpu->regs.pc;
                cpu->intermediate.previous_pc = cpu->regs.pc;
                cpu->intermediate.previous_sp = cpu->regs.sp;
                uint8_t data;
                int success = cpu_read_memory(cpu, address, &data);
                if (success) {
                    // ok, we got the data for the instruction, saving it intermediatly
                    cpu->intermediate.instruction = data;
                    //log_msg(LP_INFO, "CPU %d: Fetch was successful, loaded instruction %s", cpu->clock, cpu_instruction_string[data]);
                    cpu->intermediate.argument_count = cpu_instruction_argument_count[cpu->intermediate.instruction];

                    if (cpu->intermediate.instruction < INSTRUCTION_COUNT) {
                        cpu->last_instruction = (CPU_INSTRUCTION_MNEMONIC_t) cpu->intermediate.instruction;
                        #ifdef _CPU_DEBUG_
                        if (cpu->intermediate.argument_count == 0) {
                            log_msg(LP_DEBUG, "CPU %lld/%lld: PC %.4x - instruction: %s - sp: %.4x", 
                                cpu->clock, 
                                cpu->instruction, 
                                cpu->regs.pc, 
                                cpu_instruction_string[cpu->intermediate.instruction], 
                                cpu->regs.sp
                            );
                        }
                        #endif
                    } else {
                        //log_msg(LP_ERROR, "CPU %d: Unknown instruction [%d, PC: 0x%.4x]", cpu->clock, cpu->intermediate.instruction, cpu->regs.pc - 1);
                        cpu->state = CS_EXCEPTION;
                        break;
                    }

                    if (cpu->intermediate.argument_count == 0) {
                        cpu->regs.pc ++;
                        cpu->state = CS_EXECUTE;
                        cpu->regs.pc ++;
                        goto CS_EXECUTE;
                    } else {
                        cpu->state = CS_FETCH_ADDRESSING_MODES;
                        cpu->regs.pc ++;
                        goto CS_FETCH_ADDRESSING_MODES;
                    }
                } else {
                    //log_msg(LP_INFO, "CPU %d: Fetch was unsuccessful", cpu->clock);
                }
            }
            break;
        
        case CS_FETCH_ADDRESSING_MODES:
            //log_msg(LP_INFO, "CPU %d: Fetching addressing modes", cpu->clock);
            {
                CS_FETCH_ADDRESSING_MODES:
                cpu->regs.sr.MNI = 0;
                uint16_t address = cpu->regs.pc;
                uint8_t data;
                int success = cpu_read_memory(cpu, address, &data);
                if (success) {
                    //log_msg(LP_INFO, "CPU %d: Fetch was successful", cpu->clock);
                    // ok, we got the data for the instruction, saving it intermediatly
                    cpu->intermediate.addressing_mode.value = data;

                    if (cpu->intermediate.argument_count == 2) {
                        #ifdef _CPU_DEBUG_
                            log_msg(LP_DEBUG, "CPU %lld/%lld: PC %.4x - instruction: %s %s, %s - sp: %.4x", 
                                cpu->clock, 
                                cpu->instruction, 
                                cpu->regs.pc - 1, 
                                cpu_instruction_string[cpu->intermediate.instruction], 
                                cpu_reduced_addressing_mode_string[cpu->intermediate.addressing_mode.addressing_mode_reduced], 
                                cpu_extended_addressing_mode_string[cpu->intermediate.addressing_mode.addressing_mode_extended], 
                                cpu->regs.sp
                            );
                        #endif
                        if (cpu->intermediate.addressing_mode.addressing_mode_reduced == ADMR_NONE || cpu->intermediate.addressing_mode.addressing_mode_extended == ADMX_NONE) {
                            //log_msg(LP_ERROR, "CPU %d: Encountered an invalid instruction", cpu->clock);
                            cpu->state = CS_EXCEPTION;
                        }
                    } else {
                        if (cpu_instruction_single_operand_writeback[cpu->intermediate.instruction]) {
                            #ifdef _CPU_DEBUG_
                                log_msg(LP_DEBUG, "CPU %lld/%lld: PC %.4x - instruction: %s %s - sp: %.4x", 
                                    cpu->clock, 
                                    cpu->instruction, 
                                    cpu->regs.pc - 1, 
                                    cpu_instruction_string[cpu->intermediate.instruction], 
                                    cpu_reduced_addressing_mode_string[cpu->intermediate.addressing_mode.addressing_mode_reduced], 
                                    cpu->regs.sp
                                );
                            #endif
                            if (cpu->intermediate.addressing_mode.addressing_mode_reduced == ADMR_NONE) {
                                //log_msg(LP_ERROR, "CPU %d: Encountered an invalid instruction", cpu->clock);
                                cpu->state = CS_EXCEPTION;
                            }
                        } else {
                            #ifdef _CPU_DEBUG_
                                log_msg(LP_DEBUG, "CPU %lld/%lld: PC %.4x - instruction: %s %s - sp: %.4x", 
                                    cpu->clock, 
                                    cpu->instruction, 
                                    cpu->regs.pc - 1, 
                                    cpu_instruction_string[cpu->intermediate.instruction], 
                                    cpu_extended_addressing_mode_string[cpu->intermediate.addressing_mode.addressing_mode_extended], 
                                    cpu->regs.sp
                                );
                            #endif
                            if (cpu->intermediate.addressing_mode.addressing_mode_extended == ADMX_NONE) {
                                //log_msg(LP_ERROR, "CPU %d: Encountered an invalid instruction", cpu->clock);
                                cpu->state = CS_EXCEPTION;
                            }
                        }
                    }
                    
                    // here check how many bytes to load for 
                    cpu->intermediate.argument_bytes_to_load = 0;
                    if (cpu->intermediate.argument_count == 2) {
                        cpu->intermediate.argument_bytes_to_load = 
                            cpu_reduced_addressing_mode_bytes[cpu->intermediate.addressing_mode.addressing_mode_reduced] +
                            cpu_extended_addressing_mode_bytes[cpu->intermediate.addressing_mode.addressing_mode_extended];
                    } else if (cpu->intermediate.argument_count == 1) {
                        cpu->intermediate.argument_bytes_to_load = cpu_extended_addressing_mode_bytes[cpu->intermediate.addressing_mode.addressing_mode_extended];
                    }

                    cpu->regs.pc ++;

                    if (cpu->intermediate.argument_bytes_to_load == 0) {
                        //log_msg(LP_INFO, "CPU %d: No bytes to fetch, skipping to CS_COMPUTE_ADDRESS");
                        cpu->state = CS_COMPUTE_ADDRESS;
                        goto CS_COMPUTE_ADDRESS;
                        break;
                    }

                    //log_msg(LP_INFO, "CPU %d: Addressing modes require %d bytes for arguments", cpu->clock, cpu->intermediate.argument_bytes_to_load);
                    
                    cpu->intermediate.argument_data_raw_index = 0;
                    cpu->state = CS_FETCH_ARGUMENT_BYTES;
                    goto CS_FETCH_ARGUMENT_BYTES;
                } else {
                    //log_msg(LP_INFO, "CPU %d: Fetch was unsuccessful", cpu->clock);
                }
            }
            break;
        
        case CS_FETCH_ARGUMENT_BYTES: 
            //log_msg(LP_INFO, "CPU %d: Fetching all argument bytes", cpu->clock);
            {
                CS_FETCH_ARGUMENT_BYTES:
                cpu->regs.sr.MNI = 0;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->regs.pc, &data);
                if (success) {
                    //log_msg(LP_INFO, "CPU %d: Fetched byte %d/%d [0x%.2x]", cpu->clock, cpu->intermediate.argument_data_raw_index + 1, cpu->intermediate.argument_bytes_to_load, data);
                    cpu->intermediate.argument_data_raw[cpu->intermediate.argument_data_raw_index] = data;
                    cpu->intermediate.argument_data_raw_index ++;
                    cpu->regs.pc ++;

                    if (cpu->intermediate.argument_data_raw_index >= cpu->intermediate.argument_bytes_to_load) {
                        /*//log_msg(LP_INFO, "CPU %d: Done reading all bytes", cpu->clock);
                        for (int i = 0; i < cpu->intermediate.argument_bytes_to_load; i++) {
                            printf("%.2x ", (uint8_t) cpu->intermediate.argument_data_raw[i]);
                        } printf("\n");*/
                        
                        // if its only for admx, then skip to fetch source
                        int admxc = cpu_extended_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_extended];
                        switch (admxc) {
                            case ADMC_IND:
                                cpu->state = CS_COMPUTE_ADDRESS;
                                goto CS_COMPUTE_ADDRESS;
                                break;
                            case ADMC_IMM:
                                //log_msg(LP_INFO, "CPU %d: admc is not indirect, skipping CS_COMPUTE_ADDRESS", cpu->clock);
                                cpu->state = CS_COMPUTE_ADDRESS;//CS_FETCH_SOURCE;
                                goto CS_COMPUTE_ADDRESS;
                                break;
                            case ADMC_REG:
                                cpu->state = CS_COMPUTE_ADDRESS;//CS_FETCH_SOURCE;
                                goto CS_COMPUTE_ADDRESS;
                                break;
                            default:
                                //log_msg(LP_ERROR, "CPU %d: Unkown extended adm category %d", cpu->clock, admxc);
                                break;
                        }
                        break;
                    }
                }
            }
            break;
        
        case CS_COMPUTE_ADDRESS: // IMPORTANT: this could very well be done in CS_DECODE_ADDRESSING_MODES saving one clock cycle
            //log_msg(LP_INFO, "CPU %d: Calculating source address from argument composition", cpu->clock);
            {
                CS_COMPUTE_ADDRESS:
                cpu->regs.sr.MNI = 0;
                int admr = cpu->intermediate.addressing_mode.addressing_mode_reduced;
                int admx = cpu->intermediate.addressing_mode.addressing_mode_extended;
                int argument_index = 0;
                //log_msg(LP_INFO, "CPU %d: admx %s", cpu->clock, cpu_extended_addressing_mode_string[admx]);
                cpu->intermediate.argument_count_for_admr = argument_index;
                switch (admx) {
                    case ADMX_R0:
                    case ADMX_R1:
                    case ADMX_R2:
                    case ADMX_R3:
                    case ADMX_SP:
                    case ADMX_PC:
                        // Idea, here set flag to not fetch data from this "address"
                        //log_msg(LP_DEBUG, "CPU %d: admx is not an address", cpu->clock, cpu->intermediate.argument_address_reduced);
                        cpu->state = CS_FETCH_SOURCE;
                        break;
                    case ADMX_IMM16:
                        argument_index += 2;
                        cpu->state = CS_FETCH_SOURCE;
                        break;
                    
                    case ADMX_IND_R0:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.r0;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R1:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.r1;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R2:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.r2;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R3:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.r3;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_SP:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.sp;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended); 
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_PC:
                        {
                            cpu->intermediate.argument_address_extended = cpu->regs.pc;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended); 
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            //log_msg(LP_DEBUG, "CPU %d: admx base address [%.4x]", cpu->clock, cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R0_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.r0);
                            cpu->intermediate.argument_address_extended += cpu->regs.r0;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R1_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.r1);
                            cpu->intermediate.argument_address_extended += cpu->regs.r1;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R2_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.r2);
                            cpu->intermediate.argument_address_extended += cpu->regs.r2;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_R3_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.r3);
                            cpu->intermediate.argument_address_extended += cpu->regs.r3;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_SP_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.sp);
                            cpu->intermediate.argument_address_extended += cpu->regs.sp;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;
                    
                    case ADMX_IND_PC_OFFSET16:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, cpu->regs.pc);
                            cpu->intermediate.argument_address_extended += cpu->regs.pc;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_R0_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.r0);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.r0;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_R1_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.r1);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.r1;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_R2_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.r2);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.r2;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_R3_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.r3);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.r3;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_SP_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.sp);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.sp;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_IND16_SCALED8_PC_OFFSET:
                        {
                            cpu->intermediate.argument_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                            argument_index ++;
                            cpu->intermediate.argument_address_extended |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                            argument_index ++;
                            uint8_t scaler = cpu->intermediate.argument_data_raw[argument_index];
                            ////log_msg_inline(LP_DEBUG, "CPU %d: admx base address [%.4x + %.2x * %.4x] = ", cpu->clock, cpu->intermediate.argument_address_extended, scaler, cpu->regs.pc);
                            cpu->intermediate.argument_address_extended += scaler * cpu->regs.pc;
                            //printf("[%.4x]\n", cpu->intermediate.argument_address_extended);
                            cpu->state = CS_FETCH_SOURCE;
                        }
                        break;

                    case ADMX_NONE:
                        //log_msg(LP_DEBUG, "CPU %d: no admx address specified", cpu->clock);
                        cpu->state = CS_FETCH_DESTINATION;
                        break;

                    default:
                        //log_msg(LP_ERROR, "CPU %d: unknown extended addressing mode", cpu->clock);
                        break;
                }
                // We cant switch the oder of admr and admx to add goto skip-aheads, because of argument_index
                //log_msg(LP_INFO, "CPU %d: admr %s", cpu->clock, cpu_reduced_addressing_mode_string[admr]);
                switch (admr) {
                    case ADMR_R0:
                    case ADMR_R1:
                    case ADMR_R2:
                    case ADMR_R3:
                    case ADMR_SP:
                        //log_msg(LP_DEBUG, "CPU %d: admr is not an address", cpu->clock, cpu->intermediate.argument_address_reduced);
                        break;

                    case ADMR_IND16:
                        cpu->intermediate.argument_address_reduced = (uint8_t) cpu->intermediate.argument_data_raw[argument_index];
                        argument_index ++;
                        cpu->intermediate.argument_address_reduced |= (cpu->intermediate.argument_data_raw[argument_index] << 8);
                        //argument_index ++;
                        //log_msg(LP_DEBUG, "CPU %d: admr address [%.4x]", cpu->clock, cpu->intermediate.argument_address_reduced);
                        break;

                    case ADMR_IND_R0:
                        cpu->intermediate.argument_address_reduced = cpu->regs.r0;
                        //log_msg(LP_DEBUG, "CPU %d: admr address [%.4x]", cpu->clock, cpu->intermediate.argument_address_reduced);
                        break;

                    case ADMR_NONE:
                        //log_msg(LP_DEBUG, "CPU %d: no admx address specified", cpu->clock);
                        //cpu->state = CS_FETCH_DESTINATION;
                        break;

                    default:
                        //log_msg(LP_ERROR, "CPU %d: unknown reduced addressing mode", cpu->clock);
                        break;
                }
            }
            break;

        case CS_FETCH_SOURCE: // only fetch data from source, nothing else
            //log_msg(LP_INFO, "CPU %d: Fetching source (low) from adm address", cpu->clock);
            {
                cpu->regs.sr.MNI = 0;
                int admxc = cpu_extended_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_extended];
                switch (admxc) {
                    case ADMC_IMM:
                        {
                            int admx = cpu->intermediate.addressing_mode.addressing_mode_extended;
                            switch (admx) {
                                case ADMX_IMM16:
                                    //log_msg(LP_DEBUG, "CPU %d: IMM16", cpu->clock);
                                    cpu->intermediate.data_address_extended = (uint8_t) cpu->intermediate.argument_data_raw[cpu->intermediate.argument_count_for_admr];
                                    cpu->intermediate.data_address_extended |= (cpu->intermediate.argument_data_raw[cpu->intermediate.argument_count_for_admr + 1] << 8);
                                    break;
                                default:
                                    //log_msg(LP_ERROR, "CPU %d: unknown immediate extended addressing mode", cpu->clock);
                                    break;
                            }
                            if (cpu->intermediate.argument_count == 2) {
                                cpu->state = CS_FETCH_DESTINATION;
                                goto CS_FETCH_DESTINATION;
                            } else {
                                //log_msg(LP_DEBUG, "CPU %d: Skipping destination fetching, cause only one argument", cpu->clock);
                                cpu->state = CS_EXECUTE;
                                goto CS_EXECUTE;
                            }
                            
                        }
                        break;
                    case ADMC_REG:
                        {
                            int admx = cpu->intermediate.addressing_mode.addressing_mode_extended;
                            switch (admx) {
                                case ADMX_R0:
                                    //log_msg(LP_DEBUG, "CPU %d: r0", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->regs.r0;
                                    break;
                                case ADMX_R1:
                                    //log_msg(LP_DEBUG, "CPU %d: r1", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->regs.r1;
                                    break;
                                case ADMX_R2:
                                    //log_msg(LP_DEBUG, "CPU %d: r2", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->regs.r2;
                                    break;
                                case ADMX_R3:
                                    //log_msg(LP_DEBUG, "CPU %d: r3", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->regs.r3;
                                    break;
                                case ADMX_SP:
                                    //log_msg(LP_DEBUG, "CPU %d: sp", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->regs.sp;
                                    break;
                                case ADMX_PC:
                                    //log_msg(LP_DEBUG, "CPU %d: pc", cpu->clock);
                                    cpu->intermediate.data_address_extended = cpu->intermediate.previous_pc;
                                    break;
                                    
                                default:
                                    //log_msg(LP_ERROR, "CPU %d: unknown register extended addressing mode", cpu->clock);
                                    break;
                            }
                            if (cpu->intermediate.argument_count == 2) {
                                cpu->state = CS_FETCH_DESTINATION;
                                goto CS_FETCH_DESTINATION;
                            } else {
                                //log_msg(LP_DEBUG, "CPU %d: Skipping destination fetching, cause only one argument", cpu->clock);
                                cpu->state = CS_EXECUTE;
                                goto CS_EXECUTE;
                            }
                        }
                        break;
                    case ADMC_IND:
                        {
                            uint8_t data;
                            int success = cpu_read_memory(cpu, cpu->intermediate.argument_address_extended, &data);
                            if (success) {
                                cpu->intermediate.data_address_extended = (uint8_t) data;
                                cpu->state = CS_FETCH_SOURCE_HIGH;
                                goto CS_FETCH_SOURCE_HIGH;
                            }
                            break;
                        }
                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown indirect extended addressing mode category %d", cpu->clock, admxc);
                        break;
                }
            }
            //log_msg(LP_INFO, "CPU %d: Intermediate data_address_extended %.4x", cpu->clock, cpu->intermediate.data_address_extended);
            break;
        
        case CS_FETCH_SOURCE_HIGH: // only fetch data from source, nothing else
            //log_msg(LP_INFO, "CPU %d: Fetching source high from adm address", cpu->clock);
            {
                CS_FETCH_SOURCE_HIGH:
                cpu->regs.sr.MNI = 0;
                int admc = cpu_extended_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_extended];
                switch (admc) {
                    case ADMC_IND:
                        {
                            uint8_t data;
                            int success = cpu_read_memory(cpu, cpu->intermediate.argument_address_extended + 1, &data);
                            if (success) {
                                cpu->intermediate.data_address_extended |= (uint16_t)(data << 8);
                                
                                if (cpu->intermediate.argument_count == 2) {
                                    cpu->state = CS_FETCH_DESTINATION;
                                    goto CS_FETCH_DESTINATION;
                                } else {
                                    //log_msg(LP_DEBUG, "CPU %d: Skipping destination fetching, cause only one argument", cpu->clock);
                                    cpu->state = CS_EXECUTE;
                                    goto CS_EXECUTE;
                                }
                            }
                            break;
                        }
                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown indirect extended addressing mode category %d", cpu->clock, admc);
                        break;
                }
            }
            //log_msg(LP_INFO, "CPU %d: Intermediate data_address_extended %.4x", cpu->clock, cpu->intermediate.data_address_extended);
            break;
        
        case CS_FETCH_DESTINATION: // only fetch data from destination, nothing else
            //log_msg(LP_INFO, "CPU %d: Fetching destination from adm address", cpu->clock);
            {
                CS_FETCH_DESTINATION:
                cpu->regs.sr.MNI = 0;
                int admc = cpu_reduced_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_reduced];
                switch (admc) {
                    /*case ADMC_IMM:
                        {
                            int admr = cpu->intermediate.addressing_mode.addressing_mode_reduced;
                            switch (admr) {
                                case ADMR_IMM16:
                                    //log_msg(LP_DEBUG, "CPU %d: IMM16", cpu->clock);
                                    cpu->intermediate.data_address_reduced = (uint8_t) cpu->intermediate.argument_data_raw[cpu->intermediate.argument_count_for_admr];
                                    cpu->intermediate.data_address_reduced |= (cpu->intermediate.argument_data_raw[cpu->intermediate.argument_count_for_admr + 1] << 8);
                                    break;
                                default:
                                    //log_msg(LP_ERROR, "CPU %d: unknown immediate extended addressing mode", cpu->clock);
                                    break;
                            }
                            cpu->state = CS_EXECUTE;
                        }
                        break;*/
                    
                    case ADMC_REG:
                        {
                            int admr = cpu->intermediate.addressing_mode.addressing_mode_reduced;
                            switch (admr) {
                                case ADMR_R0:
                                    //log_msg(LP_DEBUG, "CPU %d: r0", cpu->clock);
                                    cpu->intermediate.data_address_reduced = cpu->regs.r0;
                                    break;
                                case ADMR_R1:
                                    //log_msg(LP_DEBUG, "CPU %d: r1", cpu->clock);
                                    cpu->intermediate.data_address_reduced = cpu->regs.r1;
                                    break;
                                case ADMR_R2:
                                    //log_msg(LP_DEBUG, "CPU %d: r2", cpu->clock);
                                    cpu->intermediate.data_address_reduced = cpu->regs.r2;
                                    break;
                                case ADMR_R3:
                                    //log_msg(LP_DEBUG, "CPU %d: r3", cpu->clock);
                                    cpu->intermediate.data_address_reduced = cpu->regs.r3;
                                    break;
                                case ADMR_SP:
                                    //log_msg(LP_DEBUG, "CPU %d: sp", cpu->clock);
                                    cpu->intermediate.data_address_reduced = cpu->regs.sp;
                                    break;

                                default:
                                    //log_msg(LP_ERROR, "CPU %d: unknown register extended addressing mode", cpu->clock);
                                    break;
                            }
                            cpu->state = CS_EXECUTE;
                            goto CS_EXECUTE;
                        }
                        break;
                    case ADMC_IND:
                        {
                            uint8_t data;
                            int success = cpu_read_memory(cpu, cpu->intermediate.argument_address_reduced, &data);
                            if (success) {
                                cpu->intermediate.data_address_reduced = (uint8_t) data;
                                cpu->state = CS_FETCH_DESTINATION_HIGH;
                                goto CS_FETCH_DESTINATION_HIGH;
                            }
                            break;
                        }
                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown indirect extended addressing mode category %d", cpu->clock, admc);
                        break;
                }
            }
            //log_msg(LP_INFO, "CPU %d: Intermediate data_address_reduced %.4x", cpu->clock, cpu->intermediate.data_address_reduced);
            break;

        case CS_FETCH_DESTINATION_HIGH: // only fetch data from destination, nothing else
            //log_msg(LP_INFO, "CPU %d: Fetching destination from adm address", cpu->clock);
            {
                CS_FETCH_DESTINATION_HIGH:
                cpu->regs.sr.MNI = 0;
                int admc = cpu_reduced_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_reduced];
                switch (admc) {
                    case ADMC_IND:
                        {
                            uint8_t data;
                            int success = cpu_read_memory(cpu, cpu->intermediate.argument_address_reduced + 1, &data);
                            if (success) {
                                cpu->intermediate.data_address_reduced |= (uint16_t)(data << 8);
                                cpu->state = CS_EXECUTE;
                                goto CS_EXECUTE;
                            }
                            break;
                        }
                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown indirect extended addressing mode category %d", cpu->clock, admc);
                        break;
                }
            }
            //log_msg(LP_INFO, "CPU %d: Intermediate data_address_reduced %.4x", cpu->clock, cpu->intermediate.data_address_reduced);
            break;
        
        // =======================================================================================
        case CS_EXECUTE: 
            //log_msg(LP_INFO, "CPU %d: Executing instruction and writing to intermediate result for possible writeback", cpu->clock);
            /*printf("amdx add: %.4x, admr add: %.4x, amdx data: %.4x, admr data: %.4x\n", 
                cpu->intermediate.argument_address_extended, 
                cpu->intermediate.argument_address_reduced, 
                cpu->intermediate.data_address_extended, 
                cpu->intermediate.data_address_reduced);*/
            {
                CS_EXECUTE:
                cpu->regs.sr.MNI = 0;
                switch (cpu->intermediate.instruction) {
                    case NOP:
                        cpu->instruction ++;
                        cpu->state = CS_FETCH_INSTRUCTION;
                        goto CS_FETCH_INSTRUCTION;
                        break;
                    
                    #ifdef DCFF_CORE_BASE
                        case MOV:
                            cpu->intermediate.result = cpu->intermediate.data_address_extended;
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case PUSH:
                            cpu->intermediate.result = cpu->intermediate.data_address_extended;
                            cpu->state = CS_PUSH_HIGH;
                            goto CS_PUSH_HIGH;
                            break;

                        case POP:
                            cpu->intermediate.argument_address_reduced = cpu->regs.sp;
                            cpu->state = CS_POP_LOW;
                            goto CS_POP_LOW;
                            break;

                        case PUSHSR:
                            cpu->intermediate.result = cpu->regs.sr.value;
                            cpu->state = CS_PUSH_HIGH;
                            goto CS_PUSH_HIGH;
                            break;

                        case POPSR:
                            cpu->intermediate.argument_address_reduced = cpu->regs.sp;
                            cpu->state = CS_POPSR_LOW;
                            goto CS_POPSR_LOW;
                            break;
                        
                        
                        case JMP:
                            cpu->regs.pc = cpu->intermediate.data_address_extended;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JZ:
                            if (cpu->regs.sr.Z) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNZ:
                            if (!cpu->regs.sr.Z) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                
                        case CALL:
                            cpu->intermediate.result = cpu->regs.pc;
                            cpu->regs.pc = cpu->intermediate.data_address_extended;
                            cpu->state = CS_PUSH_HIGH;
                            goto CS_PUSH_HIGH;
                            break;
                        
                        case RET:
                            cpu->intermediate.argument_address_reduced = cpu->regs.sp;
                            cpu->state = CS_POP_LOW; // the part with writing to pc is taken care of in CS_POP_HIGH
                            goto CS_POP_LOW;
                            break;

                        case CMP:
                            {
                                uint16_t a = cpu->intermediate.data_address_reduced;
                                uint16_t b = cpu->intermediate.data_address_extended;
                                cpu->regs.sr.Z = (a == b);
                                cpu->regs.sr.L = ((int16_t) a < (int16_t) b);
                                cpu->regs.sr.UL = (a < b);
                                cpu->regs.sr.FL = (float_from_f16(a) < float_from_f16(b));
                                cpu->regs.sr.BL = (float_from_bf16(a) < float_from_bf16(b));
                                cpu->instruction ++;
                                cpu->state = CS_FETCH_INSTRUCTION;
                                goto CS_FETCH_INSTRUCTION;
                            }
                            break;
                        
                        case TST:
                            {
                                uint16_t value = cpu->intermediate.data_address_extended;
                                cpu->regs.sr.Z = (value == 0);
                                cpu->regs.sr.L = ((value & 0x8000) != 0);
                                cpu->instruction ++;
                                cpu->state = CS_FETCH_INSTRUCTION;
                                goto CS_FETCH_INSTRUCTION;
                            }
                            break;
                    
                        case AND:
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced & cpu->intermediate.data_address_extended;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        
                        case OR:
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced | cpu->intermediate.data_address_extended;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        
                        case XOR:
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced ^ cpu->intermediate.data_address_extended;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        
                        case NOT:
                            cpu->intermediate.result = ~cpu->intermediate.data_address_reduced;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case BWS:
                            {
                                int16_t shift = (int16_t) cpu->intermediate.data_address_extended;
                                if (shift > 0)
                                    cpu->intermediate.result = cpu->intermediate.data_address_reduced >> shift;
                                else 
                                    cpu->intermediate.result = cpu->intermediate.data_address_reduced << (-shift);
                                cpu_update_status_register(cpu, cpu->intermediate.result);
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                            }
                            break;

                        case INT:
                            //log_msg(LP_ERROR, "CPU %d: undefined instruction INT");
                            //printf("%c", cpu->intermediate.data_address_extended & 0xffff);
                            //cpu->instruction ++;
                            //cpu->state = CS_FETCH_INSTRUCTION;
                            cpu->instruction ++;
                            cpu->state = CS_INTERRUPT_PUSH_PC_HIGH;
                            goto CS_INTERRUPT_PUSH_PC_HIGH;
                            //cpu->intermediate.irq_id = ;
                            break;
    
                        case HLT:
                            cpu->instruction ++;
                            cpu->state = CS_HALT;
                            break;
                    #endif // DCFF_CORE_BASE

                    #ifdef DCFF_CORE_LEA
                        case LEA:
                            cpu->intermediate.result = cpu->intermediate.argument_address_extended;
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif

                    #ifdef DCFF_INT_ARITH
                        case ADD:
                            cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced + (int16_t) cpu->intermediate.data_address_extended;
                            cpu->regs.sr.AO = ((uint32_t) cpu->intermediate.data_address_reduced + (uint32_t) cpu->intermediate.data_address_extended) > 0xffff;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case SUB:
                            cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced - (int16_t) cpu->intermediate.data_address_extended;
                            cpu->regs.sr.AO = ((int32_t) cpu->intermediate.data_address_reduced - (int32_t) cpu->intermediate.data_address_extended) < 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case NEG:
                            cpu->intermediate.result = ~cpu->intermediate.data_address_reduced + 1;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
    
                        case ABS: {
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced;
                            if ((int16_t) cpu->intermediate.result < 0) {
                                cpu->intermediate.result = ~cpu->intermediate.data_address_reduced + 1;
                            }
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }
    
                        case INC:
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced + 1;
                            cpu->regs.sr.AO = cpu->intermediate.result == 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
    
                        case DEC:
                            cpu->intermediate.result = cpu->intermediate.data_address_reduced - 1;
                            cpu->regs.sr.AO = cpu->intermediate.result == 0xffff;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_INT_ARITH

                    #ifdef DCFF_INT_SIGNED_SAT_ARITH
                        case SSA: {
                            int32_t tmp = (int32_t) ((int16_t) cpu->intermediate.data_address_reduced) + (int32_t) ((int16_t) cpu->intermediate.data_address_extended);
                            cpu->intermediate.result = (tmp < (int32_t)((int16_t) 0x8000)) ? 0x8000 : ((tmp > (int16_t) 0x7fff) ? 0x7fff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }

                        case SSS: {
                            int32_t tmp = (int32_t) ((int16_t) cpu->intermediate.data_address_reduced) - (int32_t) ((int16_t) cpu->intermediate.data_address_extended);
                            cpu->intermediate.result = (tmp < (int32_t)((int16_t) 0x8000)) ? 0x8000 : ((tmp > (int32_t)((int16_t) 0x7fff)) ? 0x7fff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }

                        case SSM: {
                            int32_t tmp = (int32_t) ((int16_t) cpu->intermediate.data_address_reduced) * (int32_t) ((int16_t) cpu->intermediate.data_address_extended);
                            cpu->intermediate.result = (tmp < (int32_t)((int16_t) 0x8000)) ? 0x8000 : ((tmp > (int16_t) 0x7fff) ? 0x7fff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }
                    #endif // DCFF_INT_SIGNED_SAT_ARITH

                    #ifdef DCFF_INT_UNSIGNED_SAT_ARITH
                        case USA: {
                            int32_t tmp = (int32_t) cpu->intermediate.data_address_reduced + (int32_t) cpu->intermediate.data_address_extended;
                            cpu->intermediate.result = (tmp < 0) ? 0 : ((tmp > 0xffff) ? 0xffff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }

                        case USS: {
                            int32_t tmp = (int32_t) cpu->intermediate.data_address_reduced - (int32_t) cpu->intermediate.data_address_extended;
                            cpu->intermediate.result = (tmp < 0) ? 0 : ((tmp > 0xffff) ? 0xffff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }

                        case USM: {
                            int32_t tmp = (int32_t) cpu->intermediate.data_address_reduced * (int32_t) cpu->intermediate.data_address_extended;
                            cpu->intermediate.result = (tmp < 0) ? 0 : ((tmp > 0xffff) ? 0xffff : tmp);
                            cpu->regs.sr.AO = 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                        }
                    #endif // DCFF_INT_UNSIGNED_SAT_ARITH

                    #ifdef DCFF_INT_ARITH_EXT
                        case MUL:
                            cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced * (int16_t) cpu->intermediate.data_address_extended;
                            cpu->regs.sr.AO = ((uint32_t) cpu->intermediate.data_address_reduced * (uint32_t) cpu->intermediate.data_address_extended) > 0xffff;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case DIV:
                            if ((int16_t) cpu->intermediate.data_address_extended == 0) {
                                cpu->intermediate.result = 0;
                                cpu->regs.sr.AO = 1;
                            } else {
                                cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced / (int16_t) cpu->intermediate.data_address_extended;
                            }
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_INT_ARITH_EXT

                    #ifdef DCFF_INT_CARRY_EXT
                        case ADC:
                            cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced + (int16_t) cpu->intermediate.data_address_extended + cpu->regs.sr.AO;
                            cpu->regs.sr.AO = ((uint32_t) cpu->intermediate.data_address_reduced + (uint32_t) cpu->intermediate.data_address_extended) > 0xffff;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case SBC:
                            cpu->intermediate.result = (int16_t) cpu->intermediate.data_address_reduced - (int16_t) cpu->intermediate.data_address_extended - cpu->regs.sr.AO;
                            cpu->regs.sr.AO = ((int32_t) cpu->intermediate.data_address_reduced - (int32_t) cpu->intermediate.data_address_extended) < 0;
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_INT_CARRY_EXT

                    #ifdef DCFF_LOGIC_EXT
                        case JL:
                            if (cpu->regs.sr.L) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNL:
                            if (!cpu->regs.sr.L) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JUL:
                            if (cpu->regs.sr.UL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNUL:
                            if (!cpu->regs.sr.UL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JFL:
                            if (cpu->regs.sr.FL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNFL:
                            if (!cpu->regs.sr.FL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JBL:
                            if (cpu->regs.sr.BL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNBL:
                            if (!cpu->regs.sr.BL) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JAO:
                            if (cpu->regs.sr.AO) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case JNAO:
                            if (!cpu->regs.sr.AO) {
                                cpu->regs.pc = cpu->intermediate.data_address_extended;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CLZ:
                            cpu->regs.sr.Z = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEZ:
                            cpu->regs.sr.Z = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLL:
                            cpu->regs.sr.L = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEL:
                            cpu->regs.sr.L = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLUL:
                            cpu->regs.sr.UL = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEUL:
                            cpu->regs.sr.UL = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLFL:
                            cpu->regs.sr.FL = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEFL:
                            cpu->regs.sr.FL = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLBL:
                            cpu->regs.sr.BL = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEBL:
                            cpu->regs.sr.BL = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLAO:
                            cpu->regs.sr.AO = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEAO:
                            cpu->regs.sr.AO = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLSRC:
                            cpu->regs.sr.SRC = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SESRC:
                            cpu->regs.sr.SRC = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLSWC:
                            cpu->regs.sr.SWC = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SESWC:
                            cpu->regs.sr.SWC = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case CLMI:
                            cpu->regs.sr.MI = 0;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
    
                        case SEMI:
                            cpu->regs.sr.MI = 1;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                    #endif // CFF_LOGIC_EXT

                    #ifdef DCFF_CMOV_EXT
                        case CMOVZ:
                            if (cpu->regs.sr.Z) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVNZ:
                            if (!cpu->regs.sr.Z) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVL:
                            if (cpu->regs.sr.L) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVNL:
                            if (!cpu->regs.sr.L) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVUL:
                            if (cpu->regs.sr.UL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVNUL:
                            if (!cpu->regs.sr.UL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVFL:
                            if (cpu->regs.sr.FL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVNFL:
                            if (!cpu->regs.sr.FL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVBL:
                            if (cpu->regs.sr.BL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;

                        case CMOVNBL:
                            if (!cpu->regs.sr.BL) {
                                cpu->intermediate.result = cpu->intermediate.data_address_extended;
                                cpu->state = CS_WRITEBACK_LOW;
                                goto CS_WRITEBACK_LOW;
                                break;
                            }
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                    #endif // DCFF_CMOV_EXT

                    #ifdef DCFF_BYTE_EXT
                        case CBW:
                            cpu->intermediate.result = (int16_t) ((int8_t) cpu->intermediate.data_address_reduced);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_BYTE_EXT

                    #ifdef DCFF_FLOAT16
                        case ADDF:
                            cpu->intermediate.result = f16_add(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case SUBF:
                            cpu->intermediate.result = f16_sub(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case MULF:
                            cpu->intermediate.result = f16_mult(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case DIVF:
                            cpu->intermediate.result = f16_div(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_FLOAT16

                    #ifdef DCFF_BFLOAT16
                        case ADDBF:
                            cpu->intermediate.result = bf16_add(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case SUBBF:
                            cpu->intermediate.result = bf16_sub(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case MULBF:
                            cpu->intermediate.result = bf16_mult(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case DIVBF:
                            cpu->intermediate.result = bf16_div(cpu->intermediate.data_address_reduced, cpu->intermediate.data_address_extended);
                            cpu_update_status_register(cpu, cpu->intermediate.result);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_BFLOAT16

                    #ifdef DCFF_FLOAT_CONVERT
                        case CIF:
                            cpu->intermediate.result = f16_from_float((float) ((int16_t) cpu->intermediate.data_address_reduced));
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case CIB:
                            cpu->intermediate.result = bf16_from_float((float) ((int16_t) cpu->intermediate.data_address_reduced));
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case CFI:
                            cpu->intermediate.result = (int16_t) float_from_f16(cpu->intermediate.data_address_reduced);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case CFB:
                            cpu->intermediate.result = bf16_from_float(float_from_f16(cpu->intermediate.data_address_reduced));
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case CBI:
                            cpu->intermediate.result = (int16_t) float_from_bf16(cpu->intermediate.data_address_reduced);
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;

                        case CBF:
                            cpu->intermediate.result = f16_from_float(float_from_bf16(cpu->intermediate.data_address_reduced));
                            cpu->state = CS_WRITEBACK_LOW;
                            goto CS_WRITEBACK_LOW;
                            break;
                    #endif // DCFF_FLOAT_CONVERT

                    #ifdef DCFF_CACHE_EXT
                        case INV:
                            //log_msg(LP_ERROR, "CPU %d: undefined instruction INV");
                            //cpu->state = CS_EXCEPTION;
                            cache_invalidate(cpu->cache);
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case FTC: {
                            //log_msg(LP_ERROR, "CPU %d: undefined instruction FTC");
                            // doesnt even improve performance on optimal scenraios
                            uint8_t data;
                            int success = cpu_read_memory(cpu, cpu->intermediate.data_address_extended, &data);
                            if (success) {
                                cpu->instruction ++;
                                cpu->state = CS_FETCH_INSTRUCTION;
                                goto CS_FETCH_INSTRUCTION;
                            }
                            break;
                        }
                    #endif // DCFF_CACHE_EXT

                    #ifdef DCFF_HW_INFO
                        case HWCLOCK:
                            cpu->regs.r0 = cpu->clock & ((uint64_t) 0xffff << 0);
                            cpu->regs.r1 = cpu->clock & ((uint64_t) 0xffff << 16);
                            cpu->regs.r2 = cpu->clock & ((uint64_t) 0xffff << 32);
                            cpu->regs.r3 = cpu->clock & ((uint64_t) 0xffff << 48);
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case HWINSTR:
                            cpu->regs.r0 = cpu->instruction & ((uint64_t) 0xffff << 0);
                            cpu->regs.r1 = cpu->instruction & ((uint64_t) 0xffff << 16);
                            cpu->regs.r2 = cpu->instruction & ((uint64_t) 0xffff << 32);
                            cpu->regs.r3 = cpu->instruction & ((uint64_t) 0xffff << 48);
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                        
                        case HWFFLAG:
                            cpu->regs.r0 = cpu->feature_flag;
                            cpu->instruction ++;
                            cpu->state = CS_FETCH_INSTRUCTION;
                            goto CS_FETCH_INSTRUCTION;
                            break;
                    #endif // DCFF_HW_INFO

                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unknown instruction [%d]", cpu->intermediate.instruction);
                        cpu->instruction ++;
                        cpu->state = CS_EXCEPTION;
                        break;
                }
                //log_msg(LP_INFO, "CPU %d: intermediate result: 0x%.4x", cpu->clock, cpu->intermediate.result);
            }
            break;
        // =======================================================================================
        
        case CS_WRITEBACK_LOW: 
            //log_msg(LP_INFO, "CPU %d: Writing low result back to cpu/memory", cpu->clock);
            {
                CS_WRITEBACK_LOW:
                cpu->regs.sr.MNI = 1;
                switch (cpu_reduced_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_reduced]) {
                    case ADMC_IND:
                        {
                            int success = cpu_write_memory(cpu, cpu->intermediate.argument_address_reduced, (uint8_t) (cpu->intermediate.result & 0x00ff));
                            if (success) {
                                cpu->state = CS_WRITEBACK_HIGH;
                                goto CS_WRITEBACK_HIGH;
                            }
                        }
                        break;

                    case ADMC_REG:
                        switch (cpu->intermediate.addressing_mode.addressing_mode_reduced) {
                            case ADMR_R0:
                                cpu->regs.r0 = cpu->intermediate.result;
                                break;
                            case ADMR_R1:
                                cpu->regs.r1 = cpu->intermediate.result;
                                break;
                            case ADMR_R2:
                                cpu->regs.r2 = cpu->intermediate.result;
                                break;
                            case ADMR_R3:
                                cpu->regs.r3 = cpu->intermediate.result;
                                break;
                            case ADMR_SP:
                                cpu->regs.sp = cpu->intermediate.result;
                                break;
                            default:
                                break;
                        }
                        cpu->instruction ++;
                        cpu->state = CS_FETCH_INSTRUCTION;
                        goto CS_FETCH_INSTRUCTION;
                        break;

                    case ADMC_NONE:
                        // Here if writeback happens to admx instead of admr
                        //log_msg(LP_NOTICE, "CPU %d: Writeback to ADMX instead of ADMR", cpu->clock);
                        break;

                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown addressing mode category %d", cpu->clock, cpu->intermediate.addressing_mode.addressing_mode_reduced);
                        cpu->state = CS_EXCEPTION;
                        break;
                }
            }
            break;
        
        case CS_WRITEBACK_HIGH: 
            //log_msg(LP_INFO, "CPU %d: Writing high result back to memory", cpu->clock);
            {
                CS_WRITEBACK_HIGH:
                cpu->regs.sr.MNI = 1;
                switch (cpu_reduced_addressing_mode_category[cpu->intermediate.addressing_mode.addressing_mode_reduced]) {
                    case ADMC_IND:
                        {
                            int success = cpu_write_memory(cpu, cpu->intermediate.argument_address_reduced + 1, (uint8_t) ((cpu->intermediate.result & 0xff00) >> 8));
                            if (success) {
                                cpu->instruction ++;
                                cpu->state = CS_FETCH_INSTRUCTION;
                                goto CS_FETCH_INSTRUCTION;
                            }
                        }
                        break;

                    case ADMC_REG:
                        // TODO: switch case and write to register
                        switch (cpu->intermediate.addressing_mode.addressing_mode_reduced) {
                            case ADMR_R0:
                                cpu->regs.r0 = cpu->intermediate.result;
                                break;
                            case ADMR_R1:
                                cpu->regs.r1 = cpu->intermediate.result;
                                break;
                            case ADMR_R2:
                                cpu->regs.r0 = cpu->intermediate.result;
                                break;
                            case ADMR_R3:
                                cpu->regs.r1 = cpu->intermediate.result;
                                break;
                            case ADMR_SP:
                                cpu->regs.sp = cpu->intermediate.result;
                                break;
                            default:
                                break;
                        }
                        cpu->instruction ++;
                        cpu->state = CS_FETCH_INSTRUCTION;
                        goto CS_FETCH_INSTRUCTION;
                        break;
                    
                    case ADMC_IMM:
                    case ADMC_NONE:
                    default:
                        //log_msg(LP_ERROR, "CPU %d: Unkown addressing mode category %d", cpu->clock, cpu->intermediate.argument_address_reduced);
                        break;
                }
            }
            break;
        
        case CS_PUSH_HIGH: 
            //log_msg(LP_INFO, "CPU %d: Pushing low result", cpu->clock);
            {
                CS_PUSH_HIGH:
                cpu->regs.sr.MNI = 1;
                int success = cpu_write_memory(cpu, cpu->regs.sp - 1, (uint8_t) ((cpu->intermediate.result & 0xff00) >> 8));
                if (success) {
                    cpu->regs.sp --;
                    cpu->state = CS_PUSH_LOW;
                    goto CS_PUSH_LOW;
                }
            }
            break;
        
        case CS_PUSH_LOW: 
            //log_msg(LP_INFO, "CPU %d: Pushing high result", cpu->clock);
            {   
                CS_PUSH_LOW:
                cpu->regs.sr.MNI = 1;
                int success = cpu_write_memory(cpu, cpu->regs.sp - 1, (uint8_t) (cpu->intermediate.result & 0x00ff));
                if (success) {
                    cpu->regs.sp --;
                    cpu->instruction ++;
                    cpu->state = CS_FETCH_INSTRUCTION;
                    goto CS_FETCH_INSTRUCTION;
                }
            }
            break;
        
        case CS_POP_LOW: 
            //log_msg(LP_INFO, "CPU %d: Popping low result", cpu->clock);
            {
                CS_POP_LOW:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->regs.sp, &data);
                if (success) {
                    cpu->regs.sp ++;
                    cpu->intermediate.result = data;
                    //log_msg(LP_DEBUG, "CPU %d: intermediate result: %.4x", cpu->clock, cpu->intermediate.result);
                    cpu->state = CS_POP_HIGH;
                    goto CS_POP_HIGH;
                }
            }
            break;
        
        case CS_POP_HIGH: 
            //log_msg(LP_INFO, "CPU %d: Popping high result", cpu->clock);
            {   
                CS_POP_HIGH:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->regs.sp, &data);
                if (success) {
                    cpu->regs.sp ++;
                    cpu->intermediate.result |= (uint16_t) (data << 8);
                    //log_msg(LP_DEBUG, "CPU %d: intermediate result: %.4x", cpu->clock, cpu->intermediate.result);
                    if (cpu->intermediate.instruction == RET) {
                        cpu->regs.pc = cpu->intermediate.result;
                        cpu->instruction ++;
                        cpu->state = CS_FETCH_INSTRUCTION;
                        goto CS_FETCH_INSTRUCTION;
                    } else {
                        cpu->state = CS_WRITEBACK_LOW;
                        goto CS_WRITEBACK_LOW;
                    }
                }
            }
            break;
        
        case CS_POPSR_LOW: 
            //log_msg(LP_INFO, "CPU %d: Popping low result", cpu->clock);
            {
                CS_POPSR_LOW:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->regs.sp, &data);
                if (success) {
                    cpu->regs.sp ++;
                    cpu->regs.sr.value = (uint8_t) data;
                    //log_msg(LP_DEBUG, "CPU %d: intermediate result: %.4x", cpu->clock, cpu->intermediate.result);
                    cpu->state = CS_POPSR_HIGH;
                    goto CS_POPSR_HIGH;
                }
            }
            break;
        
        case CS_POPSR_HIGH: 
            //log_msg(LP_INFO, "CPU %d: Popping high result", cpu->clock);
            {   
                CS_POPSR_HIGH:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->regs.sp, &data);
                if (success) {
                    cpu->regs.sp ++;
                    cpu->regs.sr.value |= (uint16_t) (data << 8);
                    //log_msg(LP_DEBUG, "CPU %d: intermediate result: %.4x", cpu->clock, cpu->intermediate.result);
                    cpu->state = CS_FETCH_INSTRUCTION;
                    goto CS_FETCH_INSTRUCTION;
                }
            }
            break;
        
        case CS_INTERRUPT_PUSH_PC_HIGH:
            {
                CS_INTERRUPT_PUSH_PC_HIGH:
                cpu->regs.sr.MNI = 1;
                int success = cpu_write_memory(cpu, cpu->regs.sp - 1, (uint8_t) ((cpu->regs.pc & 0xff00) >> 8));
                if (success) {
                    cpu->regs.sp --;
                    cpu->state = CS_INTERRUPT_PUSH_PC_LOW;
                    goto CS_INTERRUPT_PUSH_PC_LOW;
                }
            }
            break;
        
        case CS_INTERRUPT_PUSH_PC_LOW:
            {   
                CS_INTERRUPT_PUSH_PC_LOW:
                cpu->regs.sr.MNI = 1;
                int success = cpu_write_memory(cpu, cpu->regs.sp - 1, (uint8_t) (cpu->regs.pc & 0x00ff));
                if (success) {
                    cpu->regs.sp --;
                    cpu->state = CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW;
                    goto CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW;
                }
            }
            break;
    
        case CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW:
            {
                CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->memory_layout.segment_irq_table + cpu->intermediate.irq_id * 2, &data);
                if (success) {
                    cpu->intermediate.data_address_extended = (uint8_t) data;
                    cpu->state = CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH;
                    goto CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH;
                }
            }
            break;
    
        case CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH:
            {
                CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH:
                cpu->regs.sr.MNI = 1;
                uint8_t data;
                int success = cpu_read_memory(cpu, cpu->memory_layout.segment_irq_table + cpu->intermediate.irq_id * 2 + 1, &data);
                if (success) {
                    cpu->intermediate.data_address_extended |= ((uint16_t) data) << 8;
                    cpu->regs.pc = cpu->intermediate.data_address_extended;
                    cpu->state = CS_FETCH_INSTRUCTION;
                    goto CS_FETCH_INSTRUCTION;
                }
            }
            break;

        case CS_HALT: 
            {
                cpu->regs.sr.MNI = 1;
                if (!halted) {
                    halted = 1;
                }
                //exit(1);
                break;
            }

        case CS_EXCEPTION:
            if (!halted) {
                log_msg(LP_ERROR, "CPU %d: EXCEPTION", cpu->clock);
                halted = 1;
            }
            break;

        default:
            if (!halted) {
                log_msg(LP_ERROR, "CPU %d: UNKNOWN STATE", cpu->clock);
                halted = 1;
            }
            break;
    }

    cpu->clock ++;
    return;
}

