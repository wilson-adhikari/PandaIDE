#include "UACE.h"

namespace UACE {

UACE::UACE(QObject* parent)
    : QObject(parent)
    , m_version("0.1")
    , m_architectures({"x86-64", "x86", "arm", "arm64", "riscv"})
{
}

UACE::~UACE()
{
}

QString UACE::version() const
{
    return m_version;
}

QStringList UACE::supportedArchitectures() const
{
    return m_architectures;
}

bool UACE::compile(const QString& source, const QString& architecture)
{
    Q_UNUSED(source)
    Q_UNUSED(architecture)
    emit compilationStarted("memory");
    // TODO: Implement actual compilation
    emit compilationFinished(false, "Not implemented yet");
    return false;
}

QString UACE::disassemble(const QByteArray& binary, const QString& architecture)
{
    Q_UNUSED(binary)
    Q_UNUSED(architecture)
    return "; Disassembly not implemented yet";
}

QByteArray UACE::assemble(const QString& source, const QString& architecture)
{
    Q_UNUSED(source)
    Q_UNUSED(architecture)
    return QByteArray();
}

} // namespace UACE