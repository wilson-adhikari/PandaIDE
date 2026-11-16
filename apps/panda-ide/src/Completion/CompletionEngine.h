#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace Completion {

// Word-based code suggestion engine (NOT an AI model).
// Works like classic IDE plugins: it completes identifiers from the
// open document plus per-language keyword tables, and expands a few
// built-in snippets. A separate set of helpers keeps indentation
// correct, with special handling for Python (colon blocks, dedent
// keywords, spaces instead of tabs).
class CompletionEngine : public QObject
{
    Q_OBJECT

public:
    explicit CompletionEngine(QObject* parent = nullptr);
    ~CompletionEngine() override;

    // Words starting with prefix (case-insensitive), document words
    // first, then language keywords. Empty when there is nothing
    // worth completing (e.g. prefix shorter than 2 chars, or the
    // only match is the prefix itself).
    Q_INVOKABLE QStringList suggest(const QString& prefix,
                                    const QString& language,
                                    const QString& documentText);

    // One-line description shown next to a suggestion ("keyword",
    // "builtin", "snippet", "register", ...).
    Q_INVOKABLE QString docFor(const QString& word,
                               const QString& language) const;

    // Multi-line snippet body for word, or empty when word is a
    // plain identifier. "$0" marks where the cursor should land.
    Q_INVOKABLE QString snippetFor(const QString& word,
                                   const QString& language) const;

    // Indentation (whitespace only) for a new line created by
    // pressing Enter at the end of lineText.
    Q_INVOKABLE QString indentForNewLine(const QString& lineText,
                                         const QString& language) const;

    // What the Tab key inserts. Always spaces so Python files
    // never end up with mixed tabs/spaces.
    Q_INVOKABLE QString tabString(const QString& language) const;

    Q_INVOKABLE QStringList languages() const;

private:
    static QStringList keywordsFor(const QString& language);
};

} // namespace Completion
