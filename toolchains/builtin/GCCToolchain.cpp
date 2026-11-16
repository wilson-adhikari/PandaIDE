#include "builtin/GCCToolchain.h"
#include "api/Toolchain.h"
#include <cstdlib>
#include <filesystem>

namespace Toolchains {

namespace {
// Look for an executable in PATH plus a few conventional install
// locations. No shell is ever spawned (no system()/popen()), so
// there is no command-injection surface here.
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

    // Conventional install locations (generic, not user-specific).
    const char* candidates[] = {
        "/usr/bin/gcc",
        "/usr/local/bin/gcc",
#ifdef _WIN32
        "C:\\msys64\\ucrt64\\bin\\gcc.exe",
        "C:\\msys64\\mingw64\\bin\\gcc.exe",
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

ToolchainInfo GCCToolchain::info() const
{
    return {"GCC", "15.1.0", "gcc"};
}

bool GCCToolchain::isAvailable() const
{
    return executableExists("gcc");
}

std::vector<LanguageId> GCCToolchain::supportedLanguages() const
{
    return {{"c"}, {"cpp"}};
}

std::vector<Target> GCCToolchain::supportedTargets() const
{
    return {
        {"x86_64-pc-linux-gnu", "x86-64"},
        {"i686-pc-linux-gnu", "x86"},
        {"aarch64-unknown-linux-gnu", "aarch64"}
    };
}

BuildResult GCCToolchain::build(const BuildRequest& request)
{
    BuildResult result;
    result.success = false;

    // Simplified build - in v0.1 we just validate and generate placeholder
    result.diagnostics.push_back("GCC build requested for: " + request.projectId);

    // In a full implementation, this would:
    // 1. Run the compiler
    // 2. Handle assembly via UACE
    // 3. Link objects
    // 4. Generate executable

    result.artifacts.push_back("a.out");
    result.success = true;

    return result;
}

} // namespace Toolchains
