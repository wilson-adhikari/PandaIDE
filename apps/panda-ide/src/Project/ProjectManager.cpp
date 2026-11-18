#include "ProjectManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

namespace Project {

namespace {
const char* kLastKey = "workspace/last";
constexpr qint64 kMaxEditBytes = 4 * 1024 * 1024;   // larger files would freeze the editor
constexpr qint64 kMaxSearchBytes = 1024 * 1024;
constexpr int kMaxFiles = 5000;                       // shortcut: flat capped list; a lazy tree model is needed for huge repos
constexpr int kMaxHits = 300;

bool isBinary(const QByteArray& head)
{
    return head.contains('\0');
}
} // namespace

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
{
}

bool ProjectManager::hasProject() const { return !m_root.isEmpty(); }
QString ProjectManager::currentProjectPath() const { return m_root; }
QString ProjectManager::currentProjectName() const { return QFileInfo(m_root).fileName(); }

bool ProjectManager::openProject(const QString& path)
{
    const QFileInfo info(path);
    if (path.isEmpty() || !info.isDir()) {
        return false;
    }
    m_root = QDir::cleanPath(info.absoluteFilePath());
    QSettings().setValue(kLastKey, m_root);
    emit projectChanged();
    return true;
}

void ProjectManager::closeProject()
{
    m_root.clear();
    QSettings().remove(kLastKey);
    emit projectChanged();
}

QString ProjectManager::lastProject() const
{
    const QString last = QSettings().value(kLastKey).toString();
    return QFileInfo(last).isDir() ? last : QString();
}

QString ProjectManager::localPath(const QUrl& url) const
{
    return url.isLocalFile() ? QDir::cleanPath(url.toLocalFile()) : url.toString();
}

QStringList ProjectManager::listFiles() const
{
    static const QSet<QString> skip{QStringLiteral("node_modules"), QStringLiteral("build"),
                                    QStringLiteral("out"), QStringLiteral("dist"),
                                    QStringLiteral("__pycache__"), QStringLiteral("venv")};
    QStringList files;
    if (m_root.isEmpty()) {
        return files;
    }
    QStringList pending{m_root};
    while (!pending.isEmpty() && files.size() < kMaxFiles) {
        const QDir dir(pending.takeFirst());
        const QFileInfoList entries =
            dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsLast);
        for (const QFileInfo& e : entries) {
            if (e.fileName().startsWith(QLatin1Char('.'))) {
                continue; // .git, .venv, .vscode and other hidden entries
            }
            if (e.isDir()) {
                if (!skip.contains(e.fileName()) && !e.isSymLink()) {
                    pending << e.absoluteFilePath();
                }
            } else if (files.size() < kMaxFiles) {
                files << e.absoluteFilePath();
            }
        }
    }
    files.sort(Qt::CaseInsensitive);
    return files;
}

QVariant ProjectManager::readFile(const QString& path) const
{
    QFile file(path);
    if (file.size() > kMaxEditBytes || !file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray bytes = file.readAll();
    if (isBinary(bytes.left(8192))) {
        return {};
    }
    return QString::fromUtf8(bytes);
}

bool ProjectManager::writeFile(const QString& path, const QString& text)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(text.toUtf8());
    return file.commit();
}

QVariantList ProjectManager::searchFiles(const QString& query) const
{
    QVariantList hits;
    const QString q = query.trimmed();
    if (q.isEmpty()) {
        return hits;
    }
    for (const QString& path : listFiles()) {
        QFile file(path);
        if (file.size() > kMaxSearchBytes || !file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray bytes = file.readAll();
        if (isBinary(bytes.left(8192))) {
            continue;
        }
        const QStringList lines = QString::fromUtf8(bytes).split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            if (lines.at(i).contains(q, Qt::CaseInsensitive)) {
                hits << QVariantMap{{QStringLiteral("path"), path},
                                    {QStringLiteral("line"), i + 1},
                                    {QStringLiteral("text"), lines.at(i).trimmed().left(200)}};
                if (hits.size() >= kMaxHits) {
                    return hits;
                }
            }
        }
    }
    return hits;
}

QString ProjectManager::git(const QStringList& args, bool* ok) const
{
    if (ok) {
        *ok = false;
    }
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (exe.isEmpty() || m_root.isEmpty()) {
        return exe.isEmpty() ? QStringLiteral("Git is not installed (https://git-scm.com).") : QString();
    }
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, QStringList{QStringLiteral("-C"), m_root} + args);
    // shortcut: runs on the UI thread with a timeout; fine for normal repos, move to a worker for huge ones.
    if (!p.waitForFinished(15000)) {
        p.kill();
        return QStringLiteral("git timed out");
    }
    if (ok) {
        *ok = p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    }
    return QString::fromLocal8Bit(p.readAll()).trimmed();
}

bool ProjectManager::isGitRepo() const
{
    bool ok = false;
    return git({QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")}, &ok) == QStringLiteral("true") && ok;
}

QString ProjectManager::gitBranch() const
{
    bool ok = false;
    const QString branch = git({QStringLiteral("branch"), QStringLiteral("--show-current")}, &ok);
    return ok ? branch : QString();
}

QStringList ProjectManager::gitStatus() const
{
    bool ok = false;
    const QString out = git({QStringLiteral("status"), QStringLiteral("--porcelain=v1")}, &ok);
    return ok && !out.isEmpty() ? out.split(QLatin1Char('\n')) : QStringList();
}

QString ProjectManager::gitCommit(const QString& message)
{
    if (message.trimmed().isEmpty()) {
        return tr("Write a commit message first.");
    }
    bool ok = false;
    const QString added = git({QStringLiteral("add"), QStringLiteral("-A")}, &ok);
    if (!ok) {
        return added;
    }
    return git({QStringLiteral("commit"), QStringLiteral("-m"), message.trimmed()});
}

} // namespace Project
