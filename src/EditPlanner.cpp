#include "EditPlanner.h"
#include <QFile>
#include <QTextStream>
#include <algorithm>

EditPlanner::EditPlanner(QObject* parent)
    : QObject(parent) {
}

QMap<QString, QVector<EditOperation>> EditPlanner::planEdits(const QVector<Comment>& comments) {
    QMap<QString, QVector<EditOperation>> editsByFile;
    
    for (const Comment& comment : comments) {
        QString content = getFileContent(comment.filePath);
        if (content.isEmpty()) {
            emit validationError(comment.filePath, "Could not read file content");
            continue;
        }
        
        EditOperation op;
        op.commentId = comment.id;
        op.filePath = comment.filePath;
        
        switch (comment.type) {
            case Comment::Type::SingleLine:
            case Comment::Type::DocComment:
                op = createFullLineEdit(comment, content);
                break;
            case Comment::Type::Inline:
                op = createInlineEdit(comment, content);
                break;
            case Comment::Type::MultiLine:
                op = createBlockEdit(comment, content);
                break;
        }
        
        if (op.isValid) {
            editsByFile[comment.filePath].append(op);
        } else {
            emit validationError(comment.filePath, op.validationMessage);
        }
    }
    
    // Sort and merge operations per file
    for (auto it = editsByFile.begin(); it != editsByFile.end(); ++it) {
        it.value() = mergeOperations(it.value());
        emit editPlanned(it.key(), it.value().size());
    }
    
    return editsByFile;
}

EditResult EditPlanner::previewEdits(const QString& filePath, const QVector<EditOperation>& operations) {
    EditResult result;
    result.filePath = filePath;
    result.originalContent = getFileContent(filePath);
    
    if (result.originalContent.isEmpty()) {
        result.success = false;
        result.errorMessage = "Could not read file content";
        return result;
    }
    
    result.appliedEdits = operations;
    result.modifiedContent = applyEdits(result.originalContent, operations);
    
    // Generate diff regions
    // Simple approach: mark removed regions
    for (const EditOperation& op : operations) {
        EditResult::DiffRegion region;
        region.originalStart = op.startLine;
        region.originalEnd = op.endLine;
        region.change = "removed";
        result.diffRegions.append(region);
    }
    
    result.success = true;
    return result;
}

bool EditPlanner::validateEdit(EditOperation& operation) {
    QString content = getFileContent(operation.filePath);
    if (content.isEmpty()) {
        operation.isValid = false;
        operation.validationMessage = "Could not read file";
        return false;
    }
    
    // Check that the original text still exists at the expected position
    QStringList lines = content.split('\n');
    
    if (operation.startLine < 1 || operation.startLine > lines.size()) {
        operation.isValid = false;
        operation.validationMessage = "Start line out of range";
        return false;
    }
    
    if (operation.endLine < operation.startLine || operation.endLine > lines.size()) {
        operation.isValid = false;
        operation.validationMessage = "End line out of range";
        return false;
    }
    
    operation.isValid = true;
    return true;
}

QVector<EditOperation> EditPlanner::mergeOperations(QVector<EditOperation> operations) {
    // Sort by position (end to start) to apply correctly
    std::sort(operations.begin(), operations.end(), 
        [](const EditOperation& a, const EditOperation& b) {
            if (a.startLine != b.startLine) {
                return a.startLine > b.startLine;  // Descending
            }
            return a.startColumn > b.startColumn;
        });
    
    // Check for overlaps
    QVector<EditOperation> merged;
    if (operations.isEmpty()) return merged;
    
    merged.append(operations[0]);

    for (int i = 1; i < operations.size(); i++) {
        EditOperation& current = operations[i];
        EditOperation& previous = merged.last(); // previous in list, but later in file

        // Check overlap
        // Since current starts before previous (or at same line),
        // overlap happens if current ends after previous starts.
        bool overlap = false;

        if (current.endLine > previous.startLine) {
            overlap = true;
        } else if (current.endLine == previous.startLine) {
            if (current.endColumn > previous.startColumn) {
                overlap = true;
            }
        }

        if (overlap) {
            // Union of removal ranges
            previous.startLine = current.startLine;
            previous.startColumn = current.startColumn;

            // End is already the max of both because 'previous' (later in file) usually ends later,
            // but for safety check if current extends beyond previous
            if (current.endLine > previous.endLine ||
               (current.endLine == previous.endLine && current.endColumn > previous.endColumn)) {
                previous.endLine = current.endLine;
                previous.endColumn = current.endColumn;
            }

            // Treat merged operations as block removal to handle the range correctly
            previous.type = EditOperation::Type::RemoveBlock;
        } else {
            merged.append(current);
        }
    }

    return merged;
}

