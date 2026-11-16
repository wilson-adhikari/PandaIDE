#include "ServiceRegistry.h"

namespace Core {

ServiceRegistry* ServiceRegistry::s_instance = nullptr;

ServiceRegistry::ServiceRegistry(QObject* parent)
    : QObject(parent)
{
}

ServiceRegistry::~ServiceRegistry()
{
    clear();
}

ServiceRegistry* ServiceRegistry::instance()
{
    if (!s_instance) {
        s_instance = new ServiceRegistry();
    }
    return s_instance;
}

bool ServiceRegistry::hasService(const QString& name) const
{
    return m_services.contains(name);
}

void ServiceRegistry::unregisterService(const QString& name)
{
    m_services.remove(name);
}

void ServiceRegistry::clear()
{
    // Delete owned service objects, then drop the entries.
    // values() gives us a stable QList<QObject*> copy to iterate.
    qDeleteAll(m_services.values());
    m_services.clear();
}

} // namespace Core
