// Tests for the workspace (files, search) and the Run pipeline. Run: ctest --test-dir <build> --output-on-failure
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QtTest>

#include "Project/ProjectManager.h"
#include "Terminal/TerminalService.h"

namespace {
bool write(const QString& path, const QByteArray& bytes)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}
bool have(const QStringList& programs)
{
    for (const QString& p : programs) {
        if (!QStandardPaths::findExecutable(p).isEmpty()) {
            return true;
        }
    }
    return false;
}
} // namespace

class WorkspaceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Keep the test's remembered workspace away from the real settings.
        QCoreApplication::setOrganizationName(QStringLiteral("PandaIDE-tests"));
        QCoreApplication::setApplicationName(QStringLiteral("workspace_test"));
    }

    void listsReadsWritesAndSearches()
    {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QVERIFY(write(root + "/main.py", "print('hello')\n"));
        QVERIFY(write(root + "/src/util.c", "int add(int a, int b) { return a + b; }\n"));
        QVERIFY(write(root + "/node_modules/pkg/index.js", "x"));
        QVERIFY(write(root + "/.secret/key.txt", "x"));
        QVERIFY(write(root + "/logo.bin", QByteArray("PNG\0\0data", 9)));

        Project::ProjectManager pm;
        QVERIFY(!pm.openProject(root + "/missing"));
        QVERIFY(pm.openProject(root));
        QCOMPARE(pm.lastProject(), QDir::cleanPath(root));

        const QStringList files = pm.listFiles();
        QVERIFY(files.contains(QDir::cleanPath(root + "/main.py")));
        QVERIFY(files.contains(QDir::cleanPath(root + "/src/util.c")));
        for (const QString& f : files) {
            QVERIFY2(!f.contains("node_modules") && !f.contains(".secret"), qPrintable(f));
        }

        QCOMPARE(pm.readFile(root + "/main.py").toString(), QStringLiteral("print('hello')\n"));
        QVERIFY(!pm.readFile(root + "/logo.bin").isValid()); // binary files are not opened as text
        QVERIFY(!pm.readFile(root + "/nope.txt").isValid());
        QVERIFY(pm.writeFile(root + "/src/util.c", "int add(int a, int b);\n"));
        QCOMPARE(pm.readFile(root + "/src/util.c").toString(), QStringLiteral("int add(int a, int b);\n"));

        const QVariantList hits = pm.searchFiles(QStringLiteral("PRINT"));
        QCOMPARE(hits.size(), 1);
        QCOMPARE(hits.at(0).toMap().value("line").toInt(), 1);
        QVERIFY(hits.at(0).toMap().value("path").toString().endsWith("main.py"));
    }

    void runsFilesInPlace()
    {
        QTemporaryDir tmp;
        const QString root = tmp.path();
        QString output; // declared before `term` so it outlives every signal the service sends
        Terminal::TerminalService term;
        QSignalSpy finished(&term, &Terminal::TerminalService::finished);
        connect(&term, &Terminal::TerminalService::outputReceived, this, [&output](const QString& t) { output += t; });

        // Python runs in the file's own folder, so it can import a neighbouring module.
        if (have({"python", "python3"})) {
            QVERIFY(write(root + "/helper.py", "WORD = 'neighbour'\n"));
            QVERIFY(write(root + "/main.py", "import helper\nprint('hello', helper.WORD)\n"));
            term.runFile(root + "/main.py", QStringLiteral("python"));
            QVERIFY(finished.wait(60000));
            QVERIFY2(finished.last().at(0).toInt() == 0, qPrintable(output));
            QVERIFY2(output.contains("hello neighbour"), qPrintable(output));
        }

        // GNU assembler source: gcc assembles and links it, main is the entry point (x86-64 only).
        if (QSysInfo::currentCpuArchitecture() == QStringLiteral("x86_64") && have({"gcc", "clang", "cc"})) {
            QVERIFY(write(root + "/exit42.s", ".globl main\n.globl _main\nmain:\n_main:\n    movl $42, %eax\n    ret\n"));
            finished.clear();
            term.runFile(root + "/exit42.s", QStringLiteral("asm"));
            QVERIFY(finished.wait(60000));
            QVERIFY2(finished.last().at(0).toInt() == 42, qPrintable(output));
        }

        // NASM source: assembled for this OS's object format, then linked by the C compiler.
        if (QSysInfo::currentCpuArchitecture() == QStringLiteral("x86_64") && have({"nasm"}) && have({"gcc", "clang", "cc"})) {
            QVERIFY(write(root + "/exit42.asm", "global main\nglobal _main\nsection .text\nmain:\n_main:\n    mov eax, 42\n    ret\n"));
            finished.clear();
            term.runFile(root + "/exit42.asm", QStringLiteral("asm"));
            QVERIFY(finished.wait(60000));
            QVERIFY2(finished.last().at(0).toInt() == 42, qPrintable(output));
        }

        // Unsupported files are reported, not silently ignored.
        QVERIFY(write(root + "/notes.txt", "hi"));
        finished.clear();
        term.runFile(root + "/notes.txt", QStringLiteral("text"));
        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.last().at(0).toInt(), 1);
    }
};

QTEST_GUILESS_MAIN(WorkspaceTest)
#include "workspace_test.moc"
