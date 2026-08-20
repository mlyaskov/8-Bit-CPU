#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class CPU {
public:
    void reset();
    void loadProgram(const std::uint8_t* program, std::size_t size);
    void run(std::size_t maxInstructions = 100'000);
    void step();
    void setTrace(bool enabled) noexcept;

    std::uint8_t registerA() const noexcept { return a; }
    std::uint8_t registerB() const noexcept { return b; }
    std::uint8_t registerC() const noexcept { return c; }
    std::uint8_t registerD() const noexcept { return d; }
    std::uint8_t programCounter() const noexcept { return pc; }
    bool zeroFlag() const noexcept { return zero; }
    bool carryFlag() const noexcept { return carry; }
    bool isHalted() const noexcept { return halted; }
    std::uint8_t memoryAt(std::uint8_t address) const noexcept;

private:
    std::array<std::uint8_t, 256> memory{};
    std::uint8_t a = 0;
    std::uint8_t b = 0;
    std::uint8_t c = 0;
    std::uint8_t d = 0;
    std::uint8_t pc = 0;
    bool zero = false;
    bool carry = false;
    bool halted = false;
    bool traceEnabled = false;

    std::uint8_t fetch();
    void updateZero(std::uint8_t value);
    const char* instructionName(std::uint8_t opcode) const noexcept;
    void traceInstruction(
        std::uint8_t address,
        std::uint8_t opcode,
        std::uint8_t operand) const;
};
