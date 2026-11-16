#pragma once

#include <string>
#include <vector>
#include <memory>

struct FileExtension {
    std::string extension;
};

struct LanguageInfo {
    std::string name;
    std::string id;
    std::string version;
};

class LanguageServer {
public:
    virtual ~LanguageServer() = default;
    virtual void doComplete() = 0;
    virtual void doHover() = 0;
    virtual void doDiagnostics() = 0;
};

class Formatter {
public:
    virtual ~Formatter() = default;
    virtual std::string format(const std::string& code) = 0;
};

class LanguageProvider {
public:
    virtual ~LanguageProvider() = default;
    virtual LanguageInfo info() const = 0;
    virtual std::vector<FileExtension> extensions() const = 0;
    virtual std::unique_ptr<LanguageServer> createLanguageServer() = 0;
    virtual std::unique_ptr<Formatter> createFormatter() = 0;
};
