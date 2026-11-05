#ifndef TEXT_FILE_H
#define TEXT_FILE_H

#include <QString>
#include <QVector>
#include <algorithm>

/// UTF-8 text file whose line endings and BOM survive a read/modify/write cycle.
/// In memory, lines are always separated by '\n'.
struct TextFile {
    QString text;
    bool crlf = false;  // file used "\r\n" line endings
    bool bom = false;   // file started with a UTF-8 byte order mark

    /// Returns false (and sets *error) if the file cannot be read.
    bool read(const QString& path, QString* error = nullptr);
    /// Atomically replaces the file (temp file + rename).
    bool write(const QString& path, QString* error = nullptr) const;
};

/// Maps character offsets to 1-based lines / 0-based columns in O(log n).
class LineIndex {
public:
    explicit LineIndex(QStringView text) : m_size(int(text.size())) {
        m_starts.append(0);
        for (int i = 0; i < m_size; ++i)
            if (text[i] == u'\n') m_starts.append(i + 1);
    }
    int lineCount() const { return int(m_starts.size()); }
    int line(int offset) const {
        return int(std::upper_bound(m_starts.begin(), m_starts.end(), offset) - m_starts.begin());
    }
    int column(int offset) const { return offset - m_starts[line(offset) - 1]; }
    int lineStart(int line) const { return m_starts[line - 1]; }
    int lineEnd(int line) const { return line < lineCount() ? m_starts[line] - 1 : m_size; }
    /// Offset of (line, column), clamped to the line; -1 if the line does not exist.
    int offset(int line, int column) const {
        if (line < 1 || line > lineCount()) return -1;
        return std::min(lineStart(line) + std::max(column, 0), lineEnd(line));
    }

private:
    QVector<int> m_starts;
    int m_size;
};

#endif // TEXT_FILE_H
