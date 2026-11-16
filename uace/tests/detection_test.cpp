// Standalone smoke test for AssemblyDetector.
// Not part of the uace static library (see uace/CMakeLists.txt).
// Build manually, e.g. from the repo root:
//   g++ -std=c++17 -I uace uace/detection/AssemblyDetector.cpp
//       uace/tests/detection_test.cpp -o build/detection_test
#include "detection/AssemblyDetector.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& name)
{
    if (condition) {
        std::cout << "[PASS] " << name << "\n";
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++failures;
    }
}

} // namespace

int main()
{
    const std::string nasm = "section .text\ndefault rel\nglobal _start\n_start:\n    mov rax, 1\n";
    const std::string att = ".file \"t.s\"\n.section .text\n.globl _start\n_start:\n    movl $1, %eax\n";
    const std::string masm = ".MODEL small\n.STACK 100h\n.DATA\n.CODE\nstart:\nend start\n";
    const std::string empty;

    const DetectionResult rNasm = AssemblyDetector::detectSyntax(nasm);
    check(rNasm.syntax == "NASM", "NASM source detected as NASM");

    const DetectionResult rAtt = AssemblyDetector::detectSyntax(att);
    check(rAtt.syntax == "AT&T" || rAtt.syntax == "GAS", "AT&T source detected as AT&T/GAS");

    const DetectionResult rMasm = AssemblyDetector::detectSyntax(masm);
    check(rMasm.syntax == "MASM", "MASM source detected as MASM");

    const DetectionResult rEmpty = AssemblyDetector::detectSyntax(empty);
    check(rEmpty.syntax == "Unknown", "empty source detected as Unknown");

    check(AssemblyDetector::isValidAssembly(nasm), "NASM source is valid assembly");
    check(!AssemblyDetector::isValidAssembly("hello world"), "plain text is not assembly");

    check(AssemblyDetector::detectArchitecture(nasm) == "x86-64", "x86-64 arch detected");

    const std::vector<std::string> syntaxes = AssemblyDetector::getSupportedSyntaxes();
    check(!syntaxes.empty(), "supported syntax list is non-empty");

    if (failures == 0) {
        std::cout << "All detection tests passed.\n";
        return 0;
    }
    std::cout << failures << " test(s) failed.\n";
    return 1;
}
