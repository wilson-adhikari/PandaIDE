#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace Languages {

class LanguageProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool cppActive READ isCppActive NOTIFY cppActiveChanged)
    Q_PROPERTY(bool pythonActive READ isPythonActive NOTIFY pythonActiveChanged)
    Q_PROPERTY(bool assemblyActive READ isAssemblyActive NOTIFY assemblyActiveChanged)
    Q_PROPERTY(QStringList activeLanguages READ getActiveLanguages NOTIFY activeLanguagesChanged)
    Q_PROPERTY(QString activeLanguageDisplay READ getActiveLanguageDisplay NOTIFY activeLanguageDisplayChanged)

public:
    explicit LanguageProvider(QObject* parent = nullptr);
    ~LanguageProvider();

    bool isCppActive() const;
    bool isPythonActive() const;
    bool isAssemblyActive() const;
    QStringList getActiveLanguages() const;
    QString getActiveLanguageDisplay() const;

    Q_INVOKABLE void setCppActive(bool active);
    Q_INVOKABLE void setPythonActive(bool active);
    Q_INVOKABLE void setAssemblyActive(bool active);
    Q_INVOKABLE void toggleLanguage(const QString& language);

signals:
    void cppActiveChanged(bool active);
    void pythonActiveChanged(bool active);
    void assemblyActiveChanged(bool active);
    void activeLanguagesChanged(const QStringList& languages);
    void activeLanguageDisplayChanged(const QString& display);

private:
    bool m_cppActive;
    bool m_pythonActive;
    bool m_assemblyActive;
};

} // namespace Languages