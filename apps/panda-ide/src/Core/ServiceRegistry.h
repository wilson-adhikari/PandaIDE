#pragma once

#include <QObject>
#include <QString>
#include <QMap>

namespace Core {

// Central registry that owns the IDE's core services for the
// lifetime of the application. Services are parented to the
// registry where possible; anything still registered at shutdown
// is deleted here.
class ServiceRegistry : public QObject
{
    Q_OBJECT

public:
    static ServiceRegistry* instance();
    ~ServiceRegistry() override;

    template<typename T>
    void registerService(const QString& name, T* service)
    {
        QObject* obj = static_cast<QObject*>(service);
        if (QObject* old = m_services.value(name, nullptr)) {
            if (old != obj) {
                delete old;
            }
        }
        m_services.insert(name, obj);
    }

    template<typename T>
    T* getService(const QString& name) const
    {
        return qobject_cast<T*>(m_services.value(name, nullptr));
    }

    bool hasService(const QString& name) const;
    void unregisterService(const QString& name);
    void clear();

private:
    explicit ServiceRegistry(QObject* parent = nullptr);

    static ServiceRegistry* s_instance;
    QMap<QString, QObject*> m_services;
};

} // namespace Core
