#ifndef EDIT_PLANNER_H
#define EDIT_PLANNER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include "Comment.h"

/**
 * @brief Represents a single edit operation
 */
struct EditOperation {
    int commentId;
    QString filePath;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;
    QString originalText;
    QString replacement;  // Usually empty for deletion
    bool isValid = true;
    QString validationMessage;
    
    // Edit type
    enum class Type {
        RemoveFullLine,     // Remove entire line(s)
        RemoveInline,       // Remove inline comment only
        RemoveBlock         // Remove block comment
    };
    Type type = Type::RemoveFullLine;
};

/**
 * @brief Result of applying edits to a file
 */
struct EditResult {
    QString filePath;
    QString originalContent;
    QString modifiedContent;
    QVector<EditOperation> appliedEdits;
    bool success = true;
    QString errorMessage;
    
    // Diff information
    struct DiffRegion {
        int originalStart;
        int originalEnd;
        int modifiedStart;
        int modifiedEnd;
        QString change;  // "removed", "modified"
    };
    QVector<DiffRegion> diffRegions;
};

/**
 * @brief Plans and validates safe comment deletions
 * 
 * This class computes safe edits for removing comments while ensuring
 * that code is not affected. It handles:
 * - Full-line comments: remove entire line if it becomes whitespace-only
 * - Inline comments: remove only the comment portion
 * - Block comments: remove from start to end delimiter
 * - Multiple edits per file (properly merged)
 */
class EditPlanner : public QObject {
    Q_OBJECT
    
public:
    explicit EditPlanner(QObject* parent = nullptr);
    ~EditPlanner() = default;
    
    /**
     * @brief Plan edits for a set of comments
     * @param comments Comments to delete
     * @return Map of file path to edit operations
     */
    QMap<QString, QVector<EditOperation>> planEdits(const QVector<Comment>& comments);
    
    /**
     * @brief Preview the result of applying edits to a file
     * @param filePath Path to the file
     * @param operations Edit operations to apply
     * @return Result with original and modified content
     */
    EditResult previewEdits(const QString& filePath, const QVector<EditOperation>& operations);
    
    /**
     * @brief Validate that an edit is safe
     * @param operation The edit operation
     * @return True if edit is safe
     */
    bool validateEdit(EditOperation& operation);
    
    /**
     * @brief Merge multiple operations for the same file
     * Operations must be sorted by position (end to start) to apply correctly
     */
    QVector<EditOperation> mergeOperations(QVector<EditOperation> operations);
    
    /**
     * @brief Apply edits to content (without writing to file)
     * @param content Original content
     * @param operations Edit operations (must be sorted end-to-start)
     * @return Modified content
     */
    QString applyEdits(const QString& content, const QVector<EditOperation>& operations);
    
signals:
    void editPlanned(const QString& filePath, int operationCount);
    void validationError(const QString& filePath, const QString& message);
    
private:
    /**
     * @brief Create edit for full-line comment
     */
    EditOperation createFullLineEdit(const Comment& comment, const QString& content);
    
    /**
     * @brief Create edit for inline comment
     */
    EditOperation createInlineEdit(const Comment& comment, const QString& content);
    
    /**
     * @brief Create edit for block comment
     */
    EditOperation createBlockEdit(const Comment& comment, const QString& content);
    
    /**
     * @brief Check if removing content creates syntax issues
     */
    bool checkSyntaxSafety(const QString& original, const QString& modified, int position);
    
    /**
     * @brief Get content of file (cached)
     */
    QString getFileContent(const QString& filePath);
    
    QMap<QString, QString> m_fileCache;
};

#endif // EDIT_PLANNER_H
