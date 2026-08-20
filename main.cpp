#include "assembler.hpp"
#include "cpu.hpp"

int main()
{
    CPU cpu;
    const auto program = assemble(R"(
        LDI A, 0
        LDI B, 13
        LDI C, 11
        LDI D, 8

    loop:
        SHR C
        JNC skip_add
        ADD A, B

    skip_add:
        SHL B
        DEC D
        JNZ loop

        OUT A
        HLT
    )");

    cpu.loadProgram(program.data(), program.size());
    cpu.setTrace(true);
    cpu.run();
}
