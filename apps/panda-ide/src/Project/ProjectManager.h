#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>

namespace Project {

class ProjectManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasProject READ hasProject NOTIFY hasProjectChanged)

public:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager();

    bool hasProject() const;
    QString currentProjectPath() const;
    QString currentProjectName() const;

    Q_INVOKABLE bool newProject(const QString& name, const QString& path);
    Q_INVOKABLE bool openProject(const QString& path);
    Q_INVOKABLE bool saveProject();
    Q_INVOKABLE bool closeProject();

signals:
    void hasProjectChanged(bool hasProject);
    void projectOpened(const QString& path);
    void projectClosed();
    void projectSaved();

private:
    bool m_hasProject;
    QString m_currentProjectPath;
    QString m_currentProjectName;
    QMap<QString, QVariant> m_projectData;
};

} // namespace Project