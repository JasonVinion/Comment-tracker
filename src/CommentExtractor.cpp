#include "CommentExtractor.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <tree_sitter/api.h>

extern "C" {
    const TSLanguage *tree_sitter_json(void);
    const TSLanguage *tree_sitter_cpp(void);
    const TSLanguage *tree_sitter_python(void);
    const TSLanguage *tree_sitter_javascript(void);
}

const QStringList CommentExtractor::DEFAULT_KEYWORDS = {
    "TODO",
    "FIXME",
    "XXX",
    "HACK",
    "NOTE",
    "BUG",
    "OPTIMIZE",
    "REVIEW",
    "WARNING",
    "DEPRECATED"
};

CommentExtractor::CommentExtractor(QObject* parent)
    : QObject(parent)
    , m_keywords(DEFAULT_KEYWORDS) {
}

void CommentExtractor::setKeywords(const QStringList& keywords) {
    m_keywords = keywords;
}

QVector<Comment> CommentExtractor::extractFromFile(const QString& filePath, const QString& relativePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit error(filePath, "Cannot open file: " + file.errorString());
        return {};
    }
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content;
    try {
        content = in.readAll();
    } catch (...) {
        emit error(filePath, "Failed to read file content");
        file.close();
        return {};
    }
    file.close();
    
    // Handle empty files
    if (content.isEmpty()) {
        return {};
    }
    
    // Get language config
    const LanguageConfig* config = LanguageRegistry::instance().getLanguageByPath(filePath);
    if (!config) {
        emit error(filePath, "Unsupported file type");
        return {};
    }
    
    QVector<Comment> comments;
    try {
        if (m_parserMethod == ParserMethod::TreeSitter) {
            comments = extractWithTreeSitter(content, *config, filePath, relativePath);
        } else {
            comments = extractWithLexer(content, *config, filePath, relativePath);
        }
    } catch (const std::exception& e) {
        emit error(filePath, QString("Extraction failed: %1").arg(e.what()));
        return {};
    } catch (...) {
        emit error(filePath, "Extraction failed with unknown error");
        return {};
    }
    
    emit fileProcessed(filePath, comments.size());
    return comments;
}

QVector<Comment> CommentExtractor::extractFromContent(const QString& content,
                                                       const QString& language,
                                                       const QString& filePath) {
    // Find language config by name
    const auto& registry = LanguageRegistry::instance();
    QStringList extensions = registry.getExtensionsForLanguage(language);
    if (extensions.isEmpty()) {
        return {};
    }
    
    const LanguageConfig* config = registry.getLanguageByExtension(extensions.first());
    if (!config) {
        return {};
    }
    
    if (m_parserMethod == ParserMethod::TreeSitter) {
        return extractWithTreeSitter(content, *config, filePath, filePath);
    } else {
        return extractWithLexer(content, *config, filePath, filePath);
    }
}

