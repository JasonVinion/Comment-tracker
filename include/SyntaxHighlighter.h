#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

struct LanguageConfig;

/// Highlights comments and strings using the comment/string syntax from the
/// LanguageRegistry, plus keywords, numbers and function calls for common languages.
class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent);
    void setLanguage(const QString& language);

protected:
    void highlightBlock(const QString& text) override;

private:
    const LanguageConfig* m_config = nullptr;
    QRegularExpression m_keywords;
    QTextCharFormat m_commentFormat, m_stringFormat, m_keywordFormat, m_numberFormat, m_functionFormat;
};

#endif // SYNTAX_HIGHLIGHTER_H
