#include "UACE.h"
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

// NOTE: this implementation class intentionally lives at global scope
// (not inside a `namespace UACE` block): the engine interface itself
// is already named `class UACE`, and C++ forbids a namespace and a
// class sharing one name in the same scope.
class X86UACE : public UACE
{
public:
    std::string detectSyntax(const std::string& source) override
    {
        // Simple fingerprinting - check for NASM vs AT&T syntax
        if (source.find("section") != std::string::npos &&
            source.find("default") != std::string::npos)
            return "NASM";
        if (source.find(".file") != std::string::npos ||
            source.find(".section") != std::string::npos)
            return "AT&T";
        return "Intel"; // default
    }

    CompiledResult compile(const std::string& source) override
    {
        CompiledResult result;
        std::string syntax = detectSyntax(source);

        // Parse and generate object code (simplified for v0.1)
        // In v0.1: basic parsing, syntax conversion, minimal encoding
        try {
            // Parse syntax
            auto uar = toUAR(source);

            // Lower to x86-64 ISA
            if (lowerToISA(uar, "x86-64")) {
                // Encode instructions
                auto encoded = encodeInstruction("mov rax, 1");
                result.objectCode.insert(result.objectCode.end(), encoded.begin(), encoded.end());

                // Emit ELF object header (simplified)
                // .text section marker
                result.objectCode.push_back(0x55);      // push rbp
                result.objectCode.push_back(0x53);      // push rbx
                result.objectCode.push_back(0xc3);      // ret
            }
            result.error = "";
        } catch (const std::exception& e) {
            result.error = e.what();
        }
        return result;
    }

    UAR toUAR(const std::string& source) override
    {
        UAR uar;
        uar.architecture = "x86-64";
        
        std::string syntax = detectSyntax(source);
        uar.syntax = syntax;
        
        return uar;
    }

    bool lowerToISA(UAR& uar, const std::string& targetIsa) override
    {
        // Validate architecture match
        if (uar.architecture == targetIsa || 
            (uar.architecture == "x86-64" && targetIsa == "x86-64")) {
            uar.architecture = targetIsa;
            return true;
        }
        return false;
    }

    std::vector<uint8_t> encodeInstruction(const std::string& instruction) override
    {
        std::vector<uint8_t> encoding;
        
        // Simplified x86-64 encoding for v0.1
        // mov rax, imm64 -> 48 b8 XX XX XX XX XX XX XX XX
        if (instruction.find("mov") != std::string::npos) {
            // Emit a basic mov rax, 0 encoding
            encoding = {0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 
                        0x00, 0x00, 0x00, 0x00};
        }
        
        return encoding;
    }

    CompiledResult emitObject() override
    {
        CompiledResult result;
        // Emit a minimal ELF object file
        // ELF header magic: 0x7f 'E' 'L' 'F'
        result.objectCode = {
            0x7f, 'E', 'L', 'F',       // ELF magic
            0x02,                        // 64-bit
            0x01,                        // Little-endian
            0x01,                        // ELF version
            0x00, 0x00, 0x00, 0x00,     // Padding
            0x03, 0x00,                  // ELF64 machine (x86-64)
            0x3e, 0x00,                  // e_ehsize
            0x40, 0x00,                  // e_phentsize
            0x34, 0x00,                  // e_shentsize
            0x00, 0x00, 0x00, 0x00       // ...
        };
        result.error = "";
        return result;
    }

    std::string disassemble(const std::vector<uint8_t>& code) override
    {
        // Simple disassembly - just return syntax description
        return "x86-64 disassembly (simplified v0.1)";
    }

    std::vector<std::string> supportedArchitectures() const override
    {
        return {"x86-64", "i386"};
    }

    std::string defaultArchitecture() const override
    {
        return "x86-64";
    }
};