QVector<Comment> CommentExtractor::extractWithLexer(const QString& content,
                                                     const LanguageConfig& config,
                                                     const QString& filePath,
                                                     const QString& relativePath) {
    QVector<Comment> comments;
    int pos = 0;
    int length = content.length();
    
    while (pos < length) {
        // Skip whitespace
        while (pos < length && content[pos].isSpace()) {
            pos++;
        }
        
        if (pos >= length) break;
        
        // Try to skip string literals
        int newPos = skipString(content, pos, config);
        if (newPos > pos) {
            pos = newPos;
            continue;
        }
        
        // Try to skip raw strings
        newPos = skipRawString(content, pos, config);
        if (newPos > pos) {
            pos = newPos;
            continue;
        }
        
        // Try to skip regex literals (JavaScript, etc.)
        if (config.hasRegexLiterals) {
            newPos = skipRegex(content, pos, config);
            if (newPos > pos) {
                pos = newPos;
                continue;
            }
        }
        
        // Check for doc comments first (before regular comments)
        bool foundDocComment = false;
        
        // Check doc line comments
        for (const QString& marker : config.docLineMarkers) {
            if (pos + marker.length() <= length && 
                content.mid(pos, marker.length()) == marker) {
                // Found doc line comment
                int commentStart = pos;
                int lineEnd = content.indexOf('\n', pos);
                if (lineEnd == -1) lineEnd = length;
                
                Comment comment;
                comment.id = m_nextCommentId++;
                comment.filePath = filePath;
                comment.relativePath = relativePath;
                comment.language = config.name;
                comment.type = Comment::Type::DocComment;
                comment.rawText = content.mid(commentStart, lineEnd - commentStart);
                comment.cleanText = content.mid(commentStart + marker.length(), lineEnd - commentStart - marker.length()).trimmed();
                
                auto [startLine, startCol] = positionToLineColumn(content, commentStart);
                auto [endLine, endCol] = positionToLineColumn(content, lineEnd);
                comment.startLine = startLine;
                comment.startColumn = startCol;
                comment.endLine = endLine;
                comment.endColumn = endCol;
                
                // Check if inline
                if (isInlineComment(content, commentStart)) {
                    comment.type = Comment::Type::Inline;
                    int lineStart = content.lastIndexOf('\n', commentStart - 1) + 1;
                    comment.codeSegment = content.mid(lineStart, commentStart - lineStart).trimmed();
                    comment.commentSegment = comment.rawText;
                }
                
                comment.fullLines = getLine(content, startLine);
                extractContext(content, comment);
                detectKeywords(comment);
                
                comments.append(comment);
                pos = lineEnd + 1;
                foundDocComment = true;
                break;
            }
        }
        
        if (foundDocComment) continue;
        
        // Check doc block comments
        for (const auto& delim : config.docBlockDelimiters) {
            if (pos + delim.start.length() <= length &&
                content.mid(pos, delim.start.length()) == delim.start) {
                // Found doc block comment start
                int commentStart = pos;
                int endPos = content.indexOf(delim.end, pos + delim.start.length());
                if (endPos == -1) endPos = length - delim.end.length();
                endPos += delim.end.length();
                
                Comment comment;
                comment.id = m_nextCommentId++;
                comment.filePath = filePath;
                comment.relativePath = relativePath;
                comment.language = config.name;
                comment.type = Comment::Type::DocComment;
                comment.rawText = content.mid(commentStart, endPos - commentStart);
                comment.cleanText = content.mid(commentStart + delim.start.length(), 
                                                 endPos - commentStart - delim.start.length() - delim.end.length()).trimmed();
                
                auto [startLine, startCol] = positionToLineColumn(content, commentStart);
                auto [endLine, endCol] = positionToLineColumn(content, endPos);
                comment.startLine = startLine;
                comment.startColumn = startCol;
                comment.endLine = endLine;
                comment.endColumn = endCol;
                
                // Get all lines
                QString lines;
                for (int l = startLine; l <= endLine; l++) {
                    if (!lines.isEmpty()) lines += "\n";
                    lines += getLine(content, l);
                }
                comment.fullLines = lines;
                
                extractContext(content, comment);
                detectKeywords(comment);
                
                comments.append(comment);
                pos = endPos;
                foundDocComment = true;
                break;
            }
        }
        
        if (foundDocComment) continue;
        
        // Check for single-line comments
        QString marker = matchSingleLineStart(content, pos, config);
        if (!marker.isEmpty()) {
            int commentStart = pos;
            int lineEnd = content.indexOf('\n', pos);
            if (lineEnd == -1) lineEnd = length;
            
            Comment comment;
            comment.id = m_nextCommentId++;
            comment.filePath = filePath;
            comment.relativePath = relativePath;
            comment.language = config.name;
            comment.rawText = content.mid(commentStart, lineEnd - commentStart);
            comment.cleanText = content.mid(commentStart + marker.length(), lineEnd - commentStart - marker.length()).trimmed();
            
            auto [startLine, startCol] = positionToLineColumn(content, commentStart);
            auto [endLine, endCol] = positionToLineColumn(content, lineEnd);
            comment.startLine = startLine;
            comment.startColumn = startCol;
            comment.endLine = endLine;
            comment.endColumn = endCol;
            
            // Check if inline (code before comment on same line)
            if (isInlineComment(content, commentStart)) {
                comment.type = Comment::Type::Inline;
                int lineStart = content.lastIndexOf('\n', commentStart - 1) + 1;
                comment.codeSegment = content.mid(lineStart, commentStart - lineStart).trimmed();
                comment.commentSegment = comment.rawText;
            } else {
                comment.type = Comment::Type::SingleLine;
            }
            
            comment.fullLines = getLine(content, startLine);
            extractContext(content, comment);
            detectKeywords(comment);
            
            comments.append(comment);
            pos = lineEnd + 1;
            continue;
        }
        
        // Check for block comments
        auto [blockStart, blockEnd] = matchBlockStart(content, pos, config);
        if (!blockStart.isEmpty()) {
            int commentStart = pos;
            int endPos;
            
            if (config.supportsNestedComments) {
                // Handle nested comments
                int depth = 1;
                int searchPos = pos + blockStart.length();
                while (searchPos < length && depth > 0) {
                    int nextStart = content.indexOf(blockStart, searchPos);
                    int nextEnd = content.indexOf(blockEnd, searchPos);
                    
                    if (nextEnd == -1) {
                        endPos = length;
                        break;
                    }
                    
                    if (nextStart != -1 && nextStart < nextEnd) {
                        depth++;
                        searchPos = nextStart + blockStart.length();
                    } else {
                        depth--;
                        searchPos = nextEnd + blockEnd.length();
                        if (depth == 0) {
                            endPos = searchPos;
                        }
                    }
                }
            } else {
                endPos = content.indexOf(blockEnd, pos + blockStart.length());
                if (endPos == -1) {
                    endPos = length;
                } else {
                    endPos += blockEnd.length();
                }
            }
            
            Comment comment;
            comment.id = m_nextCommentId++;
            comment.filePath = filePath;
            comment.relativePath = relativePath;
            comment.language = config.name;
            comment.type = Comment::Type::MultiLine;
            comment.rawText = content.mid(commentStart, endPos - commentStart);
            comment.cleanText = content.mid(commentStart + blockStart.length(),
                                             endPos - commentStart - blockStart.length() - blockEnd.length()).trimmed();
            
            auto [startLine, startCol] = positionToLineColumn(content, commentStart);
            auto [endLine, endCol] = positionToLineColumn(content, endPos);
            comment.startLine = startLine;
            comment.startColumn = startCol;
            comment.endLine = endLine;
            comment.endColumn = endCol;
            
            // Check if inline
            if (isInlineComment(content, commentStart)) {
                comment.type = Comment::Type::Inline;
                int lineStart = content.lastIndexOf('\n', commentStart - 1) + 1;
                comment.codeSegment = content.mid(lineStart, commentStart - lineStart).trimmed();
                comment.commentSegment = comment.rawText;
            }
            
            // Get all lines
            QString lines;
            for (int l = startLine; l <= endLine; l++) {
                if (!lines.isEmpty()) lines += "\n";
                lines += getLine(content, l);
            }
            comment.fullLines = lines;
            
            extractContext(content, comment);
            detectKeywords(comment);
            
            comments.append(comment);
            pos = endPos;
            continue;
        }
        
        // Not in a comment or string, advance
        pos++;
    }
    
    return comments;
}

