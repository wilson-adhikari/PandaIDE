#pragma once

#include "api/Toolchain.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class ToolchainDiscovery
{
public:
    // Scan PATH for known toolchains
    static std::map<std::string, std::shared_ptr<Toolchain>> scanPath();

    // Check known installation locations
    static std::map<std::string, std::shared_ptr<Toolchain>> checkKnownLocations();

    // Check user configuration
    static std::map<std::string, std::shared_ptr<Toolchain>> checkUserConfig();

    // Detect all available toolchains on startup
    static std::vector<std::shared_ptr<Toolchain>> detectAll();
};