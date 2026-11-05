#include "CommentExtractor.h"
#include "LanguageRegistry.h"
#include "TextFile.h"
#include <tree_sitter/api.h>
#include <atomic>
#include <cstring>
#include <memory>

extern "C" {
const TSLanguage* tree_sitter_json(void);
const TSLanguage* tree_sitter_cpp(void);
const TSLanguage* tree_sitter_python(void);
const TSLanguage* tree_sitter_javascript(void);
}

namespace {
std::atomic<int> g_nextCommentId{1};

const TSLanguage* grammarFor(const QString& language) {
    if (language == u"JSONC") return tree_sitter_json();
    if (language == u"C/C++") return tree_sitter_cpp();
    if (language == u"Python") return tree_sitter_python();
    if (language == u"JavaScript" || language == u"TypeScript") return tree_sitter_javascript();
    return nullptr;
}

struct ParserDeleter { void operator()(TSParser* p) const { ts_parser_delete(p); } };
struct TreeDeleter { void operator()(TSTree* t) const { ts_tree_delete(t); } };

bool isTsComment(const char* type) {
    return !std::strcmp(type, "comment") || !std::strcmp(type, "line_comment") || !std::strcmp(type, "block_comment");
}
} // namespace

struct CommentExtractor::Context {
    const QString& text;
    const LanguageConfig& cfg;
    const QString& filePath;
    const QString& relativePath;
    LineIndex lines;
    QVector<Comment> out;
};

QStringList CommentExtractor::defaultKeywords() {
    return {"TODO", "FIXME", "XXX", "HACK", "NOTE", "BUG", "OPTIMIZE", "REVIEW", "WARNING", "DEPRECATED"};
}

CommentExtractor::CommentExtractor() { setKeywords(defaultKeywords()); }

void CommentExtractor::setKeywords(const QStringList& keywords) {
    m_keywords = keywords;
    QStringList escaped;
    for (const QString& k : keywords)
        if (!k.trimmed().isEmpty()) escaped << QRegularExpression::escape(k.trimmed());
    m_keywordRegex = QRegularExpression(escaped.isEmpty() ? QString() : "\\b(?:" + escaped.join('|') + ")\\b",
                                        QRegularExpression::CaseInsensitiveOption);
    m_keywordRegex.optimize();
}

QVector<Comment> CommentExtractor::extractFromFile(const QString& filePath, const QString& relativePath) const {
    const LanguageConfig* cfg = LanguageRegistry::instance().byPath(filePath);
    TextFile file;
    if (!cfg || !file.read(filePath)) return {};
    return extract(file.text, *cfg, filePath, relativePath);
}

QVector<Comment> CommentExtractor::extractFromContent(const QString& content, const QString& language,
                                                      const QString& filePath) const {
    const LanguageConfig* cfg = LanguageRegistry::instance().byName(language);
    return cfg ? extract(content, *cfg, filePath, filePath) : QVector<Comment>();
}

QVector<Comment> CommentExtractor::extract(const QString& content, const LanguageConfig& config,
                                           const QString& filePath, const QString& relativePath) const {
    if (content.isEmpty()) return {};
    Context ctx{content, config, filePath, relativePath, LineIndex(content), {}};
    if (m_parserMethod != ParserMethod::TreeSitter || !parseTreeSitter(ctx)) lex(ctx);
    return std::move(ctx.out);
}

