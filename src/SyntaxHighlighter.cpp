#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
    // Setup default formats
    m_commentFormat.setForeground(Qt::darkGreen);
    m_commentFormat.setFontItalic(true);
    
    m_stringFormat.setForeground(Qt::darkRed);
    
    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);
    
    m_numberFormat.setForeground(Qt::darkMagenta);
    
    m_functionFormat.setForeground(QColor(0, 102, 153));
    m_functionFormat.setFontWeight(QFont::Bold);
    
    m_preprocessorFormat.setForeground(Qt::darkMagenta);
    
    setupGenericRules();
}

void SyntaxHighlighter::setLanguage(const QString& language) {
    m_language = language;
    m_rules.clear();
    
    if (language == "C/C++" || language == "C#" || language == "Java" ||
        language == "Kotlin" || language == "Go" || language == "Rust" ||
        language == "Swift" || language == "Scala" || language == "Dart") {
        setupCppRules();
    } else if (language == "Python") {
        setupPythonRules();
    } else if (language == "JavaScript" || language == "TypeScript") {
        setupJavaScriptRules();
    } else if (language == "Shell" || language == "PowerShell") {
        setupShellRules();
    } else {
        setupGenericRules();
    }
    
    rehighlight();
}

void SyntaxHighlighter::setCommentFormat(const QTextCharFormat& format) {
    m_commentFormat = format;
}

void SyntaxHighlighter::setStringFormat(const QTextCharFormat& format) {
    m_stringFormat = format;
}

void SyntaxHighlighter::setKeywordFormat(const QTextCharFormat& format) {
    m_keywordFormat = format;
}

void SyntaxHighlighter::setNumberFormat(const QTextCharFormat& format) {
    m_numberFormat = format;
}

void SyntaxHighlighter::setFunctionFormat(const QTextCharFormat& format) {
    m_functionFormat = format;
}

