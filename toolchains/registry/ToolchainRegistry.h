#pragma once

#include "api/Toolchain.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class ToolchainRegistry
{
public:
    // Register a toolchain
    static void registerToolchain(const std::string& name, std::shared_ptr<Toolchain> toolchain);

    // Get all registered toolchains
    static std::vector<std::shared_ptr<Toolchain>> getAll();

    // Get toolchain by name
    static std::shared_ptr<Toolchain> getByName(const std::string& name);

    // Get toolchain supporting a language
    static std::shared_ptr<Toolchain> getSupportedByLanguage(const std::string& languageId);

    // Get toolchain for target
    static std::shared_ptr<Toolchain> getForTarget(const std::string& targetTriple);

    // Check if any toolchain is available for language
    static bool hasAvailable(const std::string& languageId);

    // Check if any toolchain is available for target
    static bool hasAvailableForTarget(const std::string& targetTriple);
};