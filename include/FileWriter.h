#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QFile>
#include "EditPlanner.h"

/**
 * @brief Log entry for audit trail
 */
struct AuditLogEntry {
    QDateTime timestamp;
    QString filePath;
    QString operation;  // "BACKUP", "MODIFY", "DELETE"
    QString details;
    bool success;
    QString errorMessage;
};

/**
 * @brief Handles safe file writing with backups and audit logging
 * 
 * Features:
 * - Atomic writes (temp file + rename)
 * - Automatic backups
 * - Audit logging
 * - Dry-run mode
 * - Read-only file handling
 */
class FileWriter : public QObject {
    Q_OBJECT
    
public:
    explicit FileWriter(QObject* parent = nullptr);
    ~FileWriter() = default;
    
    /**
     * @brief Set backup directory (default: .comment-explorer-backups in root)
     */
    void setBackupDirectory(const QString& path);
    QString backupDirectory() const { return m_backupDir; }
    
    /**
     * @brief Enable/disable backups
     */
    void setBackupsEnabled(bool enabled) { m_backupsEnabled = enabled; }
    bool backupsEnabled() const { return m_backupsEnabled; }
    
    /**
     * @brief Enable/disable dry-run mode (no actual writes)
     */
    void setDryRun(bool dryRun) { m_dryRun = dryRun; }
    bool isDryRun() const { return m_dryRun; }
    
    /**
     * @brief Set audit log file path
     */
    void setAuditLogPath(const QString& path);
    QString auditLogPath() const { return m_auditLogPath; }
    
    /**
     * @brief Apply edits to a file
     * @param filePath Path to the file
     * @param result Edit result with modified content
     * @return True if successful
     */
    bool applyEdits(const QString& filePath, const EditResult& result);
    
    /**
     * @brief Apply edits to multiple files
     * @param results Map of file path to edit result
     * @return Number of files successfully modified
     */
    int applyAllEdits(const QMap<QString, EditResult>& results);
    
    /**
     * @brief Create backup of a file
     * @param filePath Original file path
     * @return Backup file path, or empty on failure
     */
    QString createBackup(const QString& filePath);
    
    /**
     * @brief Restore file from backup
     * @param filePath Original file path
     * @param backupPath Backup file path
     * @return True if successful
     */
    bool restoreFromBackup(const QString& filePath, const QString& backupPath);
    
    /**
     * @brief Get audit log entries
     */
    const QVector<AuditLogEntry>& auditLog() const { return m_auditLog; }
    
    /**
     * @brief Export audit log to file
     */
    bool exportAuditLog(const QString& filePath = QString());
    
    /**
     * @brief Get last error message
     */
    QString lastError() const { return m_lastError; }
    
signals:
    void fileModified(const QString& filePath);
    void backupCreated(const QString& originalPath, const QString& backupPath);
    void error(const QString& filePath, const QString& message);
    void progress(int completed, int total);
    
private:
    /**
     * @brief Write content to file atomically
     */
    bool writeAtomic(const QString& filePath, const QString& content);
    
    /**
     * @brief Check if file is writable
     */
    bool isWritable(const QString& filePath);
    
    /**
     * @brief Log an audit entry
     */
    void logAudit(const QString& filePath, const QString& operation, 
                  const QString& details, bool success, const QString& error = QString());
    
    /**
     * @brief Generate backup file path
     */
    QString generateBackupPath(const QString& filePath);
    
    QString m_backupDir;
    QString m_auditLogPath;
    bool m_backupsEnabled = true;
    bool m_dryRun = false;
    QString m_lastError;
    
    QVector<AuditLogEntry> m_auditLog;
    QMap<QString, QString> m_backupMap;  // original -> backup path
};

#endif // FILE_WRITER_H
