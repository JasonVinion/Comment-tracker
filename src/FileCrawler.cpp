#include "FileCrawler.h"
#include "LanguageRegistry.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QtConcurrent>

const QStringList FileCrawler::DEFAULT_IGNORE_PATTERNS = {
    ".git",
    ".svn",
    ".hg",
    ".bzr",
    "node_modules",
    "vendor",
    "build",
    "dist",
    "out",
    "bin",
    "obj",
    "target",
    ".idea",
    ".vscode",
    ".vs",
    "__pycache__",
    "*.pyc",
    "*.pyo",
    ".DS_Store",
    "Thumbs.db",
    "*.min.js",
    "*.min.css",
    "*.map",
    "*.lock",
    "package-lock.json",
    "yarn.lock",
    "*.log"
};

FileCrawler::FileCrawler(QObject* parent)
    : QObject(parent) {
    addIgnorePatterns(DEFAULT_IGNORE_PATTERNS);
}

void FileCrawler::setRootPath(const QString& path) {
    m_rootPath = path;
}

void FileCrawler::addIgnorePatterns(const QStringList& patterns) {
    QMutexLocker locker(&m_mutex);
    m_ignorePatterns.append(patterns);
}

void FileCrawler::clearIgnorePatterns() {
    QMutexLocker locker(&m_mutex);
    m_ignorePatterns.clear();
}

void FileCrawler::setIncludeExtensions(const QStringList& extensions) {
    QMutexLocker locker(&m_mutex);
    m_includeExtensions.clear();
    for (const QString& ext : extensions) {
        QString e = ext.toLower();
        if (e.startsWith('.')) {
            e = e.mid(1);
        }
        m_includeExtensions.insert(e);
    }
}

void FileCrawler::startScan() {
    if (m_running) {
        return;
    }
    
    if (m_rootPath.isEmpty()) {
        emit error("No root path specified");
        return;
    }
    
    QDir dir(m_rootPath);
    if (!dir.exists()) {
        emit error("Root path does not exist: " + m_rootPath);
        return;
    }
    
    m_running = true;
    m_cancelled = false;
    m_filesScanned = 0;
    m_codeFilesFound = 0;
    m_gitignoreByDir.clear();
    
    // Start async scan
    m_scanFuture = QtConcurrent::run([this]() {
        scanDirectory(m_rootPath);
        
        if (m_cancelled) {
            emit cancelled();
        } else {
            emit scanComplete(m_filesScanned, m_codeFilesFound);
        }
        m_running = false;
    });
}

void FileCrawler::cancel() {
    m_cancelled = true;
}

void FileCrawler::scanDirectory(const QString& path, int depth) {
    if (m_cancelled) {
        return;
    }
    
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }
    
    // Load .gitignore if present
    if (m_useGitignore) {
        loadGitignore(path);
    }
    
    // Get all entries
    QFileInfoList entries = dir.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::DirsFirst
    );
    
    for (const QFileInfo& entry : entries) {
        if (m_cancelled) {
            return;
        }
        
        QString fullPath = entry.absoluteFilePath();
        QString relativePath = QDir(m_rootPath).relativeFilePath(fullPath);
        
        // Check ignore patterns
        if (shouldIgnore(relativePath)) {
            continue;
        }
        
        if (entry.isDir()) {
            // Recurse into subdirectory
            scanDirectory(fullPath, depth + 1);
        } else if (entry.isFile()) {
            m_filesScanned++;
            
            // Check if it's a code file
            if (isCodeFile(fullPath)) {
                // Skip binary files
                if (!isBinaryFile(fullPath)) {
                    m_codeFilesFound++;
                    emit fileFound(fullPath);
                }
            }
            
            // Report progress periodically
            if (m_filesScanned % 100 == 0) {
                emit progress(m_filesScanned, m_codeFilesFound, relativePath);
            }
        }
    }
}

