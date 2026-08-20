# 8 Bit CPU emulator

This is a fictional educational 8-bit CPU emulator. It has four registers,
256 bytes of shared program/data memory, fixed-width instructions, an assembler,
execution tracing, and protection against infinite programs.

## Build and run

```sh
make
./main
```

Run the automated tests with:

```sh
make test
```

The demo assembles and executes an 8-bit binary multiplication program for
`13 * 11`, producing `143`.

## Architecture

- Registers: `A`, `B`, `C`, `D`, and program counter `PC`
- Flags: zero (`Z`) and carry/borrow (`CY`)
- Memory: 256 bytes, addressed from `0x00` through `0xFF`
- Instruction size: two bytes (`opcode`, `operand`)
- Arithmetic wraps modulo 256
- Programs load at address `0x00`

Because every instruction is two bytes, assembler labels always resolve to
even addresses. Program and data memory are shared, so `STA` can modify any
memory byte, including program bytes.

## Assembler syntax

Instructions and register names are case-insensitive. Values can be decimal,
hexadecimal (`0x2A`), or binary (`0b00101010`). A semicolon starts a comment.
Labels can appear alone or before an instruction.

```asm
    LDI A, 3

loop:
    OUT A          ; print 3, 2, 1
    DEC A
    JNZ loop
    HLT
```

`assemble(source)` returns a `std::vector<std::uint8_t>` ready for
`CPU::loadProgram()`.

## Instruction set

| Opcode | Assembly | Effect | Flags |
|---:|---|---|---|
| `00` | `NOP` | No operation | Unchanged |
| `01` | `HLT` | Halt execution | Unchanged |
| `10` | `LDI A, value` | Load immediate value into A | Z |
| `11` | `LDI B, value` | Load immediate value into B | Z |
| `12` | `LDA address` | Load memory into A | Z |
| `13` | `STA address` | Store A in memory | Unchanged |
| `14` | `LDI C, value` | Load immediate value into C | Z |
| `15` | `LDI D, value` | Load immediate value into D | Z |
| `20` | `ADD A, B` | A = A + B | Z, CY |
| `21` | `SUB A, B` | A = A - B | Z, CY |
| `22` | `INC A` | Increment A | Z |
| `23` | `DEC A` | Decrement A | Z |
| `24` | `CMP A, B` | Compare A with B without changing them | Z, CY |
| `25` | `AND A, B` | Bitwise AND into A | Z; clears CY |
| `26` | `OR A, B` | Bitwise OR into A | Z; clears CY |
| `27` | `XOR A, B` | Bitwise XOR into A | Z; clears CY |
| `28` | `SHL A` | Shift A left | Z, CY |
| `29` | `SHR A` | Shift A right | Z, CY |
| `2A` | `SHL B` | Shift B left | Z, CY |
| `2B` | `SHR C` | Shift C right | Z, CY |
| `2C` | `DEC D` | Decrement D | Z |
| `30` | `JMP address` | Unconditional jump | Unchanged |
| `31` | `JZ address` | Jump when Z is set | Unchanged |
| `32` | `JNZ address` | Jump when Z is clear | Unchanged |
| `33` | `JC address` | Jump when CY is set | Unchanged |
| `34` | `JNC address` | Jump when CY is clear | Unchanged |
| `40` | `OUT A` | Print A as an unsigned decimal value | Unchanged |

For subtraction and comparison, `CY` is set when an unsigned borrow occurs.
For shifts, `CY` receives the bit shifted out of the register.

## Execution safety and debugging

`CPU::run()` stops after 100,000 instructions by default and throws an exception
if the program has not halted. A different limit can be supplied:

```cpp
cpu.run(1'000);
```

Use `cpu.setTrace(true)` before running to print each instruction and the
resulting registers, flags, and next program counter.