void CommentExtractor::addComment(Context& ctx, int start, int end) const {
    const LanguageConfig& cfg = ctx.cfg;
    const QStringView raw = QStringView(ctx.text).mid(start, end - start);
    if (start == 0 && raw.startsWith(u"#!")) return;  // shebang: a directive, not a comment

    // Work out which delimiters surround the comment text.
    int open = 0, close = 0;
    bool block = false;
    auto matchPair = [&](const QVector<Delimiter>& pairs) {
        for (const Delimiter& d : pairs) {
            if (!raw.startsWith(d.start)) continue;
            open = int(d.start.size());
            close = raw.size() >= open + d.end.size() && raw.endsWith(d.end) ? int(d.end.size()) : 0;
            return block = true;
        }
        return false;
    };
    if (!matchPair(cfg.blocks) && !matchPair(cfg.strings)) {
        for (const QString& m : cfg.lineMarkers)
            if (raw.startsWith(m)) { open = int(m.size()); break; }
    }
    bool doc = false;  // doc markers ("///", "/**") are longer variants of the plain ones
    for (const QString& m : block ? cfg.docBlockStarts : cfg.docLineMarkers) {
        if (doc || !raw.startsWith(m) || raw.size() < m.size() + close) continue;
        doc = true;
        open = std::max(open, int(m.size()));
    }

    Comment c;
    c.id = g_nextCommentId.fetch_add(1, std::memory_order_relaxed);
    c.filePath = ctx.filePath;
    c.relativePath = ctx.relativePath;
    c.language = cfg.name;
    c.rawText = raw.toString();
    c.startLine = ctx.lines.line(start);
    c.startColumn = start - ctx.lines.lineStart(c.startLine);
    c.endLine = ctx.lines.line(end);
    c.endColumn = end - ctx.lines.lineStart(c.endLine);

    int cleanEnd = int(raw.size()) - close;
    c.cleanStart = std::min(open, cleanEnd);
    while (c.cleanStart < cleanEnd && raw[c.cleanStart].isSpace()) ++c.cleanStart;
    while (cleanEnd > c.cleanStart && raw[cleanEnd - 1].isSpace()) --cleanEnd;
    c.cleanLength = cleanEnd - c.cleanStart;

    bool codeBefore = false;
    for (int i = ctx.lines.lineStart(c.startLine); i < start && !codeBefore; ++i) codeBefore = !ctx.text[i].isSpace();
    c.type = codeBefore ? Comment::Type::Inline
           : doc        ? Comment::Type::DocComment
           : block      ? Comment::Type::MultiLine
                        : Comment::Type::SingleLine;

    if (!m_keywordRegex.pattern().isEmpty() && c.cleanLength > 0) {
        for (auto it = m_keywordRegex.globalMatch(c.cleanText()); it.hasNext();) {
            const QString hit = it.next().captured();
            for (const QString& k : m_keywords)
                if (hit.compare(k.trimmed(), Qt::CaseInsensitive) == 0 && !c.keywords.contains(k)) c.keywords << k;
        }
    }
    ctx.out.append(std::move(c));
}

void CommentExtractor::lex(Context& ctx) const {
    const LanguageConfig& cfg = ctx.cfg;
    const QString& text = ctx.text;
    const QStringView v(text);
    const int n = int(text.size());
    auto at = [&](int p, QStringView tok) { return v.mid(p, tok.size()) == tok; };

    // Characters that can begin something interesting; everything else is skipped quickly.
    QString triggers = QStringLiteral("([{}])");
    for (const auto& d : cfg.blocks) triggers += d.start[0];
    for (const auto& d : cfg.strings) triggers += d.start[0];
    for (const auto& m : cfg.lineMarkers) triggers += m[0];
    if (cfg.cppRawStrings) triggers += u'R';
    if (cfg.regexLiterals) triggers += u'/';

    auto blockEnd = [&](int p, const Delimiter& d) {
        int depth = 1, i = p + int(d.start.size());
        while (true) {
            const int close = int(text.indexOf(d.end, i));
            if (close < 0) return n;
            const int open = cfg.nestedBlocks ? int(text.indexOf(d.start, i)) : -1;
            if (open >= 0 && open < close) { ++depth; i = open + int(d.start.size()); continue; }
            if (--depth == 0) return close + int(d.end.size());
            i = close + int(d.end.size());
        }
    };
    auto skipString = [&](int p) {
        for (const Delimiter& d : cfg.strings) {
            if (!at(p, d.start)) continue;
            const bool multiLine = d.start.size() > 1 || d.start == u"`";
            for (int i = p + int(d.start.size()); i < n; ++i) {
                if (v[i] == u'\\') { ++i; continue; }
                if (at(i, d.end)) return i + int(d.end.size());
                if (!multiLine && v[i] == u'\n') return i;
            }
            return n;
        }
        return p;
    };
    auto skipRawString = [&](int p) {  // C++ R"delim(...)delim"
        if (!at(p, u"R\"")) return p;
        const int paren = int(text.indexOf(u'(', p + 2));
        if (paren < 0 || paren - (p + 2) > 16 || v.mid(p + 2, paren - p - 2).contains(u'\n')) return p;
        const QString endTok = u')' + v.mid(p + 2, paren - p - 2).toString() + u'"';
        const int e = int(text.indexOf(endTok, paren + 1));
        return e < 0 ? n : e + int(endTok.size());
    };
    auto skipRegex = [&](int p) {  // JavaScript /re/flags heuristic
        if (v[p] != u'/' || at(p, u"//") || at(p, u"/*")) return p;
        int prev = p - 1;
        while (prev >= 0 && v[prev].isSpace()) --prev;
        if (prev >= 0 && !QStringView(u"=:,([{!&|?;").contains(v[prev])) {
            int w = prev;
            while (w >= 0 && (v[w].isLetterOrNumber() || v[w] == u'_')) --w;
            static const QStringList keywords = {"return", "throw", "yield", "case", "void", "typeof", "delete", "await"};
            if (!keywords.contains(v.mid(w + 1, prev - w).toString())) return p;  // probably division
        }
        for (int i = p + 1; i < n; ++i) {
            const QChar c = v[i];
            if (c == u'\\') { ++i; continue; }
            if (c == u'\n') return p;
            if (c == u'[') {
                while (++i < n && v[i] != u']' && v[i] != u'\n') if (v[i] == u'\\') ++i;
                continue;
            }
            if (c == u'/') {
                while (i + 1 < n && v[i + 1].isLetter()) ++i;
                return i + 1;
            }
        }
        return p;
    };
    auto onlySpaceBefore = [&](int p) {
        for (int i = p - 1; i >= 0 && v[i] != u'\n'; --i) if (!v[i].isSpace()) return false;
        return true;
    };
    auto continuesStatement = [&](int p) {  // previous line ends with '\' or '=' (e.g. x = \ """...""")
        int i = p - 1;
        while (i >= 0 && v[i].isSpace()) --i;
        return i >= 0 && (v[i] == u'\\' || v[i] == u'=');
    };

    int depth = 0;  // bracket depth; docstrings only appear outside brackets
    for (int p = 0; p < n;) {
        const QChar c = v[p];
        if (c.isSpace() || !triggers.contains(c)) { ++p; continue; }

        int end = -1;
        if (cfg.blocksAtLineStart && depth == 0 && onlySpaceBefore(p) && !continuesStatement(p)) {
            for (const Delimiter& d : cfg.blocks) if (at(p, d.start)) { end = blockEnd(p, d); break; }
        }
        if (end < 0) {
            int skipped = skipString(p);
            if (skipped == p && cfg.cppRawStrings) skipped = skipRawString(p);
            if (skipped == p && cfg.regexLiterals) skipped = skipRegex(p);
            if (skipped > p) { p = skipped; continue; }
        }
        if (end < 0 && !cfg.blocksAtLineStart) {
            for (const Delimiter& d : cfg.blocks) if (at(p, d.start)) { end = blockEnd(p, d); break; }
        }
        if (end < 0 && (!cfg.markerNeedsSpace || p == 0 || v[p - 1].isSpace())) {
            for (const QString& m : cfg.lineMarkers) {
                if (!at(p, m) || (m == u"#" && at(p + 1, u"[") && cfg.name == u"PHP")) continue;  // PHP 8 attribute
                end = int(text.indexOf(u'\n', p));
                if (end < 0) end = n;
                break;
            }
        }
        if (end >= 0) {
            addComment(ctx, p, end);
            p = std::max(end, p + 1);
            continue;
        }
        if (c == u'(' || c == u'[' || c == u'{') ++depth;
        else if ((c == u')' || c == u']' || c == u'}') && depth > 0) --depth;
        ++p;
    }
}

