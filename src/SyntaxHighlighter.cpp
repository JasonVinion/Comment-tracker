#include "SyntaxHighlighter.h"
#include "LanguageRegistry.h"

namespace {
const char* keywordsFor(const QString& language) {
    static const QStringList cLike = {"C/C++", "C#", "Java", "Kotlin", "Go", "Rust", "Swift", "Scala", "Dart",
                                      "Objective-C", "Groovy", "Protocol Buffers"};
    if (cLike.contains(language))
        return "alignas alignof asm auto bool break case catch char class const consteval constexpr constinit "
               "const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast "
               "else enum explicit export extern false final float for fn friend func goto if impl import inline "
               "int interface let long match mod mutable namespace new noexcept nullptr operator override package "
               "private protected pub public register reinterpret_cast requires return self short signed sizeof "
               "static static_assert static_cast struct super switch template this thread_local throw trait true "
               "try typedef typeid typename union unsigned use using val var virtual void volatile where while";
    if (language == u"Python")
        return "False None True and as assert async await break class continue def del elif else except finally "
               "for from global if import in is lambda nonlocal not or pass raise return try while with yield";
    if (language == u"JavaScript" || language == u"TypeScript")
        return "any as async await boolean break case catch class const continue debugger declare default delete "
               "do else enum export extends false finally for from function if implements import in infer "
               "instanceof interface keyof let module namespace never new null number of package private "
               "protected public readonly return static string super switch symbol this throw true try type "
               "typeof undefined unknown var void while with yield";
    if (language == u"Shell" || language == u"PowerShell")
        return "if then else elif fi case esac for while until do done in function select time coproc return";
    return "";
}
} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    m_commentFormat.setForeground(Qt::darkGreen);
    m_commentFormat.setFontItalic(true);
    m_stringFormat.setForeground(Qt::darkRed);
    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);
    m_numberFormat.setForeground(Qt::darkMagenta);
    m_functionFormat.setForeground(QColor(0, 102, 153));
}

void SyntaxHighlighter::setLanguage(const QString& language) {
    m_config = LanguageRegistry::instance().byName(language);
    const QString words = QString::fromLatin1(keywordsFor(language)).split(u' ', Qt::SkipEmptyParts).join(u'|');
    m_keywords = QRegularExpression(words.isEmpty() ? QString() : "\\b(?:" + words + ")\\b");
    rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString& text) {
    static const QRegularExpression numbers("\\b(?:0[xX][0-9a-fA-F]+|[0-9]+(?:\\.[0-9]*)?)[a-zA-Z]*\\b");
    static const QRegularExpression calls("\\b[A-Za-z_]\\w*(?=\\s*\\()");
    setCurrentBlockState(-1);
    if (!m_config) return;

    auto apply = [&](const QRegularExpression& re, const QTextCharFormat& format) {
        for (auto it = re.globalMatch(text); it.hasNext();) {
            const auto m = it.next();
            setFormat(int(m.capturedStart()), int(m.capturedLength()), format);
        }
    };
    apply(numbers, m_numberFormat);
    if (!m_keywords.pattern().isEmpty()) {
        apply(calls, m_functionFormat);
        apply(m_keywords, m_keywordFormat);
    }

    // Comments and strings, scanned left to right so each hides markers inside the other.
    const LanguageConfig& cfg = *m_config;
    const QStringView v(text);
    const int n = int(text.size());
    int i = 0;
    auto finishBlock = [&](int blockIndex, int from) {  // returns position after the block end, or n
        const QString& end = cfg.blocks[blockIndex].end;
        const int close = int(text.indexOf(end, from));
        const int stop = close < 0 ? n : close + int(end.size());
        setFormat(i, stop - i, m_commentFormat);
        if (close < 0) setCurrentBlockState(blockIndex);
        return stop;
    };
    if (previousBlockState() >= 0 && previousBlockState() < cfg.blocks.size()) i = finishBlock(previousBlockState(), 0);

    while (i < n) {
        bool matched = false;
        for (int b = 0; b < cfg.blocks.size() && !matched; ++b) {
            if (!v.mid(i).startsWith(cfg.blocks[b].start)) continue;
            i = finishBlock(b, i + int(cfg.blocks[b].start.size()));
            matched = true;
        }
        if (matched) continue;
        if (!cfg.markerNeedsSpace || i == 0 || v[i - 1].isSpace()) {
            for (const QString& marker : cfg.lineMarkers) {
                if (!v.mid(i).startsWith(marker)) continue;
                setFormat(i, n - i, m_commentFormat);
                return;
            }
        }
        for (const Delimiter& d : cfg.strings) {
            if (!v.mid(i).startsWith(d.start)) continue;
            int j = i + int(d.start.size());
            while (j < n && !v.mid(j).startsWith(d.end)) j += v[j] == u'\\' ? 2 : 1;
            j = std::min(n, j + int(d.end.size()));
            setFormat(i, j - i, m_stringFormat);
            i = j;
            matched = true;
            break;
        }
        if (!matched) ++i;
    }
}
