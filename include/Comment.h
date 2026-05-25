#ifndef COMMENT_H
#define COMMENT_H

#include <QString>
#include <QVector>

/**
 * @brief Represents a single comment extracted from source code
 */
struct Comment {
    // File information
    QString filePath;           // Absolute file path
    QString relativePath;       // Relative to scan root
    QString language;           // Detected language (e.g., "C++", "Python")
    
    // Position information
    int startLine = 0;          // 1-based line number
    int startColumn = 0;        // 0-based column
    int endLine = 0;            // 1-based line number
    int endColumn = 0;          // 0-based column
    
    // Comment content
    QString rawText;            // The comment text as it appears in file
    QString cleanText;          // Comment text without delimiters
    QString fullLines;          // Full line(s) containing the comment
    
    // Comment type
    enum class Type {
        SingleLine,             // // or # style
        MultiLine,              // /* */ style
        DocComment,             // /** */ or /// style
        Inline                  // Code followed by comment on same line
    };
    Type type = Type::SingleLine;
    
    // Keyword tags detected
    QStringList keywords;       // TODO, FIXME, NOTE, HACK, etc.
    
    // Context
    QString contextBefore;      // Lines before the comment
    QString contextAfter;       // Lines after the comment
    
    // For inline comments: the code portion of the line
    QString codeSegment;
    QString commentSegment;
    
    // Extraction method
    bool isFromFallback = false; // True if extracted via regex fallback
    
    // Selection state (for UI)
    bool isSelected = false;
    
    // Unique identifier
    int id = 0;
    
    /**
     * @brief Returns a preview of the comment (first N characters)
     */
    QString preview(int maxLength = 50) const {
        QString text = cleanText.simplified();
        if (text.length() > maxLength) {
            return text.left(maxLength - 3) + "...";
        }
        return text;
    }
    
    /**
     * @brief Returns the type as a string
     */
    QString typeString() const {
        switch (type) {
            case Type::SingleLine: return "Line";
            case Type::MultiLine: return "Block";
            case Type::DocComment: return "Doc";
            case Type::Inline: return "Inline";
        }
        return "Unknown";
    }
    
    /**
     * @brief Returns keywords as comma-separated string
     */
    QString keywordsString() const {
        return keywords.join(", ");
    }
};

/**
 * @brief Collection of comments with metadata
 */
struct CommentCollection {
    QVector<Comment> comments;
    QString rootPath;
    int totalFilesScanned = 0;
    int codeFilesFound = 0;
    int skippedFiles = 0;
    QStringList errors;
};

#endif // COMMENT_H
