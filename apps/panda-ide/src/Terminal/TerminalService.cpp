#include "TerminalService.h"

#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

namespace Terminal {

TerminalService::TerminalService(QObject* parent)
    : QObject(parent)
{
    m_runDir = QDir(QDir::temp().filePath(QStringLiteral("pandaide-run")));
    m_runDir.mkpath(QStringLiteral("."));
    m_workDir = m_runDir;

    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalService::onReadyRead);
    connect(&m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalService::onFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &TerminalService::onError);

    m_shell.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_shell, &QProcess::readyReadStandardOutput,
            this, &TerminalService::onShellReadyRead);
    connect(&m_shell,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalService::onShellFinished);

    // Boot the default interactive shell so the terminal is live
    // the moment the IDE opens, like VS Code does.
    const QList<ShellDef> shells = detectShells();
    if (!shells.isEmpty()) {
        startShellProcess(shells.first());
    }
}

TerminalService::~TerminalService()
{
    // Quietly: without this, killing the shell runs onShellFinished, which emits from a dying object and starts a new shell.
    m_shell.disconnect(this);
    m_process.disconnect(this);
    if (m_shell.state() != QProcess::NotRunning) {
        m_shell.kill();
        m_shell.waitForFinished(1000);
    }
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

QList<TerminalService::ShellDef> TerminalService::detectShells()
{
    QList<ShellDef> shells;
    auto have = [](const QString& program) {
        if (program.contains(QLatin1Char('/')) ||
            program.contains(QLatin1Char('\\'))) {
            return QFileInfo::exists(program);
        }
        return !QStandardPaths::findExecutable(program).isEmpty();
    };
#ifndef Q_OS_WIN
    // macOS and Linux: the user's login shell first, then the common ones (read from stdin, no TTY needed).
    QStringList seen;
    for (const QString& sh : {qEnvironmentVariable("SHELL"), QStringLiteral("zsh"), QStringLiteral("bash"), QStringLiteral("sh")}) {
        const QString path = sh.isEmpty() ? QString() : QStandardPaths::findExecutable(sh);
        if (!path.isEmpty() && !seen.contains(path)) {
            seen << path;
            shells << ShellDef{QFileInfo(path).fileName(), path, {}};
        }
    }
#endif
    // Preferred order on Windows: modern PowerShell, classic PowerShell,
    // Command Prompt, then Unix shells when installed.
    if (have(QStringLiteral("pwsh"))) {
        shells << ShellDef{QStringLiteral("pwsh"),
                           QStringLiteral("pwsh"),
                           {QStringLiteral("-NoLogo"),
                            QStringLiteral("-NoProfile")}};
    }
    if (have(QStringLiteral("powershell"))) {
        shells << ShellDef{QStringLiteral("PowerShell"),
                           QStringLiteral("powershell"),
                           {QStringLiteral("-NoLogo"),
                            QStringLiteral("-NoProfile")}};
    }
    if (have(QStringLiteral("cmd"))) {
        shells << ShellDef{QStringLiteral("Command Prompt"),
                           QStringLiteral("cmd"), {}};
    }
    const QString gitBash =
        QStringLiteral("C:/Program Files/Git/bin/bash.exe");
    if (have(gitBash)) {
        shells << ShellDef{QStringLiteral("Git Bash"), gitBash,
                           {QStringLiteral("--login"),
                            QStringLiteral("-i")}};
    }
    const QString msysBash = QStringLiteral("C:/msys64/usr/bin/bash.exe");
    if (have(msysBash)) {
        shells << ShellDef{QStringLiteral("MSYS2 Bash"), msysBash,
                           {QStringLiteral("--login"),
                            QStringLiteral("-i")}};
    }
    return shells;
}

QStringList TerminalService::availableShells() const
{
    QStringList names;
    for (const ShellDef& shell : detectShells()) {
        names << shell.display;
    }
    return names;
}

QString TerminalService::currentShell() const
{
    return m_shellDef.display;
}

void TerminalService::startShellProcess(const ShellDef& def)
{
    if (m_shell.state() != QProcess::NotRunning) {
        m_shellStopping = true;
        m_shell.kill();
        m_shell.waitForFinished(1000);
        m_shellStopping = false;
    }
    m_shellDef = def;
    m_shell.setWorkingDirectory(m_workDir.absolutePath());
    m_shell.setProgram(def.program);
    m_shell.setArguments(def.args);
    m_shell.start();
    emit currentShellChanged(def.display);
}

void TerminalService::startShell(const QString& name)
{
    for (const ShellDef& shell : detectShells()) {
        if (shell.display.compare(name, Qt::CaseInsensitive) == 0) {
            emit outputReceived(
                QStringLiteral("(switching to ") + shell.display +
                QStringLiteral(")"));
            startShellProcess(shell);
            return;
        }
    }
    emit outputReceived(QStringLiteral("error: unknown shell '") + name +
                        QStringLiteral("'"));
}

void TerminalService::writeInput(const QString& text)
{
    if (m_shell.state() == QProcess::NotRunning) {
        const QList<ShellDef> shells = detectShells();
        if (shells.isEmpty()) {
            emit outputReceived(
                QStringLiteral("error: no shell installed on this machine"));
            return;
        }
        startShellProcess(!m_shellDef.display.isEmpty()
                              ? m_shellDef
                              : shells.first());
    }
    m_shell.write((text + QStringLiteral("\n")).toLocal8Bit());
}

QString TerminalService::currentDirectory() const
{
    return QDir::toNativeSeparators(m_workDir.absolutePath());
}

void TerminalService::noteCd(const QString& path)
{
    // Cosmetic directory-label hint for `cd` typed into the shell.
    // The shell itself owns the real cwd; this only updates the label
    // when the target provably exists.
    const QString target = path.trimmed();
    QDir dir = target.isEmpty() ? QDir::home()
                                : QDir(target);
    if (!dir.isAbsolute()) {
        dir = QDir(m_workDir.filePath(target));
    }
    if (dir.exists()) {
        m_workDir = dir;
        emit currentDirectoryChanged(currentDirectory());
    }
}

void TerminalService::stop()
{
    bool stopped = false;
    if (m_shell.state() != QProcess::NotRunning) {
        m_shellStopping = true;
        m_shell.kill();
        stopped = true;
    }
    if (m_process.state() != QProcess::NotRunning) {
        m_steps.clear();
        m_process.kill();
        stopped = true;
    }
    if (stopped) {
        emit outputReceived(QStringLiteral("(stopped)"));
    }
}

QStringList TerminalService::splitCommand(const QString& line)
{
    // Minimal tokenizer: double/single quotes group words, a
    // backslash escapes the next char only inside double quotes
    // (so Windows paths like C:\Qt stay intact everywhere else).
    QStringList parts;
    QString cur;
    bool inSingle = false;
    bool inDouble = false;
    bool hasToken = false;
    const int n = line.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = line.at(i);
        if (inSingle) {
            if (c == QLatin1Char('\'')) {
                inSingle = false;
            } else {
                cur += c;
            }
            hasToken = true;
        } else if (inDouble) {
            if (c == QLatin1Char('"')) {
                inDouble = false;
            } else if (c == QLatin1Char('\\') && i + 1 < n &&
                       (line.at(i + 1) == QLatin1Char('"') ||
                        line.at(i + 1) == QLatin1Char('\\'))) {
                cur += line.at(++i);
            } else {
                cur += c;
            }
            hasToken = true;
        } else if (c == QLatin1Char('\'')) {
            inSingle = true;
            hasToken = true;
        } else if (c == QLatin1Char('"')) {
            inDouble = true;
            hasToken = true;
        } else if (c.isSpace()) {
            if (hasToken) {
                parts << cur;
                cur.clear();
                hasToken = false;
            }
        } else {
            cur += c;
            hasToken = true;
        }
    }
    if (hasToken) {
        parts << cur;
    }
    return parts;
}