int CommentExtractor::skipString(const QString& content, int pos, const LanguageConfig& config) const {
    for (const QString& delim : config.stringDelimiters) {
        if (pos + delim.length() <= content.length() &&
            content.mid(pos, delim.length()) == delim) {
            
            // Handle triple-quoted strings (Python, etc.)
            if (delim.length() == 3) {
                int searchStart = pos + delim.length();
                while (true) {
                    int endPos = content.indexOf(delim, searchStart);
                    if (endPos == -1) return content.length();
                    
                    // Check for odd number of backslashes before endPos (escaped delimiter)
                    int backslashCount = 0;
                    for (int i = endPos - 1; i >= pos; i--) {
                        if (content[i] == '\\') backslashCount++;
                        else break;
                    }
                    
                    if (backslashCount % 2 == 0) {
                        return endPos + delim.length();
                    }
                    
                    // Escaped, continue search
                    searchStart = endPos + 1;
                }
            }
            
            // Handle generic multi-character delimiters (e.g., Lua [[ ... ]])
            if (delim.length() > 1) {
                int endPos = content.indexOf(delim, pos + delim.length());
                if (endPos == -1) {
                    // Unterminated string: consume to end of content
                    return content.length();
                }
                return endPos + delim.length();
            }
            
            // Single character delimiter
            QChar quote = delim[0];
            int searchPos = pos + 1;
            
            while (searchPos < content.length()) {
                QChar c = content[searchPos];
                
                if (c == '\\') {
                    // Skip escaped character
                    searchPos += 2;
                    continue;
                }
                
                if (c == quote) {
                    return searchPos + 1;
                }
                
                // Handle newlines (in most languages strings don't span lines)
                if (c == '\n' && quote != '`') {  // Template literals can span lines
                    return searchPos;
                }
                
                searchPos++;
            }
            
            return content.length();
        }
    }
    
    return pos;
}

