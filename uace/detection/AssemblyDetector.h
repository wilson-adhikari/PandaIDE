#pragma once

#include "UACE.h"
#include <string>
#include <vector>
#include <map>

// Assembly syntax fingerprints for detection
struct SyntaxFingerprint {
    std::string syntax;
    std::vector<std::string> keywords;
    std::vector<std::string> directives;
};

// Architecture fingerprints
struct ArchitectureFingerprint {
    std::string arch;
    std::string syntax;
    double confidence;
};

// Result of assembly detection
struct DetectionResult {
    std::string syntax;
    std::string architecture;
    double confidence;
    std::vector<std::string> supportedDialects;
};

class AssemblyDetector
{
public:
    // Detect syntax from assembly source content
    static DetectionResult detectSyntax(const std::string& source);
    
    // Detect architecture from assembly source
    static std::string detectArchitecture(const std::string& source);
    
    // Get all supported syntaxes
    static std::vector<std::string> getSupportedSyntaxes();
    
    // Get confidence score for syntax
    static double getConfidence(const std::string& syntax, const std::string& source);
    
    // Check if source is valid assembly
    static bool isValidAssembly(const std::string& source);
    
    // Get detected directives
    static std::vector<std::string> getDirectives(const std::string& source);
    
    // Get detected instructions pattern
    static std::vector<std::string> getInstructions(const std::string& source);
};

// Built-in syntax fingerprints for supported assemblers.
// inline (C++17) so the header can be included from multiple
// translation units without ODR violations.
inline const std::vector<SyntaxFingerprint> SYNTAX_FINGERPRINTS = {
    // NASM (Intel-style)
    {"NASM", {
        "section", "default", "bits", "cpu", "org", "dw", "dd", "dq",
        "equ", "times", "db", "dw", "dd", "dq", "resb", "resw", "resd"
    }, {
        "global", "extern", "section", "default", "bits", "cpu"
    }},
    
    // AT&T syntax
    {"AT&T", {
        ".file", ".section", ".text", ".data", ".globl",
        "movl", "movq", "pushl", "popl"
    }, {
        ".file", ".section", ".text", ".data", ".globl"
    }},
    
    // MASM (Microsoft)
    {"MASM", {
        ".MODEL", ".STACK", ".DATA", ".CODE", "ASSUME",
        "invoke", "push", "pop", "ret"
    }, {
        ".MODEL", ".STACK", ".DATA", ".CODE", "ASSUME"
    }},
    
    // GAS (GNU Assembler)
    {"GAS", {
        ".file", ".section", ".globl", ".type", ".size",
        ".int", ".long", ".quad"
    }, {
        ".file", ".section", ".globl", ".type", ".size"
    }},
    
    // ARM ASM (ARMASM)
    {"ARMASM", {
        "AREA", "CODE", "DATA", "IMPORT", "EXPORT",
        "CSEG", "DSEG", "GLOBAL", "WEAK"
    }, {
        "AREA", "CODE", "DATA", "IMPORT", "EXPORT"
    }},
    
    // Motorola 68k
    {"Motorola", {
        "ORG", "EQU", "FCB", "FCC", "FDB",
        "MOVE", "ADD", "SUB", "CLR"
    }, {
        "ORG", "EQU", "FCB", "FCC", "FDB"
    }}
};

// x86-64 architecture patterns (inline: see above)
inline const std::map<std::string, std::string> ARCH_PATTERNS = {
    {"x86-64", "rsp, rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, rip"},
    {"i386", "eax, ebx, ecx, edx, esi, edi, ebp, esp"},
    {"aarch64", "x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15"},
    {"arm", "r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15"},
    {"riscv", "x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15"}
};