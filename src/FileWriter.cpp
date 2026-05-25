#include "FileWriter.h"
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryFile>

FileWriter::FileWriter(QObject* parent)
    : QObject(parent) {
}

void FileWriter::setBackupDirectory(const QString& path) {
    m_backupDir = path;
}

void FileWriter::setAuditLogPath(const QString& path) {
    m_auditLogPath = path;
}

bool FileWriter::applyEdits(const QString& filePath, const EditResult& result) {
    if (m_dryRun) {
        logAudit(filePath, "DRY_RUN", "Would modify file", true);
        return true;
    }
    
    // Check if file is writable
    if (!isWritable(filePath)) {
        m_lastError = "File is not writable: " + filePath;
        emit error(filePath, m_lastError);
        logAudit(filePath, "MODIFY", "Skipped - not writable", false, m_lastError);
        return false;
    }
    
    // Create backup if enabled
    QString backupPath;
    if (m_backupsEnabled) {
        backupPath = createBackup(filePath);
        if (backupPath.isEmpty()) {
            m_lastError = "Failed to create backup for: " + filePath;
            emit error(filePath, m_lastError);
            logAudit(filePath, "MODIFY", "Skipped - backup failed", false, m_lastError);
            return false;
        }
    }
    
    // Write atomically
    if (!writeAtomic(filePath, result.modifiedContent)) {
        emit error(filePath, m_lastError);
        logAudit(filePath, "MODIFY", "Write failed", false, m_lastError);
        
        // Restore from backup if we have one
        if (!backupPath.isEmpty()) {
            restoreFromBackup(filePath, backupPath);
        }
        return false;
    }
    
    int commentCount = result.appliedEdits.size();
    logAudit(filePath, "MODIFY", QString("Removed %1 comment(s)").arg(commentCount), true);
    emit fileModified(filePath);
    
    return true;
}

int FileWriter::applyAllEdits(const QMap<QString, EditResult>& results) {
    int successCount = 0;
    int totalCount = results.size();
    int current = 0;
    
    for (auto it = results.begin(); it != results.end(); ++it) {
        if (applyEdits(it.key(), it.value())) {
            successCount++;
        }
        current++;
        emit progress(current, totalCount);
    }
    
    // Export audit log
    if (!m_auditLogPath.isEmpty()) {
        exportAuditLog();
    }
    
    return successCount;
}

QString FileWriter::createBackup(const QString& filePath) {
    QString backupPath = generateBackupPath(filePath);
    
    // Ensure backup directory exists
    QFileInfo fi(backupPath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = "Could not create backup directory: " + dir.path();
            return QString();
        }
    }
    
    // Copy file to backup
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    
    if (!QFile::copy(filePath, backupPath)) {
        m_lastError = "Could not copy file to backup location";
        return QString();
    }
    
    m_backupMap[filePath] = backupPath;
    logAudit(filePath, "BACKUP", "Created backup at " + backupPath, true);
    emit backupCreated(filePath, backupPath);
    
    return backupPath;
}

bool FileWriter::restoreFromBackup(const QString& filePath, const QString& backupPath) {
    if (!QFile::exists(backupPath)) {
        m_lastError = "Backup file does not exist: " + backupPath;
        return false;
    }
    
    // Remove current file
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            m_lastError = "Could not remove file for restoration";
            return false;
        }
    }
    
    // Copy backup to original location
    if (!QFile::copy(backupPath, filePath)) {
        m_lastError = "Could not restore from backup";
        return false;
    }
    
    logAudit(filePath, "RESTORE", "Restored from " + backupPath, true);
    return true;
}

bool FileWriter::exportAuditLog(const QString& filePath) {
    QString logPath = filePath.isEmpty() ? m_auditLogPath : filePath;
    
    if (logPath.isEmpty()) {
        // Generate default path
        logPath = QDir::current().filePath("comment-explorer-audit.log");
    }
    
    QFile file(logPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_lastError = "Could not open audit log file: " + logPath;
        return false;
    }
    
    QTextStream out(&file);
    out << "\n=== Comment Explorer Session: " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ===\n\n";
    
    for (const AuditLogEntry& entry : m_auditLog) {
        out << entry.timestamp.toString(Qt::ISODate) << " | "
            << entry.operation << " | "
            << (entry.success ? "SUCCESS" : "FAILED") << " | "
            << entry.filePath << " | "
            << entry.details;
        
        if (!entry.errorMessage.isEmpty()) {
            out << " | ERROR: " << entry.errorMessage;
        }
        
        out << "\n";
    }
    
    file.close();
    return true;
}

bool FileWriter::writeAtomic(const QString& filePath, const QString& content) {
    QSaveFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Could not open file for writing: " + saveFile.errorString();
        return false;
    }
    
    QTextStream out(&saveFile);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    
    // Ensure all data is written
    out.flush();
    
    if (!saveFile.commit()) {
        m_lastError = "Could not write to file: " + saveFile.errorString();
        return false;
    }
    
    return true;
}

bool FileWriter::isWritable(const QString& filePath) {
    QFileInfo fi(filePath);
    
    if (!fi.exists()) {
        return false;
    }
    
    if (!fi.isWritable()) {
        return false;
    }
    
    // Also check directory is writable (for temp file)
    QFileInfo dirInfo(fi.dir().path());
    if (!dirInfo.isWritable()) {
        return false;
    }
    
    return true;
}

void FileWriter::logAudit(const QString& filePath, const QString& operation,
                          const QString& details, bool success, const QString& error) {
    AuditLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.filePath = filePath;
    entry.operation = operation;
    entry.details = details;
    entry.success = success;
    entry.errorMessage = error;
    
    m_auditLog.append(entry);
}

QString FileWriter::generateBackupPath(const QString& filePath) {
    QFileInfo fi(filePath);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    
    QString backupDir = m_backupDir;
    if (backupDir.isEmpty()) {
        // Use .comment-explorer-backups in the file's directory
        backupDir = fi.dir().filePath(".comment-explorer-backups");
    }
    
    // Create a unique path component based on the file's full path to avoid collisions
    // When using a global backup directory, include a hash of the original path
    QString pathHash = QString::number(qHash(fi.absoluteFilePath()), 16);
    QString safeFileName = fi.fileName() + "_" + pathHash;
    
    return QDir(backupDir).filePath(safeFileName + "." + timestamp + ".bak");
}
