#ifndef _CPU_INSTRUCTIONS_H_
#define _CPU_INSTRUCTIONS_H_

#include <stdint.h>

// need an instruction to just push and pop the status register

typedef enum CPU_INSTRUCTION_MNEMONIC {
    // Data Manipulation
    NOP,        // Does nothing
    MOV,        // mov dest, src    :: src -> dest
    PUSH,       // push src         :: sp -= 2; src -> [sp]
    POP,        // pop dest         :: [sp] -> dest; sp += 2
    PUSHSR,     // push status reg  :: sp -= 2; sr -> [sp] 
    POPSR,      // pop status reg   :: [sp] -> sr; sp += 2
    LEA,        // lea dest, [src]  :: address of src -> dest

    // Jumps and Calls
    JMP,        // jmp dest         :: dest -> pc
    JZ,         // jz dest          :: if Z == 1, dest -> pc
    JNZ,        // jnz dest         :: if Z == 0, dest -> pc
    JL,         // jl dest          :: if N == 1, dest -> pc
    JNL,        // jnl dest         :: if N == 0, dest -> pc
    JUL,        // jul dest         :: if UL == 1, dest -> pc
    JNUL,       // jnul dest        :: if UL == 0, dest -> pc
    JFL,        // jfl dest         :: if FL == 1, dest -> pc
    JNFL,       // jnfl dest        :: if FL == 0, dest -> pc
    JBL,        // jfl dest         :: if BL == 1, dest -> pc
    JNBL,       // jnfl dest        :: if BL == 0, dest -> pc
    JAO,        // jao dest         :: if AO == 1, dest -> pc
    JNAO,       // jnao dest        :: if AO == 0, dest -> pc

    CALL,       // call dest        :: sp -= 2; pc -> [sp]; dest -> pc
    RET,        // ret              :: [sp] -> pc; sp += 2

    // Arithmetic Integer Operations
    ADD,        // add dest, src    :: dest = dest + src (integer)
    ADC,        // adc dest, src    :: dest = dest + src (integer) + AO
    SUB,        // sub dest, src    :: dest = dest - src (integer)
    SBC,        // sbc dest, src    :: dest = dest - src (integer) - AO
    MUL,        // mul dest, src    :: dest = dest * src (integer)
    DIV,        // div dest, src    :: dest = dest / src (integer)
    NEG,        // neg dest         :: dest = dest ~ 0x8000
    ABS,        // abs dest         :: if dest < 0, dest = ~dest + 1
    INC,        // inc dest         :: dest = dest + 1
    DEC,        // dec dest         :: dest = dest - 1

    // Saturated Arithmetic Signed Integer Operations
    SSA,        // ssa dest, src    :: dest = clamp((dest + src), 0x8000, 0x7fff)
    SSS,        // ssb dest, src    :: dest = clamp((dest - src), 0x8000, 0x7fff)
    SSM,        // ssm dest, src    :: dest = clamp((dest * src), 0x8000, 0x7fff)

    // Saturated Arithmetic Unsigned Integer Operations
    USA,        // usa dest, src    :: dest = min((int32_t) (dest + src), 0xffff)
    USS,        // uss dest, src    :: dest = max((int32_t) (dest - src), 0x0000)
    USM,        // usm dest, src    :: dest = min((int32_t) (dest * src), 0xffff)

    // Arithmetic Float Operations
    ADDF,       // addf dest, src   :: dest = dest + src (float16)
    SUBF,       // subf dest, src   :: dest = dest - src (float16)
    MULF,       // mulf dest, src   :: dest = dest * src (float16)
    DIVF,       // divf dest, src   :: dest = dest / src (float16)

    // Arithmetic Float Operations
    ADDBF,       // addf dest, src   :: dest = dest + src (bfloat16)
    SUBBF,       // subf dest, src   :: dest = dest - src (bfloat16)
    MULBF,       // mulf dest, src   :: dest = dest * src (bfloat16)
    DIVBF,       // divf dest, src   :: dest = dest / src (bfloat16)

    // Type Conversion Operations
    CIF,        // cif dest         :: [c]onvert [i]nteger dest -> [f]loat16 -> dest
    CIB,        // cib dest         :: [c]onvert [i]nteger dest -> [b]float16 -> dest
    CFI,        // cfi dest         :: [c]onvert [f]loat16 dest -> [i]nteger -> dest
    CFB,        // cfb dest         :: [c]onvert [f]loat16 dest -> [b]float16 -> dest
    CBF,        // cbf dest         :: [c]onvert [b]float16 dest -> [f]loat16 -> dest
    CBI,        // cbi dest         :: [c]onvert [b]float16 dest -> [i]nteger -> dest
    CBW,        // cbw dest         :: [c]onvert [b]yte to [w]ord by sign extension (0x??80 -> 0xff80)

    // Bitwise Logic
    BWS,        // bws dest, src    :: Bitwise shift dest by src (left if src > 0, right if src < 0)
    AND,        // and dest, src    :: dest = dest & src
    OR,         // or dest, src     :: dest = dest | src
    XOR,        // xor dest, src    :: dest = dest ^ src
    NOT,        // not dest         :: dest = ~dest

    // Tests
    CMP,        // cmp a, b         :: sets Z, N, UL, FL based on (a - b); does not modify operands
    TST,        // tst val          :: sets Z and N based on val

    // Status Bit Manipulation
    CLZ,        // clear Z bit
    SEZ,        // set Z bit
    CLL,        // clear N/L bit
    SEL,        // set N/L bit
    CLUL,       // clear UL bit
    SEUL,       // set UL bit
    CLFL,       // clear FL bit
    SEFL,       // set FL bit
    CLBL,       // clear BL bit
    SEBL,       // set BL bit
    CLAO,       // clear AO bit
    SEAO,       // set AO bit
    CLSRC,      // clear SRC bit (enable cache read)
    SESRC,      // set SRC bit (skip cache read)
    CLSWC,      // clear SWC bit (enable cache write)
    SESWC,      // set SWC bit (skip cache write)
    CLMI,       // clear MI bit
    SEMI,       // set MI bit

    // Conditional Operations
    CMOVZ,      // cmovXX dest, src    :: if Z=1 {src -> dest}
    CMOVNZ,     // cmovXX dest, src    :: if Z=0 {src -> dest}
    CMOVL,      // cmovXX dest, src    :: if L=1 {src -> dest}
    CMOVNL,     // cmovXX dest, src    :: if L=0 {src -> dest}
    CMOVUL,     // cmovXX dest, src    :: if UL=1 {src -> dest}
    CMOVNUL,    // cmovXX dest, src    :: if UL=0 {src -> dest}
    CMOVFL,     // cmovXX dest, src    :: if FL=1 {src -> dest}
    CMOVNFL,    // cmovXX dest, src    :: if FL=0 {src -> dest}
    CMOVBL,     // cmovXX dest, src    :: if BL=1 {src -> dest}
    CMOVNBL,    // cmovXX dest, src    :: if BL=0 {src -> dest}

    // Cache Operations
    INV,        // [inv]alidate cache :: clears or marks all cache lines as invalid
    FTC,        // [f]e[t]ches data into [c]ache

    // Self Identification and HW-Info Operations
    HWCLOCK,    // returns the currrent [h]ard[w]are [clock] count in registers r0-r4 in little endian
    HWINSTR,    // returns the currrent [h]ard[w]are [instr]uction count in registers r0-r4 in little endian
    HWFFLAG,    // returns the hardware feature flag

    // Other
    INT,        // int dest         :: trigger interrupt; 0xEF00 + dest -> pc
    HLT,        // halt             :: stops CPU execution

    INSTRUCTION_COUNT // Number of defined instructions
} CPU_INSTRUCTION_MNEMONIC_t;


extern const char* cpu_instruction_string[INSTRUCTION_COUNT];

extern const int cpu_instruction_argument_count[INSTRUCTION_COUNT];

// this list marks all instructions, that write back to memory, but only uses one argument
// which means that the instruction should be locked to using admr only
extern const int cpu_instruction_single_operand_writeback[INSTRUCTION_COUNT];

extern const int cpu_instruction_is_jump[INSTRUCTION_COUNT];

#endif
