#include "assembler.hpp"
#include "cpu.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testAdditionAndCarry()
{
    CPU cpu;
    const std::array<std::uint8_t, 8> program{
        0x10, 250, 0x11, 10, 0x20, 0, 0x01, 0
    };
    cpu.loadProgram(program.data(), program.size());
    cpu.run();

    expect(cpu.registerA() == 4, "ADD should wrap to 4");
    expect(cpu.carryFlag(), "ADD should set carry on overflow");
}

void testMemoryRoundTrip()
{
    CPU cpu;
    const std::array<std::uint8_t, 10> program{
        0x10, 42, 0x13, 0xF0, 0x10, 0, 0x12, 0xF0, 0x01, 0
    };
    cpu.loadProgram(program.data(), program.size());
    cpu.run();

    expect(cpu.memoryAt(0xF0) == 42, "STA should write memory");
    expect(cpu.registerA() == 42, "LDA should restore the stored value");
}

void testBinaryMultiplication()
{
    CPU cpu;
    const std::array<std::uint8_t, 22> program{
        0x10, 0x00, 0x11, 0x0D, 0x14, 0x0B, 0x15, 0x08,
        0x2B, 0x00, 0x34, 0x0E, 0x20, 0x00, 0x2A, 0x00,
        0x2C, 0x00, 0x32, 0x08, 0x01, 0x00
    };
    cpu.loadProgram(program.data(), program.size());
    cpu.run();

    expect(cpu.registerA() == 143, "13 * 11 should equal 143");
    expect(cpu.isHalted(), "multiplication program should halt");
}

void testAssemblerLabels()
{
    const auto program = assemble(R"(
        LDI A, 2
    loop:
        DEC A
        JNZ loop
        HLT
    )");

    const std::array<std::uint8_t, 8> expected{
        0x10, 2, 0x23, 0, 0x32, 0x02, 0x01, 0
    };
    expect(program.size() == expected.size(), "assembler emitted the wrong size");
    expect(std::equal(program.begin(), program.end(), expected.begin()),
           "assembler resolved a label incorrectly");
}

void testExecutionLimit()
{
    CPU cpu;
    const auto program = assemble("loop: JMP loop");
    cpu.loadProgram(program.data(), program.size());

    bool limitDetected = false;
    try {
        cpu.run(10);
    } catch (const std::runtime_error& error) {
        limitDetected = std::string(error.what()).find("execution limit") !=
                        std::string::npos;
    }
    expect(limitDetected, "run should stop an infinite program at its limit");
}
}

int main()
{
    try {
        testAdditionAndCarry();
        testMemoryRoundTrip();
        testBinaryMultiplication();
        testAssemblerLabels();
        testExecutionLimit();
        std::cout << "All Tiny-8 CPU tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
