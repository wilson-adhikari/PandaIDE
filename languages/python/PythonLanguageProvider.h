#pragma once

#include "api/LanguageProvider.h"

#include <memory>
#include <string>
#include <vector>

namespace Languages {

class PythonLanguageServer : public LanguageServer
{
public:
    void doComplete() override {}
    void doHover() override {}
    void doDiagnostics() override {}
};

class PythonLanguageFormatter : public Formatter
{
public:
    std::string format(const std::string& code) override;
};

class PythonLanguageProvider : public LanguageProvider
{
public:
    LanguageInfo info() const override;
    std::vector<FileExtension> extensions() const override;
    std::unique_ptr<LanguageServer> createLanguageServer() override;
    std::unique_ptr<Formatter> createFormatter() override;
};

} // namespace Languages
