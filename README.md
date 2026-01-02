# 16‑bit CPU & Toolchain  
*(Educational System Software Project)*

## Overview

This repository contains a self‑contained **educational system software stack** centered around a custom 16‑bit CPU architecture.  
The project was developed to explore and demonstrate **low‑level concepts** such as instruction set design, memory models, compiler frontends, and emulation — **not** as production‑grade embedded software.

The stack includes:

- A configurable 16‑bit CPU emulator (registers, flags, interrupts, MMIO, optional cache, clock ticker)
- An assembler (`.asm → .bin`)
- An intermediate representation (IR) frontend / compiler (`.ir → .asm`)
- Canonicalization and macro‑lowering stages
- A peephole optimizer (O1)
- A disassembler for binary inspection and debugging

The project is intentionally **architecture‑driven** and focuses on correctness, clarity, and extensibility rather than safety certification or coding‑standard compliance (e.g. MISRA‑C).

---

## Scope & Intent (Important)

This project is:

- ✅ A **learning and research project** focused on computer architecture and system software
- ✅ Publicly documented and reproducible
- ❌ Not production‑hardened embedded firmware
- ❌ Not safety‑certified or optimized for constrained microcontrollers

Design decisions intentionally prioritize **explicitness and inspectability** over defensive hardening.  
In a professional environment, additional measures such as static analysis, coding standards, and project‑specific safety constraints would naturally apply.

---

## Quick Start

The main executable is `main`:

```bash
./main <input_file> [options]
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

---

## Toolchain Pipeline

When invoked with `-c=ir`, the pipeline is:

1. force input type to be interpreted as `.ir`
2. IR compilation
3. Canonicalization
4. Peephole Optimization
5. Macro expansion based on CPU feature flags
6. Assembly into a flat binary
7. Optional disassembly and/or emulation

Intermediate files can be retained via `-save-temps`.

---

## Project Structure

```
src/
 ├─ compiler/
 ├─ cpu/                CPU core (fetch/decode/execute, flags, addressing)
 │   ├─ asm/            Assembler, disassembler, canonicalizer, optimizer, macro code expander
 │   ├─ ir/             IR lexer, parser, ruleset, compiler
 │   └─ ccan/           [WIP] ccan transpiler (going from CCAN (canonical C) to IR)
 ├─ modules/            RAM, cache, bus, MMIO devices, hooks, device interface, system integration
 ├─ utils/              Logging, helpers, float16/bfloat16 support
 ├─ CLI.c               CLI implementation
 └─ main.c              main entry point
```

---

## CPU Architecture (Summary)

- **Registers:** r0–r3, sp, pc, sr
- **Interrupts:** Vector‑based, configurable handlers
- **Memory model:** Segmented RAM with MMIO region
- **Instruction encoding:** Compact byte‑oriented format
- **Feature flags:** Compile‑time + runtime capability control

The macro expander automatically lowers unsupported instructions depending on the configured feature set.

---

## Compiler & IR

The IR is a small structured language that compiles to assembly using a stack‑frame model:

- Explicit scope management
- Stack‑based locals
- Inline assembly with symbolic variable substitution
- Optional position‑independent code (PIC)

The IR is designed for transparency rather than aggressive optimization.

---

## Optimizer

- Single optimization level (`-O1`)
- Conservative peephole transformations
- Avoids unsafe flag or control‑flow changes
- Designed for readability and debuggability

---

## Disassembler

The disassembler produces readable assembly with:

- Auto‑generated labels
- Segment hints
- Optional raw‑byte and ASCII annotations

Its primary purpose is inspection and debugging, not full decompilation.

---

## Status & Limitations

Implemented and functional:

- Assembler, IR compiler, optimizer, emulator, disassembler

Partially implemented / experimental:

- C / CCAN frontend (parser stubs exist, backend not wired)
- Virtual instructions (defined, but not fully lowered)
- `.include` directive

---

## Why this project exists

This repository serves as a **demonstration of low‑level understanding** across:

- Instruction set design
- Memory and interrupt models
- Compiler frontends and lowering stages
- Emulation and debugging workflows

It is intended to complement academic study and professional embedded development, not replace production frameworks.

---

## License

MIT
