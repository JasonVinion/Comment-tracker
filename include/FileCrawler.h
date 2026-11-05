#ifndef FILE_CRAWLER_H
#define FILE_CRAWLER_H

#include <QFuture>
#include <QStringList>

/// Recursively finds source files below a directory, honouring glob ignore
/// patterns and (optionally) .gitignore files. Runs on the global thread pool;
/// cancel the returned future to stop early. Progress is reported as the
/// number of files examined so far.
namespace FileCrawler {

struct Options {
    QString rootPath;
    QStringList ignorePatterns;      // gitignore syntax, relative to the root
    bool useGitignore = true;
    qint64 maxFileSize = 10 * 1024 * 1024;  // larger files are skipped
};

struct Result {
    QStringList files;  // absolute paths of supported, non-binary source files
    int totalFiles = 0; // files examined (not ignored)
};

QStringList defaultIgnorePatterns();
QFuture<Result> scan(const Options& options);
Result scanBlocking(const Options& options);

/// True if relativePath (using '/') is excluded by any of the gitignore-style patterns.
bool isIgnored(const QString& relativePath, bool isDir, const QStringList& patterns);
bool isBinaryFile(const QString& filePath);

} // namespace FileCrawler

#endif // FILE_CRAWLER_H
