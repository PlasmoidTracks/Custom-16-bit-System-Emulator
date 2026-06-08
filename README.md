## Overview

This repository contains a self‑contained system centered around a custom 16‑bit CPU architecture.  
That system includes a system level emulator with a bus communicator between system components like RAM, ticker, terminal and memory banks. 
Each Module is cycle based and thus independent in their clock speed. 
Also included is an assembler, which includes a canonicalizer, peephole optimizer and a disassembler, with each their own compile-options (see main.c). 

## Quick Start

To compile with Makefile: 
```bash
make
```

The main executable is `main`:
```bash
./main <input_file> [options]
```

Try directly: 
```bash
./main ir_scripts/fibonacci.ir -run
```


Examples:

```bash
# Assemble ASM → BIN
./main hello.asm

# Compile IR → ASM → BIN, keep intermediates, then run in emulator
./main demo.ir -save-temps -run

# Compile with optimization disabled (by default enabled)
./main demo.ir -o -O0

# Compile arbitrary file with IR compiler
./main demo.txt -c=ir

# Compile as position independent binary and without main call preamble (useful for library integration)
./main demo.ir -pic -no-preamble

# Execute arbitrary files as code
./main any.file -c=bin -run

# Disassemble a binary
./main prog.bin -d
```

