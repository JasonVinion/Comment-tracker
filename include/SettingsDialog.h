#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QSpinBox;

/// Application settings persisted with QSettings. Missing values fall back to defaults().
struct AppSettings {
    QStringList ignorePatterns;
    bool useGitignore = true;
    int contextLines = 2;        // lines of code around each group in the harvest report
    QStringList keywords;
    int parserMethod = 0;        // CommentExtractor::ParserMethod (0 = lexer, 1 = Tree-sitter)
    bool backupsEnabled = true;
    QString backupDirectory;     // empty = next to each modified file

    static AppSettings defaults();
    static AppSettings load();
    void save() const;
};

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void setValues(const AppSettings& settings);
    AppSettings values() const;

private:
    QListWidget* m_ignorePatterns;
    QCheckBox* m_useGitignore;
    QListWidget* m_keywords;
    QSpinBox* m_contextLines;
    QComboBox* m_parserMethod;
    QCheckBox* m_backupsEnabled;
    QLineEdit* m_backupDir;
};

#endif // SETTINGS_DIALOG_H
