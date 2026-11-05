#ifndef EDIT_PLANNER_H
#define EDIT_PLANNER_H

#include <QMap>
#include <QVector>
#include "Comment.h"
#include "TextFile.h"

/// A span of text to delete. Positions follow the Comment convention
/// (1-based lines, 0-based columns, exclusive end column).
struct EditRange {
    int commentId = 0;
    int startLine = 0, startColumn = 0;
    int endLine = 0, endColumn = 0;
    QString expectedText;  // when set, the file must still contain exactly this text at the span
};

/// Deletions grouped by absolute file path.
using EditPlan = QMap<QString, QVector<EditRange>>;

struct EditResult {
    TextFile original;
    QString modifiedContent;
    QString errorMessage;
    bool ok() const { return errorMessage.isEmpty(); }
};

/// Computes safe comment deletions.
///
/// Every range is removed and the affected line is tidied:
///  - nothing but whitespace left on the line   -> the whole line is removed
///  - only code before the comment              -> trailing whitespace is trimmed
///  - only code after the comment               -> the code keeps the line's indentation
///  - code on both sides                        -> joined with a single space if needed
/// Overlapping ranges are merged; code outside the ranges is never touched.
namespace EditPlanner {
EditRange rangeFor(const Comment& comment);
EditPlan plan(const QVector<const Comment*>& comments);
/// Returns the edited content, or the unchanged content with *error set if any range is invalid.
QString applyEdits(const QString& content, const QVector<EditRange>& ranges, QString* error = nullptr);
/// Reads the file and applies the ranges to it in memory.
EditResult preview(const QString& filePath, const QVector<EditRange>& ranges);
} // namespace EditPlanner

#endif // EDIT_PLANNER_H
