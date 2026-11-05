#include "DiffPreview.h"
#include "SyntaxHighlighter.h"
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QTextBlock>
#include <QVBoxLayout>

namespace {
class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return {m_editor->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent* event) override { m_editor->paintLineNumbers(event); }

private:
    CodeEditor* m_editor;
};

/// Blends a highlight colour into the editor background so it works in light and dark themes.
QColor blend(const QColor& base, const QColor& color) {
    constexpr double ratio = 0.35;
    QColor c(int(base.red() + (color.red() - base.red()) * ratio), int(base.green() + (color.green() - base.green()) * ratio),
             int(base.blue() + (color.blue() - base.blue()) * ratio));
    c.setAlpha(200);
    return c;
}
} // namespace

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent), m_lineNumberArea(new LineNumberArea(this)), m_highlighter(new SyntaxHighlighter(document())) {
    QFont font(QStringLiteral("Consolas"), 10);
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(u' '));
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    connect(this, &CodeEditor::blockCountChanged, this, [this] { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); });
    connect(this, &CodeEditor::updateRequest, this, [this](const QRect& rect, int dy) {
        if (dy) m_lineNumberArea->scroll(0, dy);
        else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    });
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::setContent(const QString& text, const QString& language) {
    m_highlighter->setDocument(nullptr);  // highlight once, after the text is in place
    setPlainText(text);
    m_highlighter->setLanguage(language);
    m_highlighter->setDocument(document());
}

int CodeEditor::lineNumberAreaWidth() const {
    return 10 + fontMetrics().horizontalAdvance(u'9') * int(QString::number(qMax(1, blockCount())).size());
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height());
}

void CodeEditor::paintLineNumbers(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));
    painter.setPen(palette().color(QPalette::PlaceholderText));
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid() && top <= event->rect().bottom()) {
        const int bottom = top + qRound(blockBoundingRect(block).height());
        if (block.isVisible() && bottom >= event->rect().top())
            painter.drawText(0, top, m_lineNumberArea->width() - 5, fontMetrics().height(), Qt::AlignRight,
                             QString::number(block.blockNumber() + 1));
        block = block.next();
        top = bottom;
    }
}

void CodeEditor::highlightLines(const QVector<QPair<int, int>>& ranges, const QColor& color) {
    QList<QTextEdit::ExtraSelection> selections;
    QTextEdit::ExtraSelection sel;
    sel.format.setBackground(blend(palette().color(QPalette::Base), color));
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);
    for (const auto& [first, last] : ranges) {
        for (int line = first; line <= last; ++line) {  // full-width selections only span one block each
            const QTextBlock block = document()->findBlockByNumber(line - 1);
            if (!block.isValid()) break;
            sel.cursor = QTextCursor(block);
            selections << sel;
        }
    }
    setExtraSelections(selections);
}

void CodeEditor::scrollToLine(int line) {
    const QTextBlock block = document()->findBlockByNumber(qMax(0, line - 1));
    if (!block.isValid()) return;
    setTextCursor(QTextCursor(block));
    centerCursor();
}

DiffPreview::DiffPreview(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    for (auto [editor, title] : {std::pair{&m_original, tr("Current")}, std::pair{&m_modified, tr("After Deletion")}}) {
        auto* pane = new QWidget;
        auto* paneLayout = new QVBoxLayout(pane);
        paneLayout->setContentsMargins(0, 0, 0, 0);
        auto* label = new QLabel(QStringLiteral("<b>%1</b>").arg(title));
        label->setAlignment(Qt::AlignCenter);
        paneLayout->addWidget(label);
        paneLayout->addWidget(*editor = new CodeEditor);
        splitter->addWidget(pane);
    }
    layout->addWidget(splitter);

    auto sync = [this](CodeEditor* from, CodeEditor* to) {
        connect(from->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, to](int value) {
            if (m_syncing) return;
            m_syncing = true;
            to->verticalScrollBar()->setValue(value);
            m_syncing = false;
        });
    };
    sync(m_original, m_modified);
    sync(m_modified, m_original);
}

void DiffPreview::showDiff(const QString& original, const QString& modified, const QString& language,
                           const QVector<EditRange>& removed, int focusLine) {
    m_syncing = true;
    m_original->setContent(original, language);
    m_modified->setContent(modified, language);
    QVector<QPair<int, int>> lines;
    for (const EditRange& r : removed) lines.append({r.startLine, r.endLine});
    m_original->highlightLines(lines, QColor(255, 200, 200));
    m_original->scrollToLine(focusLine);
    m_modified->scrollToLine(focusLine);
    m_syncing = false;
}

void DiffPreview::clear() {
    for (CodeEditor* editor : {m_original, m_modified}) {
        editor->setExtraSelections({});
        editor->clear();
    }
}
