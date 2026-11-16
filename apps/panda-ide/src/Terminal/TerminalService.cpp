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
    // Preferred order: modern PowerShell, classic PowerShell,
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
        m_compileStep = false;
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
                                   const QStringList& args)
{
    m_process.setWorkingDirectory(m_workDir.absolutePath());
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
    m_compileStep = false;

    const QStringList parts = splitCommand(commandLine.trimmed());
    if (parts.isEmpty()) {
        return;
    }
    if (parts.first().compare(QStringLiteral("cd"), Qt::CaseInsensitive) == 0) {
        noteCd(parts.size() > 1 ? parts.at(1) : QString());
        return;
    }
    startProgram(parts.first(), parts.mid(1));
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

    const QString lang = language.toLower();
    if (lang == "python" || lang == "py") {
        const QString python = findProgram({"python", "python3"});
        if (python.isEmpty()) {
            emit outputReceived(
                QStringLiteral("error: no Python interpreter found in PATH"));
            emit finished(1);
            return;
        }
        startProgram(python, {filePath});
        return;
    }
    if (lang == "cpp" || lang == "c" || lang == "cc" || lang == "cxx") {
        const QString compiler = findProgram({"g++", "clang++"});
        if (compiler.isEmpty()) {
            emit outputReceived(
                QStringLiteral("error: no C++ compiler (g++/clang++) found in PATH"));
            emit finished(1);
            return;
        }
#ifdef Q_OS_WIN
        const QString exe = m_runDir.filePath(
            QFileInfo(safeName).completeBaseName() + QStringLiteral(".exe"));
#else
        const QString exe = m_runDir.filePath(
            QFileInfo(safeName).completeBaseName());
#endif
        m_compileStep = true;
        m_runExe = exe;
        startProgram(compiler, {filePath, QStringLiteral("-o"), exe});
        return;
    }
    emit outputReceived(
        QStringLiteral("Assembly runs through the UACE build pipeline; "
                       "use Build for .asm files (demo)."));
    emit finished(0);
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

void TerminalService::onFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    if (m_compileStep) {
        m_compileStep = false;
        if (exitCode == 0) {
            startProgram(m_runExe, {});
            return;
        }
    }
    emit finished(exitCode);
}

void TerminalService::onError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        emit outputReceived(
            QStringLiteral("error: cannot start process (not found or not executable)"));
        emit finished(1);
    }
}

} // namespace Terminal