void SyntaxHighlighter::setupGenericRules() {
    HighlightRule rule;
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Single-line comments (// style)
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
    
    // Single-line comments (# style)
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Multi-line comments
    m_commentStartExpr = QRegularExpression("/\\*");
    m_commentEndExpr = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupCppRules() {
    HighlightRule rule;
    
    // Keywords
    QStringList keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto",
        "bitand", "bitor", "bool", "break", "case", "catch",
        "char", "char8_t", "char16_t", "char32_t", "class", "compl",
        "concept", "const", "consteval", "constexpr", "constinit", "const_cast",
        "continue", "co_await", "co_return", "co_yield", "decltype", "default",
        "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for",
        "friend", "goto", "if", "inline", "int", "long",
        "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected",
        "public", "register", "reinterpret_cast", "requires", "return", "short",
        "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "union", "unsigned",
        "using", "virtual", "void", "volatile", "wchar_t", "while",
        "xor", "xor_eq", "override", "final"
    };
    
    for (const QString& keyword : keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Preprocessor
    rule.pattern = QRegularExpression("^\\s*#[^\n]*");
    rule.format = m_preprocessorFormat;
    m_rules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[fFlLuU]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+[uUlL]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
    
    // Multi-line comments
    m_commentStartExpr = QRegularExpression("/\\*");
    m_commentEndExpr = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupPythonRules() {
    HighlightRule rule;
    
    // Keywords
    QStringList keywords = {
        "False", "None", "True", "and", "as", "assert", "async",
        "await", "break", "class", "continue", "def", "del", "elif",
        "else", "except", "finally", "for", "from", "global", "if",
        "import", "in", "is", "lambda", "nonlocal", "not", "or",
        "pass", "raise", "return", "try", "while", "with", "yield"
    };
    
    for (const QString& keyword : keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Built-in functions
    QStringList builtins = {
        "abs", "all", "any", "bin", "bool", "bytearray", "bytes",
        "callable", "chr", "classmethod", "compile", "complex", "delattr",
        "dict", "dir", "divmod", "enumerate", "eval", "exec", "filter",
        "float", "format", "frozenset", "getattr", "globals", "hasattr",
        "hash", "help", "hex", "id", "input", "int", "isinstance",
        "issubclass", "iter", "len", "list", "locals", "map", "max",
        "memoryview", "min", "next", "object", "oct", "open", "ord",
        "pow", "print", "property", "range", "repr", "reversed", "round",
        "set", "setattr", "slice", "sorted", "staticmethod", "str", "sum",
        "super", "tuple", "type", "vars", "zip"
    };
    
    for (const QString& builtin : builtins) {
        rule.pattern = QRegularExpression("\\b" + builtin + "\\b");
        rule.format = m_functionFormat;
        m_rules.append(rule);
    }
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*[jJ]?\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Functions (definitions)
    rule.pattern = QRegularExpression("\\bdef\\s+([A-Za-z_][A-Za-z0-9_]*)");
    rule.format = m_functionFormat;
    m_rules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
    
    // Multi-line strings/comments (triple quotes)
    m_commentStartExpr = QRegularExpression("(\"\"\"|''')");
    m_commentEndExpr = QRegularExpression("(\"\"\"|''')");
}

void SyntaxHighlighter::setupJavaScriptRules() {
    HighlightRule rule;
    
    // Keywords
    QStringList keywords = {
        "break", "case", "catch", "continue", "debugger", "default",
        "delete", "do", "else", "finally", "for", "function", "if",
        "in", "instanceof", "new", "return", "switch", "this", "throw",
        "try", "typeof", "var", "void", "while", "with", "class",
        "const", "enum", "export", "extends", "import", "super",
        "implements", "interface", "let", "package", "private", "protected",
        "public", "static", "yield", "async", "await", "of"
    };
    
    for (const QString& keyword : keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // TypeScript-specific keywords
    QStringList tsKeywords = {
        "type", "interface", "namespace", "module", "declare", "as",
        "any", "boolean", "number", "string", "symbol", "never",
        "unknown", "readonly", "keyof", "infer", "extends"
    };
    
    for (const QString& keyword : tsKeywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Literals
    rule.pattern = QRegularExpression("\\b(true|false|null|undefined|NaN|Infinity)\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()");
    rule.format = m_functionFormat;
    m_rules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Template literals (basic)
    rule.pattern = QRegularExpression("`[^`]*`");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
    
    // Multi-line comments
    m_commentStartExpr = QRegularExpression("/\\*");
    m_commentEndExpr = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupShellRules() {
    HighlightRule rule;
    
    // Keywords
    QStringList keywords = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for",
        "while", "until", "do", "done", "in", "function", "select",
        "time", "coproc"
    };
    
    for (const QString& keyword : keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }
    
    // Variables
    rule.pattern = QRegularExpression("\\$[A-Za-z_][A-Za-z0-9_]*");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("\\$\\{[^}]+\\}");
    rule.format = m_numberFormat;
    m_rules.append(rule);
    
    // Strings
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    rule.pattern = QRegularExpression("'[^']*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    
    // Comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
    // Apply rules
    for (const HighlightRule& rule : m_rules) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    
    // Handle multi-line comments
    highlightMultilineComments(text);
}

void SyntaxHighlighter::highlightMultilineComments(const QString& text) {
    if (!m_commentStartExpr.isValid() || !m_commentEndExpr.isValid()) {
        return;
    }
    
    setCurrentBlockState(0);
    
    int startIndex = 0;
    bool inComment = (previousBlockState() == 1);
    
    // If continuing from a previous block's comment, start from beginning
    if (!inComment) {
        QRegularExpressionMatch startMatch = m_commentStartExpr.match(text);
        startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
    }
    
    while (startIndex >= 0 || inComment) {
        int endIndex;
        QRegularExpressionMatch endMatch;
        
        if (inComment) {
            // Continuing from previous block, search from beginning
            endMatch = m_commentEndExpr.match(text, 0);
            startIndex = 0;
        } else {
            // New comment on this line, search after start marker
            endMatch = m_commentEndExpr.match(text, startIndex + 1);
        }
        
        endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        
        if (commentLength > 0) {
            setFormat(startIndex, commentLength, m_commentFormat);
        }
        
        // Reset inComment flag after processing
        inComment = false;
        
        // Look for next comment start after the current comment
        QRegularExpressionMatch nextStartMatch = m_commentStartExpr.match(text, startIndex + commentLength);
        startIndex = nextStartMatch.hasMatch() ? nextStartMatch.capturedStart() : -1;
    }
}