bool CommentExtractor::parseTreeSitter(Context& ctx) const {
    const TSLanguage* language = grammarFor(ctx.cfg.name);
    if (!language) return false;
    thread_local std::unique_ptr<TSParser, ParserDeleter> parser(ts_parser_new());
    if (!ts_parser_set_language(parser.get(), language)) return false;

    // QString is UTF-16, so byte offsets divided by two are QString indices.
    std::unique_ptr<TSTree, TreeDeleter> tree(ts_parser_parse_string_encoding(
        parser.get(), nullptr, reinterpret_cast<const char*>(ctx.text.utf16()), uint32_t(ctx.text.size() * 2),
        TSInputEncodingUTF16));
    if (!tree) return false;
    const TSNode root = ts_tree_root_node(tree.get());
    // Syntax the grammar does not understand (e.g. TypeScript types, unexpanded macros)
    // is handled more predictably by the lexer.
    if (ts_node_is_null(root) || ts_node_has_error(root)) return false;

    const bool python = ctx.cfg.name == u"Python";
    TSTreeCursor cursor = ts_tree_cursor_new(root);
    for (bool more = true; more;) {
        const TSNode node = ts_tree_cursor_current_node(&cursor);
        const char* type = ts_node_type(node);
        bool comment = isTsComment(type);
        if (!comment && python && !std::strcmp(type, "string")) {  // docstring = string used as a statement
            const TSNode parent = ts_node_parent(node);
            comment = !ts_node_is_null(parent) && !std::strcmp(ts_node_type(parent), "expression_statement");
        }
        if (comment) addComment(ctx, int(ts_node_start_byte(node) / 2), int(ts_node_end_byte(node) / 2));
        if (!comment && ts_tree_cursor_goto_first_child(&cursor)) continue;
        while (!ts_tree_cursor_goto_next_sibling(&cursor)) {
            if (!ts_tree_cursor_goto_parent(&cursor)) { more = false; break; }
        }
    }
    ts_tree_cursor_delete(&cursor);
    return true;
}
