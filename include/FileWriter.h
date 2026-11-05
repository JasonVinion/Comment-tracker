#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <QStringList>
#include "EditPlanner.h"

/// Applies edit plans to disk: re-validates each file against the plan, makes a
/// backup, then replaces the file atomically. Every action is recorded in an audit log.
class FileWriter {
public:
    void setBackupsEnabled(bool enabled) { m_backupsEnabled = enabled; }
    /// Empty = a ".comment-explorer-backups" folder next to each file.
    void setBackupDirectory(const QString& path) { m_backupDir = path; }

    /// Returns true on success; otherwise *error explains why the file was left untouched.
    bool apply(const QString& filePath, const QVector<EditRange>& ranges, QString* error = nullptr);

    const QStringList& auditLog() const { return m_log; }
    bool exportAuditLog(const QString& path) const;

private:
    QString createBackup(const QString& filePath, QString* error);
    void log(const QString& filePath, const QString& operation, const QString& details, bool success);

    bool m_backupsEnabled = true;
    QString m_backupDir;
    QStringList m_log;
};

#endif // FILE_WRITER_H
