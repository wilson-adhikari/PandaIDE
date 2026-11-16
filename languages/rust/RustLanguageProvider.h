#pragma once

#include "languages/api/LanguageProvider.h"
#include "languages/api/LanguageInfo.h"

namespace Languages {

class RustLanguageProvider : public LanguageProvider
{
public:
    LanguageInfo info() const override;
    std::vector<FileExtension> extensions() const override;
    std::unique_ptr<LanguageServer> createLanguageServer() override;
    std::unique_ptr<Formatter> createFormatter() override;
};

} // namespace Languages