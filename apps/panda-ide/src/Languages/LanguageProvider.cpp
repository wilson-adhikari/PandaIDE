#include "LanguageProvider.h"

namespace Languages {

LanguageProvider::LanguageProvider(QObject* parent)
    : QObject(parent)
    , m_cppActive(false)
    , m_pythonActive(false)
    , m_assemblyActive(false)
{
}

LanguageProvider::~LanguageProvider()
{
}

bool LanguageProvider::isCppActive() const
{
    return m_cppActive;
}

bool LanguageProvider::isPythonActive() const
{
    return m_pythonActive;
}

bool LanguageProvider::isAssemblyActive() const
{
    return m_assemblyActive;
}

QStringList LanguageProvider::getActiveLanguages() const
{
    QStringList languages;
    if (m_cppActive) languages << "cpp";
    if (m_pythonActive) languages << "python";
    if (m_assemblyActive) languages << "assembly";
    return languages;
}

QString LanguageProvider::getActiveLanguageDisplay() const
{
    QStringList languages = getActiveLanguages();
    if (languages.isEmpty()) {
        return "None";
    }
    return languages.join(", ");
}

void LanguageProvider::setCppActive(bool active)
{
    if (m_cppActive != active) {
        m_cppActive = active;
        emit cppActiveChanged(active);
        emit activeLanguagesChanged(getActiveLanguages());
        emit activeLanguageDisplayChanged(getActiveLanguageDisplay());
    }
}

void LanguageProvider::setPythonActive(bool active)
{
    if (m_pythonActive != active) {
        m_pythonActive = active;
        emit pythonActiveChanged(active);
        emit activeLanguagesChanged(getActiveLanguages());
        emit activeLanguageDisplayChanged(getActiveLanguageDisplay());
    }
}

void LanguageProvider::setAssemblyActive(bool active)
{
    if (m_assemblyActive != active) {
        m_assemblyActive = active;
        emit assemblyActiveChanged(active);
        emit activeLanguagesChanged(getActiveLanguages());
        emit activeLanguageDisplayChanged(getActiveLanguageDisplay());
    }
}

void LanguageProvider::toggleLanguage(const QString& language)
{
    if (language == "cpp") {
        setCppActive(!m_cppActive);
    } else if (language == "python") {
        setPythonActive(!m_pythonActive);
    } else if (language == "assembly") {
        setAssemblyActive(!m_assemblyActive);
    }
}

} // namespace Languages