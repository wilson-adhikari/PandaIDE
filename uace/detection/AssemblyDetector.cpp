#include "AssemblyDetector.h"
#include <algorithm>
#include <cctype>
#include <sstream>

DetectionResult AssemblyDetector::detectSyntax(const std::string& source)
{
    DetectionResult result;
    result.confidence = 0.0;
    result.supportedDialects.clear();
    
    if (source.empty()) {
        result.syntax = "Unknown";
        return result;
    }
    
    // Normalize source for analysis (lowercase for keyword matching)
    std::string lowerSource = source;
    std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Score each syntax fingerprint
    int bestScore = 0;
    
    for (const auto& fingerprint : SYNTAX_FINGERPRINTS) {
        int score = 0;
        
        // Check keywords
        for (const auto& keyword : fingerprint.keywords) {
            // Search for keyword as whole word
            std::string lowerKeyword = keyword;
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            
            if (lowerSource.find(lowerKeyword) != std::string::npos) {
                score += 2;
            }
        }
        
        // Check directives
        for (const auto& directive : fingerprint.directives) {
            if (lowerSource.find(directive) != std::string::npos) {
                score += 3; // Directives carry more weight
            }
        }
        
        // Bonus for syntax-specific patterns
        if (fingerprint.syntax == "NASM") {
            // NASM typically has "section" and "default" together
            if (lowerSource.find("section") != std::string::npos &&
                lowerSource.find("default") != std::string::npos) {
                score += 5;
            }
        } else if (fingerprint.syntax == "AT&T") {
            // AT&T has distinct .file, .section, .globl patterns
            if (lowerSource.find(".file") != std::string::npos &&
                lowerSource.find(".globl") != std::string::npos) {
                score += 5;
            }
        } else if (fingerprint.syntax == "MASM") {
            // MASM has .MODEL, .STACK, .DATA, .CODE
            if (lowerSource.find(".model") != std::string::npos &&
                lowerSource.find(".code") != std::string::npos) {
                score += 5;
            }
        }
        
        if (score > bestScore) {
            bestScore = score;
            result.syntax = fingerprint.syntax;
        }
    }
    
    result.confidence = (bestScore > 0) ? 
        std::min(100.0, (bestScore / 15.0) * 100.0) : 0.0;
    
    // Determine supported dialects based on confidence
    if (result.confidence >= 80.0) {
        result.supportedDialects.push_back(result.syntax);
    } else if (result.confidence >= 50.0) {
        // Lower confidence - suggest multiple possibilities
        if (result.syntax == "NASM" || result.syntax == "Unknown") {
            result.supportedDialects = {"NASM", "AT&T", "Intel"};
        } else if (result.syntax == "AT&T") {
            result.supportedDialects = {"AT&T", "Intel"};
        }
    }
    
    return result;
}

std::string AssemblyDetector::detectArchitecture(const std::string& source)
{
    if (source.empty()) return "Unknown";
    
    std::string lowerSource = source;
    std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // NOTE: the ARCH_PATTERNS table holds full register lists for
    // documentation, but real sources never contain the whole list
    // literally, so detection scans for individual register and
    // mnemonic tokens instead (order matters: most specific first).
    const auto containsAny = [&](const std::vector<std::string>& tokens) {
        for (const auto& t : tokens) {
            if (lowerSource.find(t) != std::string::npos) {
                return true;
            }
        }
        return false;
    };

    // x86-64: 64-bit mode directives or 64-bit registers.
    if (lowerSource.find("bits 64") != std::string::npos ||
        lowerSource.find("default 64") != std::string::npos ||
        containsAny({"rax", "rbx", "rcx", "rdx", "rsi", "rdi",
                     "rbp", "rsp", "rip", "r8", "r9", "r10",
                     "r11", "r12", "r13", "r14", "r15"})) {
        return "x86-64";
    }

    // i386: 32-bit mode directives or 32-bit registers.
    if (lowerSource.find("bits 32") != std::string::npos ||
        lowerSource.find("default 32") != std::string::npos ||
        containsAny({"eax", "ebx", "ecx", "edx", "esi",
                     "edi", "ebp", "esp"})) {
        return "i386";
    }

    // RISC-V: distinctive mnemonics.
    if (containsAny({"addi ", "lw ", "sw ", "jalr", "ecall",
                     "ebreak", "lui ", "auipc"})) {
        return "riscv";
    }

    // AArch64: adrp is effectively unique to 64-bit ARM.
    if (lowerSource.find("adrp") != std::string::npos) {
        return "aarch64";
    }

    // ARM (32-bit): LDR/STR/MOV on r-registers, bx lr, push/pop sets.
    if (containsAny({"ldr r", "str r", "mov r", "bx lr",
                     "push {", "pop {", ".arm"})) {
        return "arm";
    }

    return "Unknown";
}

double AssemblyDetector::getConfidence(const std::string& syntax, const std::string& source)
{
    DetectionResult result = detectSyntax(source);
    if (result.syntax == syntax) {
        return result.confidence;
    }
    return 0.0;
}

bool AssemblyDetector::isValidAssembly(const std::string& source)
{
    if (source.empty()) return false;
    
    // Must have at least some recognizable patterns
    std::string lowerSource = source;
    std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Check for common assembly patterns
    int patternCount = 0;
    std::vector<std::string> patterns = {
        "section", "default", "bits", "org", "equ", "global",
        ".file", ".section", ".text", ".data", ".globl",
        ".model", ".stack", ".code", "area", "org", "equ"
    };
    
    for (const auto& pattern : patterns) {
        if (lowerSource.find(pattern) != std::string::npos) {
            patternCount++;
        }
    }
    
    return patternCount >= 2;
}

std::vector<std::string> AssemblyDetector::getDirectives(const std::string& source)
{
    std::vector<std::string> directives;
    std::string lowerSource = source;
    std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Common directive patterns
    std::vector<std::string> allDirectives = {
        "section", "default", "bits", "cpu", "org", "equ",
        ".file", ".section", ".text", ".data", ".bss",
        ".globl", ".type", ".size", ".model", ".stack",
        ".code", "area", "cseg", "dseg", "import", "export"
    };
    
    for (const auto& dir : allDirectives) {
        if (lowerSource.find(dir) != std::string::npos) {
            directives.push_back(dir);
        }
    }
    
    return directives;
}

std::vector<std::string> AssemblyDetector::getInstructions(const std::string& source)
{
    std::vector<std::string> instructions;
    std::string lowerSource = source;
    std::transform(lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    
    // Common x86/instruction patterns (simplified)
    // In a full implementation, this would use proper parsing
    std::vector<std::string> commonInstr = {
        "mov", "add", "sub", "xor", "and", "or", "not",
        "push", "pop", "pushf", "popf",
        "call", "ret", "jmp", "je", "jg", "jl",
        "int", "iret", "cli", "sti",
        "mova", "movb", "movw", "movl",
        "ldr", "str", "add", "sub", "mul", "div"
    };
    
    for (const auto& instr : commonInstr) {
        // Count occurrences
        size_t count = 0;
        size_t pos = 0;
        while ((pos = lowerSource.find(instr, pos)) != std::string::npos) {
            count++;
            pos += instr.length();
        }
        if (count > 0) {
            instructions.push_back(instr);
        }
    }
    
    return instructions;
}

std::vector<std::string> AssemblyDetector::getSupportedSyntaxes()
{
    std::vector<std::string> syntaxes;
    for (const auto& fingerprint : SYNTAX_FINGERPRINTS) {
        syntaxes.push_back(fingerprint.syntax);
    }
    return syntaxes;
}