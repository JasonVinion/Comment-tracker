#include "DiffPreview.h"
#include "SyntaxHighlighter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QPalette>

// LineNumberArea implementation

LineNumberArea::LineNumberArea(CodeEditor* editor)
    : QWidget(editor)
    , m_codeEditor(editor) {
}

QSize LineNumberArea::sizeHint() const {
    return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
    m_codeEditor->lineNumberAreaPaintEvent(event);
}

// CodeEditor implementation

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);
    
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    
    updateLineNumberAreaWidth(0);
    
    // Set monospace font
    QFont font("Consolas", 10);
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    
    // Set tab width
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    
    // Read-only by default
    setReadOnly(true);
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }
    
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* e) {
    QPlainTextEdit::resizeEvent(e);
    
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = adjustedHighlightColor(QColor(Qt::yellow).lighter(160));
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    // Add line highlights
    for (const auto& [line, color] : m_lineHighlights) {
        QTextBlock block = document()->findBlockByNumber(line - 1);
        if (block.isValid()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(adjustedHighlightColor(color));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            QTextCursor cursor(block);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            selection.cursor = cursor;
            extraSelections.append(selection);
        }
    }
    
    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));
    
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::darkGray);
            painter.drawText(0, top, m_lineNumberArea->width() - 5, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::highlightLines(int startLine, int endLine, const QColor& color) {
    for (int line = startLine; line <= endLine; line++) {
        m_lineHighlights.append({line, color});
    }
    highlightCurrentLine();
}

void CodeEditor::clearHighlights() {
    m_lineHighlights.clear();
    highlightCurrentLine();
}

QColor CodeEditor::adjustedHighlightColor(const QColor& color) const {
    QColor base = palette().color(QPalette::Base);
    constexpr qreal blendRatio = 0.35;
    QColor blended(
        static_cast<int>(base.red() + (color.red() - base.red()) * blendRatio),
        static_cast<int>(base.green() + (color.green() - base.green()) * blendRatio),
        static_cast<int>(base.blue() + (color.blue() - base.blue()) * blendRatio)
    );
    blended.setAlpha(200);
    return blended;
}

void CodeEditor::scrollToLine(int line) {
    QTextBlock block = document()->findBlockByNumber(line - 1);
    if (block.isValid()) {
        QTextCursor cursor(block);
        setTextCursor(cursor);
        centerCursor();
    }
}

// DiffPreview implementation

DiffPreview::DiffPreview(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void DiffPreview::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Headers
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* originalLabel = new QLabel("Current");
    QLabel* modifiedLabel = new QLabel("After Deletion");
    
    QFont boldFont = originalLabel->font();
    boldFont.setBold(true);
    originalLabel->setFont(boldFont);
    modifiedLabel->setFont(boldFont);
    
    originalLabel->setAlignment(Qt::AlignCenter);
    modifiedLabel->setAlignment(Qt::AlignCenter);
    
    headerLayout->addWidget(originalLabel);
    headerLayout->addWidget(modifiedLabel);
    layout->addLayout(headerLayout);
    
    // Editors
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    m_originalEditor = new CodeEditor(this);
    m_modifiedEditor = new CodeEditor(this);
    
    m_originalHighlighter = new SyntaxHighlighter(m_originalEditor->document());
    m_modifiedHighlighter = new SyntaxHighlighter(m_modifiedEditor->document());
    
    m_splitter->addWidget(m_originalEditor);
    m_splitter->addWidget(m_modifiedEditor);
    
    layout->addWidget(m_splitter);
    
    // Sync scrolling
    connect(m_originalEditor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffPreview::syncScroll);
    connect(m_modifiedEditor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffPreview::syncScroll);
}

void DiffPreview::setOriginal(const QString& content, const QString& language) {
    m_originalEditor->setPlainText(content);
    m_originalHighlighter->setLanguage(language);
}

void DiffPreview::setModified(const QString& content) {
    m_modifiedEditor->setPlainText(content);
}

void DiffPreview::setFromEditResult(const EditResult& result, const QString& language) {
    setOriginal(result.originalContent, language);
    setModified(result.modifiedContent);
    m_modifiedHighlighter->setLanguage(language);
    
    highlightDiff(result);
}

void DiffPreview::highlightComment(const Comment& comment) {
    m_originalEditor->clearHighlights();
    m_modifiedEditor->clearHighlights();
    m_originalEditor->highlightLines(comment.startLine, comment.endLine, QColor(255, 255, 200));
    m_originalEditor->scrollToLine(comment.startLine);
    m_modifiedEditor->scrollToLine(comment.startLine);
}

void DiffPreview::highlightDiff(const EditResult& result) {
    m_originalEditor->clearHighlights();
    
    for (const auto& region : result.diffRegions) {
        m_originalEditor->highlightLines(region.originalStart, region.originalEnd, QColor(255, 200, 200));
    }
    
    if (!result.diffRegions.isEmpty()) {
        m_originalEditor->scrollToLine(result.diffRegions.first().originalStart);
    }
}

void DiffPreview::scrollToLine(int line) {
    m_originalEditor->scrollToLine(line);
}

void DiffPreview::clear() {
    m_originalEditor->clear();
    m_modifiedEditor->clear();
    m_originalEditor->clearHighlights();
    m_modifiedEditor->clearHighlights();
}

void DiffPreview::syncScroll() {
    if (m_syncingScroll) return;
    
    m_syncingScroll = true;
    
    QScrollBar* sender = qobject_cast<QScrollBar*>(QObject::sender());
    if (sender == m_originalEditor->verticalScrollBar()) {
        m_modifiedEditor->verticalScrollBar()->setValue(sender->value());
    } else {
        m_originalEditor->verticalScrollBar()->setValue(sender->value());
    }
    
    m_syncingScroll = false;
}
