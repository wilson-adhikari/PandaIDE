#include "ProjectManager.h"
#include <QDateTime>
#include <QFileInfo>
#include <QVariant>

namespace Project {

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
    , m_hasProject(false)
    , m_currentProjectPath()
    , m_currentProjectName()
{
}

ProjectManager::~ProjectManager()
{
}

bool ProjectManager::hasProject() const
{
    return m_hasProject;
}

QString ProjectManager::currentProjectPath() const
{
    return m_currentProjectPath;
}

QString ProjectManager::currentProjectName() const
{
    return m_currentProjectName;
}

bool ProjectManager::newProject(const QString& name, const QString& path)
{
    m_currentProjectName = name;
    m_currentProjectPath = path;
    m_hasProject = true;
    m_projectData.clear();
    m_projectData["name"] = name;
    m_projectData["path"] = path;
    m_projectData["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit hasProjectChanged(true);
    emit projectOpened(path);
    return true;
}

bool ProjectManager::openProject(const QString& path)
{
    m_currentProjectPath = path;
    m_currentProjectName = QFileInfo(path).baseName();
    m_hasProject = true;
    m_projectData["path"] = path;
    m_projectData["opened"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit hasProjectChanged(true);
    emit projectOpened(path);
    return true;
}

bool ProjectManager::saveProject()
{
    if (!m_hasProject) return false;
    
    m_projectData["saved"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    emit projectSaved();
    return true;
}

bool ProjectManager::closeProject()
{
    if (!m_hasProject) return true;
    
    m_hasProject = false;
    m_currentProjectPath.clear();
    m_currentProjectName.clear();
    m_projectData.clear();
    
    emit hasProjectChanged(false);
    emit projectClosed();
    return true;
}

} // namespace Project