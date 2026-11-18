#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

namespace Project {

// The open workspace folder: lists, reads, writes and searches its files and runs Git in it.
class ProjectManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasProject READ hasProject NOTIFY projectChanged)
    Q_PROPERTY(QString projectPath READ currentProjectPath NOTIFY projectChanged)
    Q_PROPERTY(QString projectName READ currentProjectName NOTIFY projectChanged)

public:
    explicit ProjectManager(QObject* parent = nullptr);

    bool hasProject() const;
    QString currentProjectPath() const;
    QString currentProjectName() const;

    // Open a folder as the workspace; it is reopened at the next start.
    Q_INVOKABLE bool openProject(const QString& path);
    Q_INVOKABLE void closeProject();
    // Workspace of the previous session, or "" when there was none or it is gone.
    Q_INVOKABLE QString lastProject() const;

    // file:/// URL from a file dialog -> local path.
    Q_INVOKABLE QString localPath(const QUrl& url) const;
    // Files of the workspace (absolute paths), skipping hidden, VCS and build folders.
    Q_INVOKABLE QStringList listFiles() const;
    // File text, or undefined in QML when it is missing, binary or too large to edit.
    Q_INVOKABLE QVariant readFile(const QString& path) const;
    // Atomic write: the old file stays intact if writing fails.
    Q_INVOKABLE bool writeFile(const QString& path, const QString& text);
    // Case-insensitive search across the workspace: [{path, line, text}].
    Q_INVOKABLE QVariantList searchFiles(const QString& query) const;

    Q_INVOKABLE bool isGitRepo() const;
    Q_INVOKABLE QString gitBranch() const;
    // "XY path" lines from git status --porcelain.
    Q_INVOKABLE QStringList gitStatus() const;
    // Stage everything and commit; returns Git's output.
    Q_INVOKABLE QString gitCommit(const QString& message);

signals:
    void projectChanged();

private:
    QString git(const QStringList& args, bool* ok = nullptr) const;

    QString m_root;
};

} // namespace Project
