#ifndef DIFF_PREVIEW_H
#define DIFF_PREVIEW_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QScrollBar>
#include "Comment.h"
#include "EditPlanner.h"

class SyntaxHighlighter;

/**
 * @brief Custom text editor with line numbers and highlighting
 */
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
    
public:
    explicit CodeEditor(QWidget* parent = nullptr);
    
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();
    
    /**
     * @brief Highlight a range of lines
     */
    void highlightLines(int startLine, int endLine, const QColor& color);
    
    /**
     * @brief Clear all highlights
     */
    void clearHighlights();
    
    /**
     * @brief Scroll to a specific line
     */
    void scrollToLine(int line);
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    
private:
    QWidget* m_lineNumberArea;
    QVector<QPair<int, QColor>> m_lineHighlights;
    QColor adjustedHighlightColor(const QColor& color) const;
};

/**
 * @brief Line number area widget for CodeEditor
 */
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor);
    
    QSize sizeHint() const override;
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    CodeEditor* m_codeEditor;
};

/**
 * @brief Side-by-side diff preview widget
 * 
 * Shows "Current" and "After deletion" views with synchronized scrolling
 * and syntax highlighting.
 */
class DiffPreview : public QWidget {
    Q_OBJECT
    
public:
    explicit DiffPreview(QWidget* parent = nullptr);
    ~DiffPreview() = default;
    
    /**
     * @brief Set the original file content
     */
    void setOriginal(const QString& content, const QString& language);
    
    /**
     * @brief Set the modified file content
     */
    void setModified(const QString& content);
    
    /**
     * @brief Set both contents from an edit result
     */
    void setFromEditResult(const EditResult& result, const QString& language);
    
    /**
     * @brief Highlight the comment region
     */
    void highlightComment(const Comment& comment);
    
    /**
     * @brief Highlight diff regions
     */
    void highlightDiff(const EditResult& result);
    
    /**
     * @brief Scroll to show a specific line
     */
    void scrollToLine(int line);
    
    /**
     * @brief Clear all content
     */
    void clear();
    
    /**
     * @brief Get the original editor
     */
    CodeEditor* originalEditor() const { return m_originalEditor; }
    
    /**
     * @brief Get the modified editor
     */
    CodeEditor* modifiedEditor() const { return m_modifiedEditor; }
    
private slots:
    void syncScroll();
    
private:
    void setupUI();
    
    QSplitter* m_splitter;
    CodeEditor* m_originalEditor;
    CodeEditor* m_modifiedEditor;
    SyntaxHighlighter* m_originalHighlighter;
    SyntaxHighlighter* m_modifiedHighlighter;
    
    bool m_syncingScroll = false;
};

#endif // DIFF_PREVIEW_H
