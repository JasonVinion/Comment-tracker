#include "FileWriter.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

bool FileWriter::apply(const QString& filePath, const QVector<EditRange>& ranges, QString* error) {
    // Re-read and re-validate right before writing, so a file edited since the
    // preview is never overwritten with stale content.
    EditResult result = EditPlanner::preview(filePath, ranges);
    QString problem = result.errorMessage;
    if (problem.isEmpty() && !QFileInfo(filePath).isWritable()) problem = QStringLiteral("file is not writable");
    if (problem.isEmpty() && m_backupsEnabled) createBackup(filePath, &problem);
    if (problem.isEmpty()) {
        result.original.text = std::move(result.modifiedContent);
        result.original.write(filePath, &problem);
    }
    log(filePath, QStringLiteral("MODIFY"),
        problem.isEmpty() ? QStringLiteral("Removed %1 comment(s)").arg(ranges.size()) : problem, problem.isEmpty());
    if (error) *error = problem;
    return problem.isEmpty();
}

QString FileWriter::createBackup(const QString& filePath, QString* error) {
    const QFileInfo fi(filePath);
    const QDir dir(m_backupDir.isEmpty() ? fi.dir().filePath(QStringLiteral(".comment-explorer-backups")) : m_backupDir);
    // A short hash of the full path keeps same-named files from different folders apart.
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(fi.absoluteFilePath().toUtf8(), QCryptographicHash::Md5).toHex().left(8));
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmsszzz"));
    QString path = dir.filePath(QStringLiteral("%1_%2.%3.bak").arg(fi.fileName(), hash, stamp));
    for (int i = 1; QFile::exists(path); ++i)  // never overwrite an earlier backup
        path = dir.filePath(QStringLiteral("%1_%2.%3-%4.bak").arg(fi.fileName(), hash, stamp).arg(i));

    if (!dir.mkpath(QStringLiteral("."))) *error = QStringLiteral("could not create backup folder ") + dir.path();
    else if (!QFile::copy(filePath, path)) *error = QStringLiteral("could not create backup at ") + path;
    else {
        log(filePath, QStringLiteral("BACKUP"), QStringLiteral("Created backup at ") + path, true);
        return path;
    }
    return QString();
}

void FileWriter::log(const QString& filePath, const QString& operation, const QString& details, bool success) {
    m_log << QStringLiteral("%1 | %2 | %3 | %4 | %5")
                 .arg(QDateTime::currentDateTime().toString(Qt::ISODate), operation,
                      success ? QStringLiteral("SUCCESS") : QStringLiteral("FAILED"), filePath, details);
}

bool FileWriter::exportAuditLog(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) return false;
    QTextStream out(&file);
    out << "\n=== Comment Explorer Session: " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ===\n\n";
    for (const QString& line : m_log) out << line << '\n';
    return out.status() == QTextStream::Ok;
}
