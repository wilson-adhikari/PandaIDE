#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

namespace UACE {

class UACE : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QStringList supportedArchitectures READ supportedArchitectures CONSTANT)

public:
    explicit UACE(QObject* parent = nullptr);
    ~UACE();

    QString version() const;
    QStringList supportedArchitectures() const;

    Q_INVOKABLE bool compile(const QString& source, const QString& architecture);
    Q_INVOKABLE QString disassemble(const QByteArray& binary, const QString& architecture);
    Q_INVOKABLE QByteArray assemble(const QString& source, const QString& architecture);

signals:
    void compilationStarted(const QString& file);
    void compilationFinished(bool success, const QString& output);
    void errorOccurred(const QString& message);

private:
    QString m_version;
    QStringList m_architectures;
};

} // namespace UACE