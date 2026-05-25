#ifndef LANGUAGE_REGISTRY_H
#define LANGUAGE_REGISTRY_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QRegularExpression>
#include <memory>

/**
 * @brief Defines how comments are structured in a programming language
 */
struct LanguageConfig {
    QString name;                      // Human-readable name
    QStringList extensions;            // File extensions (without dot)
    QStringList filenames;             // Exact filename matches (e.g., "Makefile", "Dockerfile")
    
    // Single-line comment markers
    QStringList singleLineMarkers;     // e.g., "//", "#"
    
    // Multi-line comment delimiters
    struct BlockDelimiter {
        QString start;                 // e.g., "/*"
        QString end;                   // e.g., "*/"
    };
    QVector<BlockDelimiter> blockDelimiters;
    
    // Doc comment markers (subset of above, for special handling)
    QStringList docLineMarkers;        // e.g., "///"
    QVector<BlockDelimiter> docBlockDelimiters; // e.g., "/**" ... "*/"
    
    // String delimiters (to avoid false positives)
    QStringList stringDelimiters;      // e.g., "\"", "'"
    QStringList rawStringMarkers;      // e.g., R" for C++
    
    // Regex literals (for languages like JavaScript)
    bool hasRegexLiterals = false;
    
    // Nested comments support
    bool supportsNestedComments = false;
    
    // Whether the language is supported by parser (vs regex fallback)
    bool parserSupported = false;
};

/**
 * @brief Registry mapping file extensions to language configurations
 * 
 * This class provides a central registry of programming languages and their
 * comment syntax. It's designed to be extensible without modifying the core
 * parsing pipeline.
 */
class LanguageRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static LanguageRegistry& instance();
    
    /**
     * @brief Register a new language configuration
     */
    void registerLanguage(const LanguageConfig& config);
    
    /**
     * @brief Get the language configuration for a file extension
     * @param extension File extension without dot (e.g., "cpp")
     * @return Language config or nullptr if not found
     */
    const LanguageConfig* getLanguageByExtension(const QString& extension) const;
    
    /**
     * @brief Get the language configuration for a file path
     * @param filePath Full or relative file path
     * @return Language config or nullptr if not found
     */
    const LanguageConfig* getLanguageByPath(const QString& filePath) const;
    
    /**
     * @brief Check if a file extension is registered
     */
    bool isSupported(const QString& extension) const;
    
    /**
     * @brief Get all registered language names
     */
    QStringList getLanguageNames() const;
    
    /**
     * @brief Get all registered extensions
     */
    QStringList getAllExtensions() const;
    
    /**
     * @brief Get extensions for a specific language
     */
    QStringList getExtensionsForLanguage(const QString& languageName) const;
    
private:
    LanguageRegistry();
    ~LanguageRegistry() = default;
    
    void initializeDefaultLanguages();
    
    QMap<QString, LanguageConfig> m_languagesByName;
    QMap<QString, QString> m_extensionToLanguage; // extension -> language name
    QMap<QString, QString> m_filenameToLanguage;  // filename -> language name
};

#endif // LANGUAGE_REGISTRY_H