int CommentExtractor::skipRawString(const QString& content, int pos, const LanguageConfig& config) const {
    for (const QString& marker : config.rawStringMarkers) {
        if (pos + marker.length() <= content.length() &&
            content.mid(pos, marker.length()) == marker) {
            
            // C++ raw string: R"delimiter(...)delimiter"
            // Find the delimiter
            int delimStart = pos + 2;  // After R"
            int parenPos = content.indexOf('(', delimStart);
            if (parenPos == -1) return pos;
            
            QString delimiter = content.mid(delimStart, parenPos - delimStart);
            QString endMarker = ")" + delimiter + "\"";
            
            int endPos = content.indexOf(endMarker, parenPos + 1);
            if (endPos == -1) return content.length();
            
            return endPos + endMarker.length();
        }
    }
    
    return pos;
}

int CommentExtractor::skipRegex(const QString& content, int pos, const LanguageConfig& config) const {
    Q_UNUSED(config);
    
    // Simple heuristic for JavaScript regex literals
    // A regex starts with / and is not followed by another / or *
    if (pos >= content.length() || content[pos] != '/') {
        return pos;
    }
    
    // Check it's not // or /*
    if (pos + 1 < content.length()) {
        QChar next = content[pos + 1];
        if (next == '/' || next == '*') {
            return pos;  // It's a comment, not regex
        }
    }
    
    // Try to detect if this is a regex by looking at what precedes it
    // This is a simplification - real parsing would need more context
    int prevNonSpace = pos - 1;
    while (prevNonSpace >= 0 && content[prevNonSpace].isSpace()) {
        prevNonSpace--;
    }
    
    if (prevNonSpace >= 0) {
        QChar prev = content[prevNonSpace];
        // If preceded by these characters, it's likely a regex
        QString regexPrecursors = "=:,([{!&|?;";
        if (!regexPrecursors.contains(prev)) {
            // Check for keywords like return, throw, etc.
            // Read backwards to get the word
            int wordEnd = prevNonSpace;
            int wordStart = wordEnd;
            while (wordStart >= 0 && (content[wordStart].isLetterOrNumber() || content[wordStart] == '_')) {
                wordStart--;
            }
            wordStart++;

            bool isKeyword = false;
            if (wordStart <= wordEnd) {
                QString word = content.mid(wordStart, wordEnd - wordStart + 1);
                static const QStringList keywords = {
                    "return", "throw", "yield", "case", "void", "typeof", "delete", "await"
                };
                if (keywords.contains(word)) {
                    isKeyword = true;
                }
            }

            if (!isKeyword) {
                return pos;  // Probably division operator
            }
        }
    }
    
    // Find the end of the regex
    int searchPos = pos + 1;
    while (searchPos < content.length()) {
        QChar c = content[searchPos];
        
        if (c == '\\') {
            searchPos += 2;  // Skip escaped character
            continue;
        }
        
        if (c == '[') {
            // Character class - find closing ]
            searchPos++;
            while (searchPos < content.length() && content[searchPos] != ']') {
                if (content[searchPos] == '\\') {
                    searchPos++;
                }
                searchPos++;
            }
            searchPos++;
            continue;
        }
        
        if (c == '/') {
            // End of regex - skip flags
            searchPos++;
            while (searchPos < content.length() && content[searchPos].isLetter()) {
                searchPos++;
            }
            return searchPos;
        }
        
        if (c == '\n') {
            return pos;  // Not a valid regex
        }
        
        searchPos++;
    }
    
    return pos;
}

QString CommentExtractor::matchSingleLineStart(const QString& content, int pos, const LanguageConfig& config) const {
    // Sort markers by length (longest first) to match correctly
    QStringList markers = config.singleLineMarkers;
    std::sort(markers.begin(), markers.end(), [](const QString& a, const QString& b) {
        return a.length() > b.length();
    });
    
    for (const QString& marker : markers) {
        if (pos + marker.length() <= content.length() &&
            content.mid(pos, marker.length()) == marker) {
            return marker;
        }
    }
    
    return QString();
}

