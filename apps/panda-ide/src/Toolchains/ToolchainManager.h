#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace Toolchains {

class ToolchainManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasToolchains READ hasToolchains NOTIFY toolchainsChanged)
    Q_PROPERTY(QStringList availableToolchains READ getAvailableToolchains NOTIFY toolchainsChanged)
    Q_PROPERTY(QString currentToolchain READ currentToolchain NOTIFY currentToolchainChanged)

public:
    explicit ToolchainManager(QObject* parent = nullptr);
    ~ToolchainManager();

    bool hasToolchains() const;
    QStringList getAvailableToolchains() const;
    QString currentToolchain() const;

    Q_INVOKABLE void setCurrentToolchain(const QString& toolchain);
    Q_INVOKABLE void addToolchain(const QString& name, const QString& path);
    Q_INVOKABLE void removeToolchain(const QString& name);

signals:
    void toolchainsChanged();
    void currentToolchainChanged(const QString& toolchain);

private:
    QMap<QString, QString> m_toolchains;
    QString m_currentToolchain;
};

} // namespace Toolchains