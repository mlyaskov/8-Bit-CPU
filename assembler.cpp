#include "assembler.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

using Labels = std::unordered_map<std::string, std::uint8_t>;

struct SourceInstruction {
    std::size_t line;
    std::string text;
};

std::string trim(std::string text)
{
    const auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    const auto first = std::find_if(text.begin(), text.end(), notSpace);
    if (first == text.end()) {
        return {};
    }
    const auto last = std::find_if(text.rbegin(), text.rend(), notSpace).base();
    return std::string(first, last);
}

std::string uppercase(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return text;
}

[[noreturn]] void fail(std::size_t line, const std::string& message)
{
    throw std::runtime_error("assembly line " + std::to_string(line) + ": " + message);
}

bool validLabel(const std::string& label)
{
    if (label.empty()) {
        return false;
    }
    const auto first = static_cast<unsigned char>(label.front());
    if (!std::isalpha(first) && label.front() != '_') {
        return false;
    }
    return std::all_of(label.begin() + 1, label.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == '_';
    });
}

std::vector<std::string> tokenize(std::string text)
{
    std::replace(text.begin(), text.end(), ',', ' ');
    std::istringstream input(uppercase(std::move(text)));
    std::vector<std::string> tokens;
    for (std::string token; input >> token;) {
        tokens.push_back(std::move(token));
    }
    return tokens;
}

std::uint8_t parseByte(
    const std::string& token,
    const Labels& labels,
    std::size_t line)
{
    if (const auto label = labels.find(token); label != labels.end()) {
        return label->second;
    }

    unsigned long value = 0;
    std::size_t parsed = 0;
    try {
        if (token.size() > 2 && token[0] == '0' && token[1] == 'B') {
            value = std::stoul(token.substr(2), &parsed, 2);
            if (parsed != token.size() - 2) {
                fail(line, "invalid binary value '" + token + "'");
            }
        } else {
            value = std::stoul(token, &parsed, 0);
            if (parsed != token.size()) {
                fail(line, "invalid value '" + token + "'");
            }
        }
    } catch (const std::exception&) {
        fail(line, "unknown label or invalid byte '" + token + "'");
    }
    if (value > 0xFF) {
        fail(line, "value is outside the 8-bit range: " + token);
    }
    return static_cast<std::uint8_t>(value);
}

void requireCount(
    const std::vector<std::string>& tokens,
    std::size_t expected,
    std::size_t line)
{
    if (tokens.size() != expected) {
        fail(line, "wrong number of operands for " + tokens.front());
    }
}

std::pair<std::uint8_t, std::uint8_t> encode(
    const SourceInstruction& instruction,
    const Labels& labels)
{
    const auto tokens = tokenize(instruction.text);
    const auto line = instruction.line;
    if (tokens.empty()) {
        fail(line, "empty instruction");
    }
    const std::string& name = tokens.front();

    if (name == "NOP" || name == "HLT") {
        requireCount(tokens, 1, line);
        return {name == "NOP" ? 0x00 : 0x01, 0};
    }
    if (name == "LDI") {
        requireCount(tokens, 3, line);
        const std::unordered_map<std::string, std::uint8_t> opcodes{
            {"A", 0x10}, {"B", 0x11}, {"C", 0x14}, {"D", 0x15}
        };
        const auto opcode = opcodes.find(tokens[1]);
        if (opcode == opcodes.end()) {
            fail(line, "LDI requires register A, B, C, or D");
        }
        return {opcode->second, parseByte(tokens[2], labels, line)};
    }
    if (name == "LDA" || name == "STA") {
        requireCount(tokens, 2, line);
        return {name == "LDA" ? 0x12 : 0x13, parseByte(tokens[1], labels, line)};
    }
    if (name == "ADD" || name == "SUB" || name == "CMP" ||
        name == "AND" || name == "OR" || name == "XOR") {
        requireCount(tokens, 3, line);
        if (tokens[1] != "A" || tokens[2] != "B") {
            fail(line, name + " requires operands A, B");
        }
        const std::unordered_map<std::string, std::uint8_t> opcodes{
            {"ADD", 0x20}, {"SUB", 0x21}, {"CMP", 0x24},
            {"AND", 0x25}, {"OR", 0x26}, {"XOR", 0x27}
        };
        return {opcodes.at(name), 0};
    }
    if (name == "INC" || name == "DEC") {
        requireCount(tokens, 2, line);
        if (name == "INC" && tokens[1] == "A") {
            return {0x22, 0};
        }
        if (name == "DEC" && tokens[1] == "A") {
            return {0x23, 0};
        }
        if (name == "DEC" && tokens[1] == "D") {
            return {0x2C, 0};
        }
        fail(line, "unsupported register for " + name);
    }
    if (name == "SHL" || name == "SHR") {
        requireCount(tokens, 2, line);
        if (name == "SHL" && tokens[1] == "A") return {0x28, 0};
        if (name == "SHR" && tokens[1] == "A") return {0x29, 0};
        if (name == "SHL" && tokens[1] == "B") return {0x2A, 0};
        if (name == "SHR" && tokens[1] == "C") return {0x2B, 0};
        fail(line, "unsupported register for " + name);
    }
    if (name == "JMP" || name == "JZ" || name == "JNZ" ||
        name == "JC" || name == "JNC") {
        requireCount(tokens, 2, line);
        const std::unordered_map<std::string, std::uint8_t> opcodes{
            {"JMP", 0x30}, {"JZ", 0x31}, {"JNZ", 0x32},
            {"JC", 0x33}, {"JNC", 0x34}
        };
        return {opcodes.at(name), parseByte(tokens[1], labels, line)};
    }
    if (name == "OUT") {
        requireCount(tokens, 2, line);
        if (tokens[1] != "A") {
            fail(line, "OUT requires register A");
        }
        return {0x40, 0};
    }

    fail(line, "unknown instruction '" + name + "'");
}
}

std::vector<std::uint8_t> assemble(std::string_view source)
{
    Labels labels;
    std::vector<SourceInstruction> instructions;
    std::istringstream input{std::string(source)};
    std::size_t address = 0;
    std::size_t lineNumber = 0;

    for (std::string line; std::getline(input, line);) {
        ++lineNumber;
        if (const auto comment = line.find(';'); comment != std::string::npos) {
            line.erase(comment);
        }
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }

        if (const auto colon = line.find(':'); colon != std::string::npos) {
            const std::string label = uppercase(trim(line.substr(0, colon)));
            if (!validLabel(label)) {
                fail(lineNumber, "invalid label");
            }
            if (address > 0xFF) {
                fail(lineNumber, "label is outside Tiny-8 memory");
            }
            if (!labels.emplace(label, static_cast<std::uint8_t>(address)).second) {
                fail(lineNumber, "duplicate label '" + label + "'");
            }
            line = trim(line.substr(colon + 1));
            if (line.empty()) {
                continue;
            }
        }

        if (address + 2 > 256) {
            fail(lineNumber, "program exceeds 256 bytes");
        }
        instructions.push_back({lineNumber, std::move(line)});
        address += 2;
    }

    std::vector<std::uint8_t> program;
    program.reserve(instructions.size() * 2);
    for (const auto& instruction : instructions) {
        const auto [opcode, operand] = encode(instruction, labels);
        program.push_back(opcode);
        program.push_back(operand);
    }
    return program;
}
