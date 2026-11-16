#pragma once

#include <string>
#include <vector>

struct LanguageId {
    std::string id;
};

struct Target {
    std::string triple;
    std::string architecture;
};

struct BuildRequest {
    std::string projectId;
    std::string target;
    std::string configuration;
};

struct BuildResult {
    bool success;
    std::vector<std::string> artifacts;
    std::vector<std::string> diagnostics;
};

struct ToolchainInfo {
    std::string name;
    std::string version;
    std::string path;
};

class Toolchain {
public:
    virtual ~Toolchain() = default;

    virtual ToolchainInfo info() const = 0;
    virtual bool isAvailable() const = 0;
    virtual std::vector<LanguageId> supportedLanguages() const = 0;
    virtual std::vector<Target> supportedTargets() const = 0;
    virtual BuildResult build(const BuildRequest& request) = 0;
};