bool FileCrawler::shouldIgnore(const QString& path) const {
    // Check built-in patterns
    {
        QMutexLocker locker(&m_mutex);
        for (const QString& pattern : m_ignorePatterns) {
            if (matchesPattern(path, pattern)) {
                return true;
            }
        }
    }
    
    // Check gitignore patterns - only patterns from parent directories apply
    // Walk up the path hierarchy and check patterns from each gitignore
    QFileInfo pathInfo(QDir(m_rootPath).filePath(path));
    QString currentDir = pathInfo.isDir() ? path : pathInfo.path();
    
    // Check patterns from each ancestor directory's .gitignore
    while (!currentDir.isEmpty() && currentDir != ".") {
        QString fullDirPath = QDir(m_rootPath).filePath(currentDir);
        auto it = m_gitignoreByDir.find(fullDirPath);
        if (it != m_gitignoreByDir.end()) {
            // Get relative path from the gitignore directory
            QString relativeToDirPath = QDir(fullDirPath).relativeFilePath(
                QDir(m_rootPath).filePath(path));
            for (const QString& pattern : it.value()) {
                if (matchesPattern(relativeToDirPath, pattern) || 
                    matchesPattern(path, pattern)) {
                    return true;
                }
            }
        }
        
        // Move to parent directory
        int lastSlash = currentDir.lastIndexOf('/');
        if (lastSlash > 0) {
            currentDir = currentDir.left(lastSlash);
        } else {
            break;
        }
    }
    
    // Also check root .gitignore
    auto rootIt = m_gitignoreByDir.find(m_rootPath);
    if (rootIt != m_gitignoreByDir.end()) {
        for (const QString& pattern : rootIt.value()) {
            if (matchesPattern(path, pattern)) {
                return true;
            }
        }
    }
    
    return false;
}

bool FileCrawler::matchesPattern(const QString& path, const QString& pattern) const {
    // Simple glob matching
    QString regexPattern = pattern;
    
    // Handle directory patterns
    if (pattern.endsWith('/')) {
        // Match directory name anywhere in path
        QString dirName = pattern.left(pattern.length() - 1);
        return path.contains("/" + dirName + "/") || 
               path.startsWith(dirName + "/") ||
               path == dirName;
    }
    
    // Handle simple patterns without wildcards
    if (!pattern.contains('*') && !pattern.contains('?')) {
        // Match as path component
        return path.contains("/" + pattern) || 
               path.startsWith(pattern + "/") ||
               path.endsWith("/" + pattern) ||
               path == pattern;
    }
    
    // Convert glob to regex
    regexPattern.replace(".", "\\.");
    regexPattern.replace("**", ".*");
    regexPattern.replace("*", "[^/]*");
    regexPattern.replace("?", "[^/]");
    
    // Match against full path or filename
    QRegularExpression regex("(^|/)" + regexPattern + "($|/)");
    return regex.match(path).hasMatch() || 
           regex.match(QFileInfo(path).fileName()).hasMatch();
}

void FileCrawler::loadGitignore(const QString& dirPath) {
    QString gitignorePath = dirPath + "/.gitignore";
    QFile file(gitignorePath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList patterns;
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            
            // Skip empty lines and comments
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }
            
            // Remove leading slash (absolute patterns become relative to gitignore location)
            if (line.startsWith('/')) {
                line = line.mid(1);
            }
            
            patterns.append(line);
        }
        file.close();
        
        // Store patterns associated with this directory
        if (!patterns.isEmpty()) {
            m_gitignoreByDir[dirPath] = patterns;
        }
    }
}

bool FileCrawler::isCodeFile(const QString& filePath) const {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    
    // Check include filter if set
    {
        QMutexLocker locker(&m_mutex);
        if (!m_includeExtensions.isEmpty()) {
            return m_includeExtensions.contains(ext);
        }
    }
    
    // Check language registry using full path to support extension-less/special-name files
    return LanguageRegistry::instance().getLanguageByPath(filePath) != nullptr;
}

bool FileCrawler::isBinaryFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return true;  // Can't open, treat as binary
    }
    
    // Read first 8KB to check for binary content
    QByteArray data = file.read(8192);
    file.close();
    
    if (data.isEmpty()) {
        return false;  // Empty file is not binary
    }
    
    // Check for null bytes (strong indicator of binary)
    if (data.contains('\0')) {
        return true;
    }
    
    // Check ratio of non-printable characters
    int nonPrintable = 0;
    for (char c : data) {
        unsigned char uc = static_cast<unsigned char>(c);
        // Allow common control characters (tab, newline, carriage return)
        if (uc < 32 && uc != 9 && uc != 10 && uc != 13) {
            nonPrintable++;
        }
        // Also check for high non-UTF8 bytes
        if (uc >= 128 && uc < 192) {
            // Could be valid UTF-8 continuation byte, check context
            // For simplicity, we'll allow these
        }
    }
    
    // If more than 10% non-printable, consider binary
    double ratio = static_cast<double>(nonPrintable) / data.size();
    return ratio > 0.1;
}
