#include "CompletionEngine.h"

#include <QRegularExpression>
#include <QSet>

namespace Completion {

CompletionEngine::CompletionEngine(QObject* parent)
    : QObject(parent)
{
}

CompletionEngine::~CompletionEngine()
{
}

QStringList CompletionEngine::languages() const
{
    return {"cpp", "python", "asm", "rust", "csharp"};
}

QStringList CompletionEngine::keywordsFor(const QString& language)
{
    const QString lang = language.toLower();
    if (lang == "python" || lang == "py") {
        return {
            "False", "None", "True", "and", "as", "assert", "async",
            "await", "break", "class", "continue", "def", "del",
            "elif", "else", "except", "finally", "for", "from",
            "global", "if", "import", "in", "is", "lambda",
            "nonlocal", "not", "or", "pass", "raise", "return",
            "try", "while", "with", "yield",
            // builtins
            "print", "len", "range", "str", "int", "float",
            "bool", "list", "dict", "set", "tuple", "open",
            "enumerate", "zip", "map", "filter", "sorted",
            "isinstance", "hasattr", "getattr", "setattr",
            "super", "self", "cls", "__init__", "__main__",
            "__name__"
        };
    }
    if (lang == "asm" || lang == "assembly" || lang == "nasm") {
        return {
            // mnemonics (x86-64)
            "mov", "lea", "push", "pop", "call", "ret",
            "add", "sub", "mul", "div", "inc", "dec",
            "and", "or", "xor", "not", "shl", "shr",
            "cmp", "test", "jmp", "je", "jne", "jg",
            "jl", "jge", "jle", "ja", "jb", "loop",
            "nop", "int", "syscall",
            // registers
            "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
            "rbp", "rsp", "rip", "r8", "r9", "r10",
            "r11", "r12", "r13", "r14", "r15",
            "eax", "ebx", "ecx", "edx",
            // directives
            "section", "global", "extern", "bits",
            "default", "db", "dw", "dd", "dq",
            "resb", "resw", "resd", "resq", "equ",
            "times", "org", "%macro", "%endmacro"
        };
    }
    if (lang == "rust" || lang == "rs") {
        return {
            "as", "async", "await", "break", "const", "continue", "crate",
            "dyn", "else", "enum", "extern", "false", "fn", "for", "if",
            "impl", "in", "let", "loop", "match", "mod", "move", "mut",
            "pub", "ref", "return", "self", "Self", "static", "struct",
            "super", "trait", "true", "type", "unsafe", "use", "where",
            "while",
            // common std items
            "println!", "print!", "format!", "vec!", "panic!", "assert!",
            "String", "Vec", "Option", "Some", "None", "Result", "Ok",
            "Err", "Box", "HashMap", "i32", "i64", "u8", "u32", "u64",
            "usize", "f64", "bool", "str"
        };
    }
    if (lang == "csharp" || lang == "cs") {
        return {
            "abstract", "async", "await", "base", "bool", "break", "case",
            "catch", "class", "const", "continue", "decimal", "default",
            "do", "double", "else", "enum", "false", "finally", "float",
            "for", "foreach", "if", "in", "int", "interface", "internal",
            "is", "long", "namespace", "new", "null", "out", "override",
            "private", "protected", "public", "readonly", "record", "ref",
            "return", "sealed", "static", "string", "struct", "switch",
            "this", "throw", "true", "try", "using", "var", "virtual",
            "void", "while",
            // common library names
            "Console", "WriteLine", "ReadLine", "List", "Dictionary",
            "Task", "Math", "String", "DateTime"
        };
    }
    // C++ (also used for plain C files in v0.1)
    return {
        "alignas", "alignof", "and", "and_eq", "asm",
        "auto", "bitand", "bitor", "bool", "break",
        "case", "catch", "char", "char8_t", "char16_t",
        "char32_t", "class", "compl", "concept", "const",
        "consteval", "constexpr", "constinit", "const_cast",
        "continue", "co_await", "co_return", "co_yield",
        "decltype", "default", "delete", "do", "double",
        "dynamic_cast", "else", "enum", "explicit", "export",
        "extern", "false", "float", "for", "friend",
        "goto", "if", "inline", "int", "long",
        "mutable", "namespace", "new", "noexcept", "not",
        "not_eq", "nullptr", "operator", "or", "or_eq",
        "private", "protected", "public", "register",
        "reinterpret_cast", "return", "short", "signed",
        "sizeof", "static", "static_assert", "static_cast",
        "struct", "switch", "template", "this", "thread_local",
        "throw", "true", "try", "typedef", "typeid",
        "typename", "union", "unsigned", "using", "virtual",
        "void", "volatile", "wchar_t", "while", "xor",
        "xor_eq", "override", "final",
        "include", "define", "ifdef", "ifndef", "endif",
        "pragma", "std", "string", "vector", "cout",
        "cin", "endl", "main"
    };
}

QStringList CompletionEngine::suggest(const QString& prefix,
                                      const QString& language,
                                      const QString& documentText)
{
    if (prefix.size() < 2) {
        return {};
    }

    QStringList out;
    QSet<QString> seen;

    auto addWord = [&](const QString& word) {
        if (word.isEmpty() || seen.contains(word)) {
            return;
        }
        seen.insert(word);
        out << word;
    };

    // 1. Identifiers already used in the document, most recent first.
    //    This is what makes completion feel "aware" of your code.
    static const QRegularExpression identRe(
        QStringLiteral("\\b[A-Za-z_][A-Za-z0-9_]{1,}\\b"));
    QStringList docWords;
    auto it = identRe.globalMatch(documentText);
    while (it.hasNext()) {
        docWords << it.next().captured(0);
    }
    for (auto rit = docWords.crbegin(); rit != docWords.crend(); ++rit) {
        if (rit->startsWith(prefix, Qt::CaseInsensitive) &&
            rit->compare(prefix, Qt::CaseInsensitive) != 0) {
            addWord(*rit);
            if (out.size() >= 12) {
                return out;
            }
        }
    }

    // 2. Language keywords / builtins / mnemonics.
    for (const QString& word : keywordsFor(language)) {
        if (word.startsWith(prefix, Qt::CaseInsensitive) &&
            word.compare(prefix, Qt::CaseInsensitive) != 0) {
            addWord(word);
            if (out.size() >= 12) {
                break;
            }
        }
    }

    return out;
}

QString CompletionEngine::docFor(const QString& word,
                                 const QString& language) const
{
    const QString lang = language.toLower();
    const QString snippet = snippetFor(word, language);
    if (!snippet.isEmpty()) {
        return QStringLiteral("snippet");
    }
    if (lang == "asm" || lang == "assembly" || lang == "nasm") {
        static const QSet<QString> regs = {
            "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp",
            "rsp", "rip", "r8", "r9", "r10", "r11", "r12",
            "r13", "r14", "r15", "eax", "ebx", "ecx", "edx"
        };
        if (regs.contains(word.toLower())) {
            return QStringLiteral("register");
        }
        static const QSet<QString> dirs = {
            "section", "global", "extern", "bits", "default",
            "db", "dw", "dd", "dq", "resb", "resw", "resd",
            "resq", "equ", "times", "org"
        };
        if (dirs.contains(word.toLower())) {
            return QStringLiteral("directive");
        }
        return QStringLiteral("mnemonic");
    }
    if (lang == "python" || lang == "py") {
        static const QSet<QString> builtins = {
            "print", "len", "range", "str", "int", "float",
            "bool", "list", "dict", "set", "tuple", "open",
            "enumerate", "zip", "map", "filter", "sorted",
            "isinstance", "super", "self", "cls"
        };
        if (builtins.contains(word)) {
            return QStringLiteral("builtin");
        }
        return QStringLiteral("keyword");
    }
    return QStringLiteral("keyword");
}

QString CompletionEngine::snippetFor(const QString& word,
                                     const QString& language) const
{
    const QString lang = language.toLower();
    const QString w = word.toLower();
    if (lang == "python" || lang == "py") {
        if (w == "def") {
            return QStringLiteral("def $0():\n    pass");
        }
        if (w == "class") {
            return QStringLiteral("class $0:\n    def __init__(self):\n        pass");
        }
        if (w == "for") {
            return QStringLiteral("for $0 in range():\n    pass");
        }
        if (w == "while") {
            return QStringLiteral("while $0:\n    pass");
        }
        if (w == "if") {
            return QStringLiteral("if $0:\n    pass");
        }
        if (w == "try") {
            return QStringLiteral("try:\n    $0\n except Exception as e:\n    raise");
        }
        if (w == "with") {
            return QStringLiteral("with $0:\n    pass");
        }
        return {};
    }
    if (lang == "asm" || lang == "assembly" || lang == "nasm") {
        if (w == "section") {
            return QStringLiteral("section $0");
        }
        if (w == "%macro") {
            return QStringLiteral("%macro $0 0\n%endmacro");
        }
        return {};
    }
    // C++
    if (w == "for") {
        return QStringLiteral("for (int $0 = 0; $0 < n; ++$0) {\n}");
    }
    if (w == "if") {
        return QStringLiteral("if ($0) {\n}");
    }
    if (w == "while") {
        return QStringLiteral("while ($0) {\n}");
    }
    if (w == "class") {
        return QStringLiteral("class $0 {\npublic:\nprivate:\n};");
    }
    if (w == "main") {
        return QStringLiteral("int main() {\n    $0\n    return 0;\n}");
    }
    if (w == "include") {
        return QStringLiteral("#include <$0>");
    }
    return {};
}

QString CompletionEngine::tabString(const QString& /*language*/) const
{
    // Spaces everywhere on purpose: Python forbids mixing tabs
    // and spaces, so the editor never inserts a tab character.
    return QStringLiteral("    ");
}

QString CompletionEngine::indentForNewLine(const QString& lineText,
                                           const QString& language) const
{
    // Keep the current line's leading whitespace as the base so
    // Python files preserve whatever indentation they use.
    int i = 0;
    while (i < lineText.size() &&
           (lineText.at(i) == QLatin1Char(' ') ||
            lineText.at(i) == QLatin1Char('\t'))) {
        ++i;
    }
    QString indent = lineText.left(i);
    const QString trimmed = lineText.trimmed();
    if (trimmed.isEmpty()) {
        return indent;
    }

    auto unit = [&]() -> QString {
        // Match the file's existing style: tabs stay tabs,
        // otherwise use four spaces.
        if (indent.contains(QLatin1Char('\t')) &&
            !indent.contains(QLatin1Char(' '))) {
            return QStringLiteral("\t");
        }
        return QStringLiteral("    ");
    };
    auto dedentOnce = [&](QString s) -> QString {
        const QString u = unit();
        if (s.endsWith(u)) {
            s.chop(u.size());
        } else {
            // Fall back to removing up to one level of spaces.
            int removed = 0;
            while (!s.isEmpty() && removed < 4 &&
                   s.endsWith(QLatin1Char(' '))) {
                s.chop(1);
                ++removed;
            }
            if (s.endsWith(QLatin1Char('\t'))) {
                s.chop(1);
            }
        }
        return s;
    };

    const QString lang = language.toLower();
    if (lang == "python" || lang == "py") {
        // Block openers indent the next line...
        if (trimmed.endsWith(QLatin1Char(':'))) {
            return indent + unit();
        }
        // ...while these dedent one level (typing `else:` at a
        // deeper indent would otherwise break the file).
        static const char* dedentWords[] = {
            "elif", "else", "except", "finally", "case", nullptr
        };
        for (int k = 0; dedentWords[k] != nullptr; ++k) {
            const QString kw = QString::fromLatin1(dedentWords[k]);
            if (trimmed == kw || trimmed == kw + QStringLiteral(":") ||
                trimmed.startsWith(kw + QStringLiteral(" ")) ||
                trimmed.startsWith(kw + QStringLiteral(":"))) {
                return dedentOnce(indent);
            }
        }
        return indent;
    }

    // C-like and assembly: indent after an opening bracket/paren.
    const QChar last = trimmed.at(trimmed.size() - 1);
    if (last == QLatin1Char('{') || last == QLatin1Char('(') ||
        last == QLatin1Char('[')) {
        return indent + unit();
    }
    return indent;
}

} // namespace Completion