QString TerminalService::findProgram(const QStringList& names)
{
    for (const QString& name : names) {
        const QString found = QStandardPaths::findExecutable(name);
        if (!found.isEmpty()) {
            return found;
        }
    }
    return {};
}

void TerminalService::startProgram(const QString& program,
                                   const QStringList& args,
                                   const QDir& dir)
{
    m_process.setWorkingDirectory(dir.absolutePath());
    m_process.setProgram(program);
    m_process.setArguments(args);
    m_process.start();
}

void TerminalService::execute(const QString& commandLine)
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
        emit outputReceived(QStringLiteral("(previous process terminated)"));
    }
    m_steps.clear();

    const QStringList parts = splitCommand(commandLine.trimmed());
    if (parts.isEmpty()) {
        return;
    }
    if (parts.first().compare(QStringLiteral("cd"), Qt::CaseInsensitive) == 0) {
        noteCd(parts.size() > 1 ? parts.at(1) : QString());
        return;
    }
    startProgram(parts.first(), parts.mid(1), m_workDir);
}

void TerminalService::runText(const QString& text,
                              const QString& language,
                              const QString& fileName)
{
    // Never trust the caller with a path: keep only the file name.
    QString safeName = QFileInfo(fileName).fileName();
    if (safeName.isEmpty()) {
        safeName = QStringLiteral("untitled");
    }
    const QString filePath = m_runDir.filePath(safeName);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit outputReceived(QStringLiteral("error: cannot write ") + filePath);
        emit finished(1);
        return;
    }
    {
        QTextStream out(&file);
        out << text;
    }
    runFile(filePath, language);
}

