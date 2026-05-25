#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QSettings>

/**
 * @brief Settings dialog for configuring the application
 */
class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() = default;
    
    // Ignore patterns
    QStringList ignorePatterns() const;
    void setIgnorePatterns(const QStringList& patterns);
    
    // Gitignore
    bool useGitignore() const;
    void setUseGitignore(bool use);
    
    // Context lines
    int contextLines() const;
    void setContextLines(int lines);
    
    // Backups
    bool backupsEnabled() const;
    void setBackupsEnabled(bool enabled);
    QString backupDirectory() const;
    void setBackupDirectory(const QString& path);
    
    // Keywords
    QStringList keywords() const;
    void setKeywords(const QStringList& keywords);
    
    // Parser method
    enum class ParserMethod { Lexer, TreeSitter };
    ParserMethod parserMethod() const;
    void setParserMethod(ParserMethod method);
    
    // Load/save settings
    void loadSettings();
    void saveSettings();
    
private slots:
    void onAddIgnorePattern();
    void onRemoveIgnorePattern();
    void onAddKeyword();
    void onRemoveKeyword();
    void onBrowseBackupDir();
    void onResetDefaults();
    
private:
    void setupUI();
    
    // Ignore patterns
    QListWidget* m_ignorePatternsEdit;
    QCheckBox* m_useGitignoreCheck;
    
    // Context
    QSpinBox* m_contextLinesSpin;
    
    // Backups
    QCheckBox* m_backupsEnabledCheck;
    QLineEdit* m_backupDirEdit;
    
    // Keywords
    QListWidget* m_keywordsEdit;
    
    // Parser method
    QComboBox* m_parserMethodCombo;
    
    // Default values
    static const QStringList DEFAULT_IGNORE_PATTERNS;
    static const QStringList DEFAULT_KEYWORDS;
};

#endif // SETTINGS_DIALOG_H