QPair<QString, QString> CommentExtractor::matchBlockStart(const QString& content, int pos, const LanguageConfig& config) const {
    for (const auto& delim : config.blockDelimiters) {
        if (pos + delim.start.length() <= content.length() &&
            content.mid(pos, delim.start.length()) == delim.start) {
            return {delim.start, delim.end};
        }
    }
    
    return {QString(), QString()};
}

void CommentExtractor::extractContext(const QString& content, Comment& comment) const {
    // Get lines before
    QStringList linesBefore;
    for (int i = 1; i <= m_contextLines; i++) {
        int lineNum = comment.startLine - i;
        if (lineNum > 0) {
            linesBefore.prepend(getLine(content, lineNum));
        }
    }
    comment.contextBefore = linesBefore.join("\n");
    
    // Get lines after
    QStringList linesAfter;
    for (int i = 1; i <= m_contextLines; i++) {
        int lineNum = comment.endLine + i;
        QString line = getLine(content, lineNum);
        if (!line.isNull()) {
            linesAfter.append(line);
        }
    }
    comment.contextAfter = linesAfter.join("\n");
}

void CommentExtractor::detectKeywords(Comment& comment) const {
    QString text = comment.cleanText.toUpper();
    
    for (const QString& keyword : m_keywords) {
        // Match keyword at word boundary
        QRegularExpression regex("\\b" + QRegularExpression::escape(keyword.toUpper()) + "\\b");
        if (regex.match(text).hasMatch()) {
            comment.keywords.append(keyword);
        }
    }
}

QPair<int, int> CommentExtractor::positionToLineColumn(const QString& content, int pos) const {
    int line = 1;
    int column = 0;
    int lastNewline = -1;
    
    for (int i = 0; i < pos && i < content.length(); i++) {
        if (content[i] == '\n') {
            line++;
            lastNewline = i;
        }
    }
    
    column = pos - lastNewline - 1;
    return {line, column};
}

QString CommentExtractor::getLine(const QString& content, int lineNumber) const {
    if (lineNumber < 1) return QString();
    
    int currentLine = 1;
    int lineStart = 0;
    
    for (int i = 0; i < content.length(); i++) {
        if (currentLine == lineNumber) {
            int lineEnd = content.indexOf('\n', i);
            if (lineEnd == -1) lineEnd = content.length();
            return content.mid(lineStart, lineEnd - lineStart);
        }
        
        if (content[i] == '\n') {
            currentLine++;
            lineStart = i + 1;
        }
    }
    
    if (currentLine == lineNumber) {
        return content.mid(lineStart);
    }
    
    return QString();
}

bool CommentExtractor::isInlineComment(const QString& content, int commentStart) const {
    // Find start of line
    int lineStart = content.lastIndexOf('\n', commentStart - 1) + 1;
    
    // Check if there's non-whitespace code before the comment on this line
    QString before = content.mid(lineStart, commentStart - lineStart);
    return !before.trimmed().isEmpty();
}

