#ifndef COMMENT_EXTRACTOR_H
#define COMMENT_EXTRACTOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include "Comment.h"
#include "LanguageRegistry.h"

/**
 * @brief Extracts comments from source code files
 * 
 * This class implements comment extraction that properly handles strings,
 * regex literals, and other language constructs to avoid false positives.
 * 
 * Supports two extraction methods:
 * - Lexer-based: Fast but may have edge cases in complex constructs
 * - Tree-sitter: More accurate parsing (requires tree-sitter setup)
 * 
 * The extractor uses the LanguageRegistry to determine comment syntax
 * for each language.
 */
class CommentExtractor : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief Extraction method to use
     */
    enum class ParserMethod {
        Lexer,       // Fast lexer-based extraction
        TreeSitter   // Parser-based using Tree-sitter
    };
    
    explicit CommentExtractor(QObject* parent = nullptr);
    ~CommentExtractor() = default;
    
    /**
     * @brief Set the parser method to use
     */
    void setParserMethod(ParserMethod method) { m_parserMethod = method; }
    ParserMethod parserMethod() const { return m_parserMethod; }
    
    /**
     * @brief Configure the number of context lines to capture
     */
    void setContextLines(int lines) { m_contextLines = lines; }
    int contextLines() const { return m_contextLines; }
    
    /**
     * @brief Configure keywords to detect (TODO, FIXME, etc.)
     */
    void setKeywords(const QStringList& keywords);
    QStringList keywords() const { return m_keywords; }
    
    /**
     * @brief Extract all comments from a file
     * @param filePath Full path to the file
     * @param relativePath Relative path from scan root
     * @return Vector of extracted comments
     */
    QVector<Comment> extractFromFile(const QString& filePath, const QString& relativePath);
    
    /**
     * @brief Extract comments from content (for testing)
     * @param content Source code content
     * @param language Language name
     * @param filePath Optional file path for context
     * @return Vector of extracted comments
     */
    QVector<Comment> extractFromContent(const QString& content, 
                                         const QString& language,
                                         const QString& filePath = QString());
    
signals:
    /**
     * @brief Emitted when extraction is complete for a file
     */
    void fileProcessed(const QString& filePath, int commentCount);
    
    /**
     * @brief Emitted on extraction error
     */
    void error(const QString& filePath, const QString& message);
    
private:
    /**
     * @brief Extract comments using lexer-based approach
     */
    QVector<Comment> extractWithLexer(const QString& content, 
                                       const LanguageConfig& config,
                                       const QString& filePath,
                                       const QString& relativePath);
    
    /**
     * @brief Skip string literal starting at position
     * @return New position after the string, or -1 if not a string start
     */
    int skipString(const QString& content, int pos, const LanguageConfig& config) const;
    
    /**
     * @brief Skip raw string literal
     */
    int skipRawString(const QString& content, int pos, const LanguageConfig& config) const;
    
    /**
     * @brief Skip regex literal (JavaScript, etc.)
     */
    int skipRegex(const QString& content, int pos, const LanguageConfig& config) const;
    
    /**
     * @brief Check if position is at a single-line comment start
     */
    QString matchSingleLineStart(const QString& content, int pos, const LanguageConfig& config) const;
    
    /**
     * @brief Check if position is at a block comment start
     */
    QPair<QString, QString> matchBlockStart(const QString& content, int pos, const LanguageConfig& config) const;
    
    /**
     * @brief Extract context lines around a comment
     */
    void extractContext(const QString& content, Comment& comment) const;
    
    /**
     * @brief Detect keywords in comment text
     */
    void detectKeywords(Comment& comment) const;
    
    /**
     * @brief Calculate line/column from position
     */
    QPair<int, int> positionToLineColumn(const QString& content, int pos) const;
    
    /**
     * @brief Get line content at given line number
     */
    QString getLine(const QString& content, int lineNumber) const;
    
    /**
     * @brief Determine if comment is inline (code before it on same line)
     */
    bool isInlineComment(const QString& content, int commentStart) const;
    
    /**
     * @brief Extract using Tree-sitter
     */
    QVector<Comment> extractWithTreeSitter(const QString& content,
                                            const LanguageConfig& config,
                                            const QString& filePath,
                                            const QString& relativePath);
    
    ParserMethod m_parserMethod = ParserMethod::TreeSitter;
    int m_contextLines = 2;
    QStringList m_keywords;
    int m_nextCommentId = 0;
    
    static const QStringList DEFAULT_KEYWORDS;
};

#endif // COMMENT_EXTRACTOR_H
