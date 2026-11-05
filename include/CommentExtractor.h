#ifndef COMMENT_EXTRACTOR_H
#define COMMENT_EXTRACTOR_H

#include <QRegularExpression>
#include <QVector>
#include "Comment.h"

struct LanguageConfig;
class LineIndex;

/// Extracts comments from source files. Configure it first; afterwards all
/// extraction methods are const and safe to call from several threads at once.
class CommentExtractor {
public:
    enum class ParserMethod { Lexer, TreeSitter };

    CommentExtractor();

    void setParserMethod(ParserMethod method) { m_parserMethod = method; }
    ParserMethod parserMethod() const { return m_parserMethod; }

    void setKeywords(const QStringList& keywords);
    const QStringList& keywords() const { return m_keywords; }
    static QStringList defaultKeywords();

    /// Reads and parses a file. Returns nothing for unreadable or unsupported files.
    QVector<Comment> extractFromFile(const QString& filePath, const QString& relativePath) const;
    /// Parses in-memory content written in the named language (e.g. "C/C++").
    QVector<Comment> extractFromContent(const QString& content, const QString& language,
                                        const QString& filePath = QString()) const;
    QVector<Comment> extract(const QString& content, const LanguageConfig& config,
                             const QString& filePath, const QString& relativePath) const;

private:
    struct Context;
    void lex(Context& ctx) const;
    bool parseTreeSitter(Context& ctx) const;
    void addComment(Context& ctx, int start, int end) const;

    ParserMethod m_parserMethod = ParserMethod::Lexer;
    QStringList m_keywords;
    QRegularExpression m_keywordRegex;
};

#endif // COMMENT_EXTRACTOR_H