QVector<Comment> CommentExtractor::extractWithTreeSitter(const QString& content,
                                                          const LanguageConfig& config,
                                                          const QString& filePath,
                                                          const QString& relativePath) {
    // Map language to TSLanguage
    const TSLanguage* tsLanguage = nullptr;
    if (config.name == "JSON" || config.name == "JSONC") {
        tsLanguage = tree_sitter_json();
    } else if (config.name == "C/C++") {
        tsLanguage = tree_sitter_cpp();
    } else if (config.name == "Python") {
        tsLanguage = tree_sitter_python();
    } else if (config.name == "JavaScript" || config.name == "TypeScript") {
        tsLanguage = tree_sitter_javascript();
    }

    if (!tsLanguage) {
        // Fall back to lexer if grammar not available for this language
        return extractWithLexer(content, config, filePath, relativePath);
    }

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, tsLanguage);

    // Use UTF-16 to match QString internal encoding
    const ushort* buffer = content.utf16();
    uint32_t length = content.length() * 2; // bytes

    TSTree* tree = ts_parser_parse_string_encoding(
        parser,
        NULL,
        reinterpret_cast<const char*>(buffer),
        length,
        TSInputEncodingUTF16
    );
    
    TSNode root_node = ts_tree_root_node(tree);
    QVector<Comment> comments;
    
    TSTreeCursor cursor = ts_tree_cursor_new(root_node);
    
    // Traverse the tree
    bool working = true;
    // Handle empty tree
    if (ts_node_is_null(root_node)) {
        working = false;
    }

    while (working) {
        TSNode node = ts_tree_cursor_current_node(&cursor);
        const char* type = ts_node_type(node);

        // Check if it is a comment
        // different grammars might use different names, but "comment" is standard.
        // C++ uses "comment"
        // JSON uses "comment"
        // Python uses "comment"
        bool isComment = (strcmp(type, "comment") == 0 || strcmp(type, "line_comment") == 0 || strcmp(type, "block_comment") == 0);
        
        // Special handling for Python docstrings (which are expression statements containing a string)
        if (!isComment && config.name == "Python" && strcmp(type, "string") == 0) {
            TSNode parent = ts_node_parent(node);
            if (!ts_node_is_null(parent) && strcmp(ts_node_type(parent), "expression_statement") == 0) {
                isComment = true;
            }
        }

        if (isComment) {
             uint32_t start_byte = ts_node_start_byte(node);
             uint32_t end_byte = ts_node_end_byte(node);
             TSPoint start_point = ts_node_start_point(node);
             TSPoint end_point = ts_node_end_point(node);

             Comment comment;
             comment.id = m_nextCommentId++;
             comment.filePath = filePath;
             comment.relativePath = relativePath;
             comment.language = config.name;

             // Convert byte offsets to QString indices (divide by 2 for UTF-16)
             int startIdx = start_byte / 2;
             int lengthChars = (end_byte - start_byte) / 2;

             comment.rawText = content.mid(startIdx, lengthChars);

             // Clean text - simple heuristic based on config
             QString clean = comment.rawText;
             // Remove block delimiters
             for (const auto& delim : config.blockDelimiters) {
                 if (clean.startsWith(delim.start) && clean.endsWith(delim.end)) {
                     clean = clean.mid(delim.start.length(), clean.length() - delim.start.length() - delim.end.length());
                     break;
                 }
             }
             // Remove single line markers
             for (const auto& marker : config.singleLineMarkers) {
                 if (clean.startsWith(marker)) {
                     clean = clean.mid(marker.length());
                     break;
                 }
             }
             comment.cleanText = clean.trimmed();

             // Tree-sitter columns are byte offsets. For UTF-16, divide by 2 to get character index.
             comment.startLine = start_point.row + 1;
             comment.startColumn = start_point.column / 2;
             comment.endLine = end_point.row + 1;
             comment.endColumn = end_point.column / 2;

             if (comment.startLine == comment.endLine) {
                 if (isInlineComment(content, startIdx)) {
                     comment.type = Comment::Type::Inline;
                     int lineStart = content.lastIndexOf('\n', startIdx - 1) + 1;
                     comment.codeSegment = content.mid(lineStart, startIdx - lineStart).trimmed();
                     comment.commentSegment = comment.rawText;
                 } else {
                     comment.type = Comment::Type::SingleLine;
                 }
             } else {
                 comment.type = Comment::Type::MultiLine;
             }

             // Check for doc comment
             bool isDoc = false;
             for (const auto& marker : config.docLineMarkers) {
                 if (comment.rawText.startsWith(marker)) {
                     comment.type = Comment::Type::DocComment;
                     isDoc = true;
                     break;
                 }
             }
             if (!isDoc) {
                 for (const auto& delim : config.docBlockDelimiters) {
                     if (comment.rawText.startsWith(delim.start)) {
                         comment.type = Comment::Type::DocComment;
                         break;
                     }
                 }
             }

             comment.fullLines = getLine(content, comment.startLine);
             if (comment.endLine > comment.startLine) {
                 for (int l = comment.startLine + 1; l <= comment.endLine; l++) {
                     QString line = getLine(content, l);
                     if (!line.isEmpty()) {
                         comment.fullLines += "\n" + line;
                     }
                 }
             }

             extractContext(content, comment);
             detectKeywords(comment);

             comments.append(comment);
        }

        // Depth-first traversal
        if (ts_tree_cursor_goto_first_child(&cursor)) {
            continue;
        }

        if (ts_tree_cursor_goto_next_sibling(&cursor)) {
            continue;
        }

        // Go up and next
        while (true) {
            if (!ts_tree_cursor_goto_parent(&cursor)) {
                working = false;
                break;
            }
            if (ts_tree_cursor_goto_next_sibling(&cursor)) {
                break;
            }
        }
    }

    ts_tree_cursor_delete(&cursor);
    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return comments;
}
