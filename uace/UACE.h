#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct UAR {
    std::string architecture;
    std::string syntax;
};

struct CompiledResult {
    std::vector<uint8_t> objectCode;
    std::string error;
};

class UACE
{
public:
    virtual ~UACE() = default;

    // Detection - automatically detect syntax from source
    virtual std::string detectSyntax(const std::string& source) = 0;

    // Frontend pipeline
    virtual CompiledResult compile(const std::string& source) = 0;

    // UAR (Universal Assembly Representation)
    virtual UAR toUAR(const std::string& source) = 0;

    // ISA backend lowering
    virtual bool lowerToISA(UAR& uar, const std::string& targetIsa) = 0;

    // Instruction encoding
    virtual std::vector<uint8_t> encodeInstruction(const std::string& instruction) = 0;

    // Object file generation
    virtual CompiledResult emitObject() = 0;

    // Disassembly
    virtual std::string disassemble(const std::vector<uint8_t>& code) = 0;

    // Architecture support
    virtual std::vector<std::string> supportedArchitectures() const = 0;
    virtual std::string defaultArchitecture() const = 0;
};