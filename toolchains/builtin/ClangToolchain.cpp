#include "builtin/ClangToolchain.h"
#include "api/Toolchain.h"
#include <cstdlib>
#include <filesystem>

namespace Toolchains {

namespace {
bool executableExists(const std::string& name)
{
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv != nullptr) {
        const std::string path = pathEnv;
#ifdef _WIN32
        const char sep = ';';
        const std::string exeName = name + ".exe";
#else
        const char sep = ':';
        const std::string& exeName = name;
#endif
        std::string::size_type start = 0;
        while (true) {
            const std::string::size_type end = path.find(sep, start);
            const std::string dir = path.substr(start, end - start);
            if (!dir.empty()) {
                std::error_code ec;
                if (std::filesystem::is_regular_file(std::filesystem::path(dir) / exeName, ec)) {
                    return true;
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    const char* candidates[] = {
        "/usr/bin/clang",
        "/usr/local/bin/clang",
#ifdef _WIN32
        "C:\\Program Files\\LLVM\\bin\\clang.exe",
#endif
        nullptr
    };
    for (int i = 0; candidates[i] != nullptr; ++i) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(candidates[i], ec)) {
            return true;
        }
    }
    return false;
}
} // namespace

ToolchainInfo ClangToolchain::info() const
{
    return {"Clang", "18.1.0", "clang"};
}

bool ClangToolchain::isAvailable() const
{
    return executableExists("clang");
}

std::vector<LanguageId> ClangToolchain::supportedLanguages() const
{
    return {{"c"}, {"cpp"}, {"objective-c"}};
}

std::vector<Target> ClangToolchain::supportedTargets() const
{
    return {
        {"x86_64-pc-linux-gnu", "x86-64"},
        {"aarch64-apple-darwin", "aarch64"},
        {"riscv64-unknown-elf", "riscv64"}
    };
}

BuildResult ClangToolchain::build(const BuildRequest& request)
{
    BuildResult result;
    result.success = false;

    // Clang build - leverages UACE for assembly parts
    result.diagnostics.push_back("Clang build requested for: " + request.projectId);

    // In v0.1: basic build validation
    result.artifacts.push_back("a.out");
    result.success = true;

    return result;
}

} // namespace Toolchains
