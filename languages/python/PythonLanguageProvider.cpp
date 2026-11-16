#include "PythonLanguageProvider.h"

#include <memory>

namespace Languages {

LanguageInfo PythonLanguageProvider::info() const
{
    return {"Python", "python3", "3.14"};
}

std::vector<FileExtension> PythonLanguageProvider::extensions() const
{
    return {{".py"}, {".pyw"}};
}

std::unique_ptr<LanguageServer> PythonLanguageProvider::createLanguageServer()
{
    return std::make_unique<PythonLanguageServer>();
}

std::unique_ptr<Formatter> PythonLanguageProvider::createFormatter()
{
    return std::make_unique<PythonLanguageFormatter>();
}

std::string PythonLanguageFormatter::format(const std::string& code)
{
    // v0.1: identity formatter (a Ruff integration would run here).
    return code;
}

} // namespace Languages
