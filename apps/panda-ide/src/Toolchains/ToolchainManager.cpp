#include "ToolchainManager.h"
#include <QMap>

namespace Toolchains {

ToolchainManager::ToolchainManager(QObject* parent)
    : QObject(parent)
{
}

ToolchainManager::~ToolchainManager()
{
}

bool ToolchainManager::hasToolchains() const
{
    return !m_toolchains.isEmpty();
}

QStringList ToolchainManager::getAvailableToolchains() const
{
    return m_toolchains.keys();
}

QString ToolchainManager::currentToolchain() const
{
    return m_currentToolchain;
}

void ToolchainManager::setCurrentToolchain(const QString& toolchain)
{
    if (m_currentToolchain != toolchain && m_toolchains.contains(toolchain)) {
        m_currentToolchain = toolchain;
        emit currentToolchainChanged(toolchain);
    }
}

void ToolchainManager::addToolchain(const QString& name, const QString& path)
{
    m_toolchains[name] = path;
}

void ToolchainManager::removeToolchain(const QString& name)
{
    if (m_toolchains.remove(name) && m_currentToolchain == name) {
        m_currentToolchain.clear();
    }
}

} // namespace Toolchains