#pragma once

#include <QDir>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace Terminal {

// Integrated terminal backend: a persistent interactive shell
// (PowerShell / pwsh / CMD / Git Bash / MSYS2 Bash - whichever are
// installed), exactly like VS Code's terminal panel. Typed input is
// written straight into the live shell; its output streams back.
//
// A second, one-shot runner handles the Run button and plain
// `execute()` calls so ad-hoc commands never disturb the shell.
// No command shell is ever spawned for parsing: command lines are
// tokenized here, so metacharacters like && || ; | travel as plain
// arguments rather than being interpreted.
class TerminalService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentDirectory READ currentDirectory NOTIFY currentDirectoryChanged)
    Q_PROPERTY(QString currentShell READ currentShell NOTIFY currentShellChanged)

public:
    explicit TerminalService(QObject* parent = nullptr);
    ~TerminalService() override;

    // Shells installed on this machine, e.g. {"PowerShell", "Command Prompt"}.
    Q_INVOKABLE QStringList availableShells() const;
    // Switch the interactive shell (restarts it). Unknown names are ignored.
    Q_INVOKABLE void startShell(const QString& name);
    // Type text into the live shell (a newline is appended).
    Q_INVOKABLE void writeInput(const QString& text);

    // Run one command line as a single process, e.g. "python main.py".
    // A running one-shot process is terminated first.
    Q_INVOKABLE void execute(const QString& commandLine);

    // Write an editor buffer to the run scratch directory and execute
    // it with the right tool for its language (python/g++). Used by
    // the Run button so "Run" really runs the current file.
    Q_INVOKABLE void runText(const QString& text,
                             const QString& language,
                             const QString& fileName);

    Q_INVOKABLE void stop();

    // Hint for the directory label after the user runs `cd` in the
    // shell. Only applied when the path exists locally.
    Q_INVOKABLE void noteCd(const QString& path);

    QString currentDirectory() const;
    QString currentShell() const;

signals:
    void outputReceived(const QString& text);
    void finished(int exitCode);
    void currentDirectoryChanged(const QString& dir);
    void currentShellChanged(const QString& name);

private slots:
    void onShellReadyRead();
    void onShellFinished(int exitCode, QProcess::ExitStatus status);
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onError(QProcess::ProcessError error);

private:
    struct ShellDef {
        QString display;
        QString program;
        QStringList args;
    };
    static QList<ShellDef> detectShells();
    void startShellProcess(const ShellDef& def);
    void startProgram(const QString& program, const QStringList& args);
    static QStringList splitCommand(const QString& line);
    static QString findProgram(const QStringList& names);

    QProcess m_shell;
    QProcess m_process;
    ShellDef m_shellDef;
    bool m_shellStopping = false;
    QDir m_workDir;
    QDir m_runDir;
    bool m_compileStep = false;
    QString m_runExe;
};

} // namespace Terminal
