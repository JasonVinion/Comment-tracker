#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

/**
 * @brief Multi-language syntax highlighter
 * 
 * Provides basic syntax highlighting for comments, strings, keywords, etc.
 * across multiple programming languages.
 */
class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
    
public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
    
    /**
     * @brief Set the language for highlighting
     */
    void setLanguage(const QString& language);
    QString language() const { return m_language; }
    
    /**
     * @brief Set custom formats
     */
    void setCommentFormat(const QTextCharFormat& format);
    void setStringFormat(const QTextCharFormat& format);
    void setKeywordFormat(const QTextCharFormat& format);
    void setNumberFormat(const QTextCharFormat& format);
    void setFunctionFormat(const QTextCharFormat& format);
    
protected:
    void highlightBlock(const QString& text) override;
    
private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    
    void setupGenericRules();
    void setupCppRules();
    void setupPythonRules();
    void setupJavaScriptRules();
    void setupShellRules();
    
    void highlightMultilineComments(const QString& text);
    
    QString m_language;
    QVector<HighlightRule> m_rules;
    
    // Multi-line comment state
    QRegularExpression m_commentStartExpr;
    QRegularExpression m_commentEndExpr;
    
    // Formats
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_preprocessorFormat;
};

#endif // SYNTAX_HIGHLIGHTER_H
