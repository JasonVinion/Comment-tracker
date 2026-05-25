#ifndef FILE_CRAWLER_H
#define FILE_CRAWLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QSet>
#include <QFileInfo>
#include <QFuture>
#include <QMutex>
#include <atomic>

/**
 * @brief Recursively scans directories for source code files
 * 
 * Features:
 * - Async operation with progress reporting
 * - Cancellation support
 * - Configurable ignore patterns (glob-style)
 * - .gitignore support
 * - Binary file detection
 */
class FileCrawler : public QObject {
    Q_OBJECT
    
public:
    explicit FileCrawler(QObject* parent = nullptr);
    ~FileCrawler() = default;
    
    /**
     * @brief Set the root path to scan
     */
    void setRootPath(const QString& path);
    
    /**
     * @brief Get the current root path
     */
    QString rootPath() const { return m_rootPath; }
    
    /**
     * @brief Add ignore patterns (glob-style)
     * Examples: "*.log", "node_modules", "build/*"
     */
    void addIgnorePatterns(const QStringList& patterns);
    
    /**
     * @brief Clear all ignore patterns
     */
    void clearIgnorePatterns();
    
    /**
     * @brief Enable/disable .gitignore parsing
     */
    void setUseGitignore(bool use) { m_useGitignore = use; }
    
    /**
     * @brief Set file extensions to include (empty = all supported)
     */
    void setIncludeExtensions(const QStringList& extensions);
    
    /**
     * @brief Start scanning (async)
     */
    void startScan();
    
    /**
     * @brief Cancel ongoing scan
     */
    void cancel();
    
    /**
     * @brief Check if scan is running
     */
    bool isRunning() const { return m_running; }
    
    /**
     * @brief Check if file is binary
     */
    static bool isBinaryFile(const QString& filePath);
    
signals:
    /**
     * @brief Emitted when a code file is found
     */
    void fileFound(const QString& filePath);
    
    /**
     * @brief Emitted to report scan progress
     * @param filesScanned Number of files scanned so far
     * @param codeFilesFound Number of code files found
     * @param currentPath Current path being scanned
     */
    void progress(int filesScanned, int codeFilesFound, const QString& currentPath);
    
    /**
     * @brief Emitted when scan is complete
     */
    void scanComplete(int totalFiles, int codeFiles);
    
    /**
     * @brief Emitted on error
     */
    void error(const QString& message);
    
    /**
     * @brief Emitted when scan is cancelled
     */
    void cancelled();
    
private:
    void scanDirectory(const QString& path, int depth = 0);
    bool shouldIgnore(const QString& path) const;
    bool matchesPattern(const QString& path, const QString& pattern) const;
    void loadGitignore(const QString& dirPath);
    bool isCodeFile(const QString& filePath) const;
    
    QString m_rootPath;
    QStringList m_ignorePatterns;
    QMap<QString, QStringList> m_gitignoreByDir;  // directory path -> patterns from that dir's .gitignore
    QSet<QString> m_includeExtensions;
    bool m_useGitignore = true;
    
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancelled{false};
    QFuture<void> m_scanFuture;
    
    int m_filesScanned = 0;
    int m_codeFilesFound = 0;
    
    mutable QMutex m_mutex;

    // Default ignore patterns
    static const QStringList DEFAULT_IGNORE_PATTERNS;
};

#endif // FILE_CRAWLER_H
