#include "cpu.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>

void CPU::reset()
{
    a = b = c = d = pc = 0;
    zero = carry = halted = false;
}

void CPU::loadProgram(const std::uint8_t* program, std::size_t size)
{
    if (program == nullptr && size != 0) {
        throw std::invalid_argument("program pointer is null");
    }
    if (size > memory.size()) {
        throw std::length_error("program is too large for Tiny-8 memory");
    }
    memory.fill(0);
    if (size != 0) {
        std::copy_n(program, size, memory.begin());
    }
    reset();
}

std::uint8_t CPU::fetch()
{
    const std::uint8_t value = memory[pc];
    ++pc;
    return value;
}

void CPU::updateZero(std::uint8_t value)
{
    zero = (value == 0);
}

std::uint8_t CPU::memoryAt(std::uint8_t address) const noexcept
{
    return memory[address];
}

void CPU::step()
{
    if (halted) {
        return;
    }
    const std::uint8_t address = pc;
    const std::uint8_t opcode = fetch();
    const std::uint8_t operand = fetch();
    bool validInstruction = true;

    switch (opcode) {
        case 0x00: break;
        case 0x01: halted = true; break;
        case 0x10: a = operand; updateZero(a); break;
        case 0x11: b = operand; updateZero(b); break;
        case 0x12: a = memory[operand]; updateZero(a); break;
        case 0x13: memory[operand] = a; break;
        case 0x14: c = operand; updateZero(c); break;
        case 0x15: d = operand; updateZero(d); break;
        case 0x20: {
            const std::uint16_t result =
                static_cast<std::uint16_t>(a) + static_cast<std::uint16_t>(b);
            carry = result > 0xFF;
            a = static_cast<std::uint8_t>(result);
            updateZero(a);
            break;
        }
        case 0x21: carry = a < b; a = static_cast<std::uint8_t>(a - b); updateZero(a); break;
        case 0x22: ++a; updateZero(a); break;
        case 0x23: --a; updateZero(a); break;
        case 0x24: zero = (a == b); carry = (a < b); break;
        case 0x25: a = static_cast<std::uint8_t>(a & b); carry = false; updateZero(a); break;
        case 0x26: a = static_cast<std::uint8_t>(a | b); carry = false; updateZero(a); break;
        case 0x27: a = static_cast<std::uint8_t>(a ^ b); carry = false; updateZero(a); break;
        case 0x28: carry = (a & 0x80) != 0; a = static_cast<std::uint8_t>(a << 1); updateZero(a); break;
        case 0x29: carry = (a & 0x01) != 0; a = static_cast<std::uint8_t>(a >> 1); updateZero(a); break;
        case 0x2A: carry = (b & 0x80) != 0; b = static_cast<std::uint8_t>(b << 1); updateZero(b); break;
        case 0x2B: carry = (c & 0x01) != 0; c = static_cast<std::uint8_t>(c >> 1); updateZero(c); break;
        case 0x2C: --d; updateZero(d); break;
        case 0x30: pc = operand; break;
        case 0x31: if (zero) { pc = operand; } break;
        case 0x32: if (!zero) { pc = operand; } break;
        case 0x33: if (carry) { pc = operand; } break;
        case 0x34: if (!carry) { pc = operand; } break;
        case 0x40: std::cout << "OUTPUT: " << static_cast<unsigned int>(a) << '\n'; break;
        default: halted = true; validInstruction = false; break;
    }

    if (traceEnabled) {
        traceInstruction(address, opcode, operand);
    }
    if (!validInstruction) {
        throw std::runtime_error("unknown Tiny-8 instruction");
    }
}

void CPU::setTrace(bool enabled) noexcept
{
    traceEnabled = enabled;
}

void CPU::run(std::size_t maxInstructions)
{
    std::size_t executed = 0;
    while (!halted) {
        if (executed >= maxInstructions) {
            throw std::runtime_error("Tiny-8 execution limit exceeded");
        }
        step();
        ++executed;
    }
}

const char* CPU::instructionName(std::uint8_t opcode) const noexcept
{
    switch (opcode) {
        case 0x00: return "NOP";
        case 0x01: return "HLT";
        case 0x10: return "LDI A";
        case 0x11: return "LDI B";
        case 0x12: return "LDA";
        case 0x13: return "STA";
        case 0x14: return "LDI C";
        case 0x15: return "LDI D";
        case 0x20: return "ADD A,B";
        case 0x21: return "SUB A,B";
        case 0x22: return "INC A";
        case 0x23: return "DEC A";
        case 0x24: return "CMP A,B";
        case 0x25: return "AND A,B";
        case 0x26: return "OR A,B";
        case 0x27: return "XOR A,B";
        case 0x28: return "SHL A";
        case 0x29: return "SHR A";
        case 0x2A: return "SHL B";
        case 0x2B: return "SHR C";
        case 0x2C: return "DEC D";
        case 0x30: return "JMP";
        case 0x31: return "JZ";
        case 0x32: return "JNZ";
        case 0x33: return "JC";
        case 0x34: return "JNC";
        case 0x40: return "OUT A";
        default: return "INVALID";
    }
}

void CPU::traceInstruction(
    std::uint8_t address,
    std::uint8_t opcode,
    std::uint8_t operand) const
{
    const auto oldFlags = std::cout.flags();
    const auto oldFill = std::cout.fill();
    std::cout << std::uppercase << std::hex << std::setfill('0')
              << "ADDR=" << std::setw(2) << static_cast<unsigned int>(address)
              << " BYTES=" << std::setw(2) << static_cast<unsigned int>(opcode)
              << ' ' << std::setw(2) << static_cast<unsigned int>(operand)
              << std::setfill(' ') << "  " << std::left << std::setw(7)
              << instructionName(opcode) << std::right << std::setfill('0')
              << " A=" << std::setw(2) << static_cast<unsigned int>(a)
              << " B=" << std::setw(2) << static_cast<unsigned int>(b)
              << " C=" << std::setw(2) << static_cast<unsigned int>(c)
              << " D=" << std::setw(2) << static_cast<unsigned int>(d)
              << std::dec << "  Z=" << zero << " CY=" << carry
              << std::uppercase << std::hex << "  NEXT=" << std::setw(2)
              << static_cast<unsigned int>(pc) << '\n';
    std::cout.flags(oldFlags);
    std::cout.fill(oldFill);
}