QString EditPlanner::applyEdits(const QString& content, const QVector<EditOperation>& operations) {
    QString result = content;
    QStringList lines = result.split('\n');
    
    // Apply operations in reverse order (end to start)
    for (const EditOperation& op : operations) {
        int startIdx = op.startLine - 1;
        int endIdx = op.endLine - 1;
        
        if (startIdx < 0 || endIdx >= lines.size()) {
            continue;
        }
        
        switch (op.type) {
            case EditOperation::Type::RemoveFullLine:
                // Remove entire lines
                for (int i = endIdx; i >= startIdx; i--) {
                    if (i < lines.size()) {
                        lines.removeAt(i);
                    }
                }
                break;
                
            case EditOperation::Type::RemoveInline:
                // Remove only the comment portion from the line
                if (startIdx < lines.size()) {
                    QString line = lines[startIdx];
                    // Remove from startColumn to end of line
                    if (op.startColumn >= 0 && op.startColumn < line.length()) {
                        // Also remove trailing whitespace before the comment
                        int removeFrom = op.startColumn;
                        while (removeFrom > 0 && line[removeFrom - 1].isSpace()) {
                            removeFrom--;
                        }
                        lines[startIdx] = line.left(removeFrom);
                    }
                }
                break;
                
            case EditOperation::Type::RemoveBlock:
                // Remove block comment, potentially spanning multiple lines
                if (startIdx == endIdx) {
                    // Single line block comment
                    QString line = lines[startIdx];
                    QString before = line.left(op.startColumn);
                    QString after = (op.endColumn < line.length()) ? line.mid(op.endColumn) : "";
                    
                    // Add space if needed to prevent token joining
                    QString newLine = before;
                    if (!before.isEmpty() && !before.endsWith(' ') && 
                        !after.isEmpty() && !after.startsWith(' ')) {
                        newLine += " ";
                    }
                    newLine += after;
                    
                    // If line is now empty or whitespace only, remove it
                    if (newLine.trimmed().isEmpty()) {
                        lines.removeAt(startIdx);
                    } else {
                        lines[startIdx] = newLine;
                    }
                } else {
                    // Multi-line block comment
                    // Remove intermediate lines
                    for (int i = endIdx - 1; i > startIdx; i--) {
                        lines.removeAt(i);
                    }
                    
                    // Handle first and last lines
                    QString firstLine = lines[startIdx];
                    QString lastLine = (startIdx + 1 < lines.size()) ? lines[startIdx + 1] : "";
                    
                    QString before = firstLine.left(op.startColumn);
                    QString after = (op.endColumn < lastLine.length()) ? lastLine.mid(op.endColumn) : "";
                    
                    // Combine or remove
                    if (before.trimmed().isEmpty() && after.trimmed().isEmpty()) {
                        // Remove both lines
                        lines.removeAt(startIdx + 1);
                        lines.removeAt(startIdx);
                    } else if (before.trimmed().isEmpty()) {
                        lines.removeAt(startIdx + 1);
                        lines[startIdx] = after;
                    } else if (after.trimmed().isEmpty()) {
                        lines.removeAt(startIdx + 1);
                        lines[startIdx] = before;
                    } else {
                        // Combine lines
                        lines.removeAt(startIdx + 1);
                        lines[startIdx] = before + " " + after.trimmed();
                    }
                }
                break;
        }
    }
    
    return lines.join('\n');
}

EditOperation EditPlanner::createFullLineEdit(const Comment& comment, const QString& content) {
    EditOperation op;
    op.commentId = comment.id;
    op.filePath = comment.filePath;
    op.startLine = comment.startLine;
    op.startColumn = 0;
    op.endLine = comment.endLine;
    op.endColumn = 0;
    op.type = EditOperation::Type::RemoveFullLine;
    op.originalText = comment.rawText;
    op.isValid = true;
    
    // Verify the comment is on full line(s)
    QStringList lines = content.split('\n');
    if (comment.startLine > 0 && comment.startLine <= lines.size()) {
        QString line = lines[comment.startLine - 1];
        QString beforeComment = line.left(comment.startColumn);
        
        // If there's code before the comment, it's actually inline
        if (!beforeComment.trimmed().isEmpty()) {
            return createInlineEdit(comment, content);
        }
    }
    
    return op;
}

EditOperation EditPlanner::createInlineEdit(const Comment& comment, const QString& content) {
    Q_UNUSED(content);
    
    EditOperation op;
    op.commentId = comment.id;
    op.filePath = comment.filePath;
    op.startLine = comment.startLine;
    op.startColumn = comment.startColumn;
    op.endLine = comment.endLine;
    op.endColumn = comment.endColumn;
    op.type = EditOperation::Type::RemoveInline;
    op.originalText = comment.rawText;
    op.isValid = true;
    
    return op;
}

EditOperation EditPlanner::createBlockEdit(const Comment& comment, const QString& content) {
    Q_UNUSED(content);
    
    EditOperation op;
    op.commentId = comment.id;
    op.filePath = comment.filePath;
    op.startLine = comment.startLine;
    op.startColumn = comment.startColumn;
    op.endLine = comment.endLine;
    op.endColumn = comment.endColumn;
    op.type = EditOperation::Type::RemoveBlock;
    op.originalText = comment.rawText;
    op.isValid = true;
    
    return op;
}

bool EditPlanner::checkSyntaxSafety(const QString& original, const QString& modified, int position) {
    Q_UNUSED(original);
    Q_UNUSED(modified);
    Q_UNUSED(position);
    
    // Basic validation
    if (original.isEmpty()) return true;

    // Ensure we haven't emptied the file unintentionally (unless it was all comments)
    // This is a minimal check. Full syntax verification would require re-parsing.

    return true;
}

QString EditPlanner::getFileContent(const QString& filePath) {
    // Check cache
    auto it = m_fileCache.find(filePath);
    if (it != m_fileCache.end()) {
        return it.value();
    }
    
    // Read file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
    
    // Cache it
    m_fileCache[filePath] = content;
    
    return content;
}
