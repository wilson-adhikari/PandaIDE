#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPalette>

#include "Core/ServiceRegistry.h"
#include "Project/ProjectManager.h"
#include "Languages/LanguageProvider.h"
#include "Toolchains/ToolchainManager.h"
#include "UACE/UACE.h"
#include "Completion/CompletionEngine.h"
#include "Terminal/TerminalService.h"

namespace {

// Fusion + a dark palette so every native control (menus,
// scrollbars, buttons, tabs) renders dark like the QML theme.
void applyDarkPalette(QApplication& app)
{
    QApplication::setStyle(QStringLiteral("Fusion"));
    QPalette p;
    p.setColor(QPalette::Window, QColor("#23252a"));
    p.setColor(QPalette::WindowText, QColor("#bcbec4"));
    p.setColor(QPalette::Base, QColor("#1e1f22"));
    p.setColor(QPalette::AlternateBase, QColor("#26282d"));
    p.setColor(QPalette::Text, QColor("#bcbec4"));
    p.setColor(QPalette::Button, QColor("#2e3138"));
    p.setColor(QPalette::ButtonText, QColor("#bcbec4"));
    p.setColor(QPalette::BrightText, QColor("#ff6b68"));
    p.setColor(QPalette::Highlight, QColor("#3574f0"));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::Link, QColor("#4eade5"));
    p.setColor(QPalette::ToolTipBase, QColor("#2e3138"));
    p.setColor(QPalette::ToolTipText, QColor("#bcbec4"));
    app.setPalette(p);
}

} // namespace

int main(int argc, char *argv[])
{
    // Note: Qt6 enables high-DPI scaling by default; no opt-in attribute needed.
    QApplication::setOrganizationName("PandaIDE");
    QApplication::setApplicationName("PandaIDE");
    QApplication::setApplicationVersion("0.1");

    QApplication app(argc, argv);
    applyDarkPalette(app);

    // Initialize service registry and register core services.
    // Services are parented to the registry so they are cleaned up on exit.
    Core::ServiceRegistry* registry = Core::ServiceRegistry::instance();
    auto* projectManager = new Project::ProjectManager(registry);
    auto* languageProvider = new Languages::LanguageProvider(registry);
    auto* toolchainManager = new Toolchains::ToolchainManager(registry);
    auto* uace = new UACE::UACE(registry);
    registry->registerService("projectManager", projectManager);
    registry->registerService("languageProvider", languageProvider);
    registry->registerService("toolchainManager", toolchainManager);
    registry->registerService("uace", uace);
    // Word-based code suggestions (not AI): keyword tables plus
    // identifiers from the open document, with Python-aware indent.
    auto* completionEngine = new Completion::CompletionEngine(registry);
    registry->registerService("completionEngine", completionEngine);
    // Real terminal backend (QProcess, no shell wrapper).
    auto* terminalService = new Terminal::TerminalService(registry);
    registry->registerService("terminalService", terminalService);

    // Load QML interface. Each service is exposed both through the
    // "ide" registry object and directly under its own name so the
    // QML in src/qml/main.qml can bind to it.
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("ide", registry);
    engine.rootContext()->setContextProperty("projectManager", projectManager);
    engine.rootContext()->setContextProperty("languageProvider", languageProvider);
    engine.rootContext()->setContextProperty("toolchainManager", toolchainManager);
    engine.rootContext()->setContextProperty("uace", uace);
    engine.rootContext()->setContextProperty("completionEngine", completionEngine);
    engine.rootContext()->setContextProperty("terminalService", terminalService);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