void TerminalService::runFile(const QString& path, const QString& language)
{
    const QFileInfo info(path);
    if (!info.isFile()) {
        emit outputReceived(QStringLiteral("error: file not found: ") + path);
        emit finished(1);
        return;
    }
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
    m_steps.clear();
    m_stepDir = info.dir();
    const QString file = info.absoluteFilePath();
    // Build output goes to the scratch folder, never next to the user's sources.
#ifdef Q_OS_WIN
    const QString exe = m_runDir.filePath(info.completeBaseName() + QStringLiteral(".exe"));
#else
    const QString exe = m_runDir.filePath(info.completeBaseName());
#endif
    auto fail = [this](const QString& why) {
        emit outputReceived(QStringLiteral("error: ") + why);
        emit finished(1);
    };

    const QString lang = language.toLower();
    if (lang == "python" || lang == "py") {
        const QString python = findProgram({"python", "python3"});
        if (python.isEmpty()) {
            return fail(QStringLiteral("no Python interpreter found in PATH"));
        }
        m_steps << Step{python, {file}};
    } else if (lang == "cpp" || lang == "c" || lang == "cc" || lang == "cxx") {
        const QString compiler = findProgram({"g++", "clang++"});
        if (compiler.isEmpty()) {
            return fail(QStringLiteral("no C++ compiler (g++/clang++) found in PATH"));
        }
        m_steps << Step{compiler, {file, QStringLiteral("-o"), exe}} << Step{exe, {}};
    } else if (lang == "rust" || lang == "rs") {
        // Inside a Cargo project, cargo builds the whole crate; a lone file is compiled with rustc.
        const QString crate = projectRootWith(info.dir(), QStringLiteral("Cargo.toml"));
        if (!crate.isEmpty()) {
            const QString cargo = findProgram({"cargo"});
            if (cargo.isEmpty()) {
                return fail(QStringLiteral("cargo not found in PATH. Install Rust from https://rustup.rs"));
            }
            m_stepDir = QDir(crate);
            m_steps << Step{cargo, {QStringLiteral("run"), QStringLiteral("--quiet")}};
        } else {
            const QString rustc = findProgram({"rustc"});
            if (rustc.isEmpty()) {
                return fail(QStringLiteral("rustc not found in PATH. Install Rust from https://rustup.rs"));
            }
            m_steps << Step{rustc, {file, QStringLiteral("-o"), exe}} << Step{exe, {}};
        }
    } else if (lang == "csharp" || lang == "cs") {
        // A .csproj nearby is run as a project; a lone .cs file runs as a .NET 10+ file-based app.
        const QString dotnet = findProgram({"dotnet"});
        if (dotnet.isEmpty()) {
            return fail(QStringLiteral("dotnet not found in PATH. Install the .NET SDK (version 10 or newer) from https://dot.net"));
        }
        const QString project = projectRootWith(info.dir(), QStringLiteral("*.csproj"));
        m_steps << Step{dotnet, project.isEmpty() ? QStringList{QStringLiteral("run"), file}
                                                  : QStringList{QStringLiteral("run"), QStringLiteral("--project"), project}};
    } else if (lang == "asm") {
        // The C compiler links the program (and assembles GNU .s files itself), so `main` is the entry point.
        const QString cc = findProgram({"gcc", "clang", "cc"});
        if (cc.isEmpty()) {
            return fail(QStringLiteral("no C compiler (gcc/clang) found in PATH; it links assembly programs"));
        }
        if (info.suffix().compare(QStringLiteral("s"), Qt::CaseInsensitive) == 0) {
            m_steps << Step{cc, {file, QStringLiteral("-o"), exe}};
        } else {
            const QString nasm = findProgram({"nasm"});
            if (nasm.isEmpty()) {
                return fail(QStringLiteral("NASM not found in PATH. Install it (https://nasm.us, "
                                           "'brew install nasm' or 'sudo apt install nasm'), "
                                           "or save GNU-syntax code as a .s file"));
            }
            const QString obj = exe + QStringLiteral(".o");
#if defined(Q_OS_WIN)
            const QString format = QStringLiteral("win64");
            const QStringList link{obj, QStringLiteral("-o"), exe};
#elif defined(Q_OS_MACOS)
            const QString format = QStringLiteral("macho64");
            // x86-64 code: link it as x86-64 (Apple silicon runs it under Rosetta).
            const QStringList link{QStringLiteral("-arch"), QStringLiteral("x86_64"), obj, QStringLiteral("-o"), exe};
#else
            const QString format = QStringLiteral("elf64");
            // NASM code often uses absolute addresses, which position-independent executables reject.
            const QStringList link{QStringLiteral("-no-pie"), obj, QStringLiteral("-o"), exe};
#endif
            m_steps << Step{nasm, {QStringLiteral("-f"), format, file, QStringLiteral("-o"), obj}}
                    << Step{cc, link};
        }
        m_steps << Step{exe, {}};
    } else {
        return fail(QStringLiteral("Run supports Python, C, C++, Rust, C# and assembly (.asm with NASM, .s with the GNU assembler)"));
    }
    runNextStep();
}

