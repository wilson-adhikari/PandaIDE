#include "CLanguageProvider.h"

#include <memory>

namespace Languages {

LanguageInfo CLanguageProvider::info() const
{
    return {"C", "c", "C11"};
}

std::vector<FileExtension> CLanguageProvider::extensions() const
{
    return {{".c"}, {".h"}};
}

std::unique_ptr<LanguageServer> CLanguageProvider::createLanguageServer()
{
    return std::make_unique<CLanguageServer>();
}

std::unique_ptr<Formatter> CLanguageProvider::createFormatter()
{
    return std::make_unique<CLanguageFormatter>();
}

std::string CLanguageFormatter::format(const std::string& code)
{
    // v0.1: identity formatter (a real clang-format integration
    // would run here).
    return code;
}

} // namespace Languages
