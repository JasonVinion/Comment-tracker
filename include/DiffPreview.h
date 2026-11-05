#ifndef DIFF_PREVIEW_H
#define DIFF_PREVIEW_H

#include <QPlainTextEdit>
#include "EditPlanner.h"

class SyntaxHighlighter;

/// Read-only code view with line numbers and highlighted line ranges.
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);

    void setContent(const QString& text, const QString& language);
    /// Highlights whole lines; each pair is an inclusive 1-based [first, last] range.
    void highlightLines(const QVector<QPair<int, int>>& ranges, const QColor& color);
    void scrollToLine(int line);

    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent* event);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget* m_lineNumberArea;
    SyntaxHighlighter* m_highlighter;
};

/// Side-by-side "Current" / "After Deletion" view with synchronised scrolling.
class DiffPreview : public QWidget {
    Q_OBJECT

public:
    explicit DiffPreview(QWidget* parent = nullptr);

    void showDiff(const QString& original, const QString& modified, const QString& language,
                  const QVector<EditRange>& removed, int focusLine);
    void clear();

private:
    CodeEditor* m_original;
    CodeEditor* m_modified;
    bool m_syncing = false;
};

#endif // DIFF_PREVIEW_H