QString TerminalService::projectRootWith(QDir dir, const QString& pattern)
{
    // The nearest folder (this one or up to five above it) that holds a matching project file.
    for (int level = 0; level < 6; ++level) {
        if (!dir.entryList({pattern}, QDir::Files).isEmpty()) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return {};
}

void TerminalService::runNextStep()
{
    const Step step = m_steps.takeFirst();
    startProgram(step.program, step.args, m_stepDir);
}

void TerminalService::setWorkingDirectory(const QString& dir)
{
    const QDir target(dir);
    if (dir.isEmpty() || !target.exists()) {
        return;
    }
    m_workDir = target;
    emit currentDirectoryChanged(currentDirectory());
    if (!m_shellDef.program.isEmpty()) {
        startShellProcess(m_shellDef);
    }
}

void TerminalService::onShellReadyRead()
{
    const QString text =
        QString::fromLocal8Bit(m_shell.readAllStandardOutput());
    if (!text.isEmpty()) {
        emit outputReceived(text);
    }
}

void TerminalService::onShellFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    // Keep the terminal usable like VS Code does: a fresh shell is
    // started automatically (explicit stops stay quiet otherwise).
    if (m_shellStopping) {
        m_shellStopping = false;
    } else {
        emit finished(exitCode);
        emit outputReceived(QStringLiteral("(shell exited, starting a new one)"));
    }
    const QList<ShellDef> shells = detectShells();
    if (!shells.isEmpty()) {
        ShellDef next = shells.first();
        for (const ShellDef& shell : shells) {
            if (shell.display == m_shellDef.display) {
                next = shell;
                break;
            }
        }
        startShellProcess(next);
    }
}

void TerminalService::onReadyRead()
{
    const QString text =
        QString::fromLocal8Bit(m_process.readAllStandardOutput());
    if (!text.isEmpty()) {
        emit outputReceived(text);
    }
}

void TerminalService::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (exitCode == 0 && status == QProcess::NormalExit && !m_steps.isEmpty()) {
        runNextStep();
        return;
    }
    m_steps.clear();
    emit finished(exitCode);
}

void TerminalService::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        m_steps.clear();
        emit outputReceived(
            QStringLiteral("error: cannot start process (not found or not executable)"));
        emit finished(1);
    }
}

} // namespace Terminal
