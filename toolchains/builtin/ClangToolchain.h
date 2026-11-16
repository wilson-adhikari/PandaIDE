#pragma once

#include "api/Toolchain.h"

namespace Toolchains {

class ClangToolchain : public Toolchain
{
public:
    ToolchainInfo info() const override;
    bool isAvailable() const override;
    std::vector<LanguageId> supportedLanguages() const override;
    std::vector<Target> supportedTargets() const override;
    BuildResult build(const BuildRequest& request) override;
};

} // namespace Toolchains
