#ifndef COMMENT_H
#define COMMENT_H

#include <QString>
#include <QStringList>

/// A single comment extracted from a source file.
/// Kept deliberately small: path/language strings are implicitly shared between all
/// comments of a file, and the "clean" text is a view into rawText rather than a copy.
struct Comment {
    enum class Type : quint8 { SingleLine, MultiLine, DocComment, Inline };

    QString filePath;      // absolute path
    QString relativePath;  // relative to the scan root
    QString language;      // language name from the registry
    QString rawText;       // the comment exactly as it appears in the file
    QStringList keywords;  // detected tags (TODO, FIXME, ...)
    int id = 0;
    int startLine = 0, startColumn = 0;  // 1-based line, 0-based column
    int endLine = 0, endColumn = 0;      // end column is exclusive
    int cleanStart = 0, cleanLength = 0; // rawText span without delimiters and outer whitespace
    Type type = Type::SingleLine;
    bool isSelected = false;

    QStringView cleanView() const { return QStringView(rawText).mid(cleanStart, cleanLength); }
    QString cleanText() const { return cleanView().toString(); }

    QString preview(int maxLength = 50) const {
        // Only simplify a bounded prefix so huge block comments stay cheap to display.
        QString text = cleanView().left(maxLength * 4).toString().simplified();
        return text.size() > maxLength ? text.left(maxLength - 3) + QLatin1String("...") : text;
    }

    QString typeString() const {
        switch (type) {
            case Type::SingleLine: return QStringLiteral("Line");
            case Type::MultiLine: return QStringLiteral("Block");
            case Type::DocComment: return QStringLiteral("Doc");
            case Type::Inline: return QStringLiteral("Inline");
        }
        return QStringLiteral("Unknown");
    }

    QString keywordsString() const { return keywords.join(QLatin1String(", ")); }
};

#endif // COMMENT_H
