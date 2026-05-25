#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>

const QStringList SettingsDialog::DEFAULT_IGNORE_PATTERNS = {
    ".git",
    ".svn",
    "node_modules",
    "vendor",
    "build",
    "dist",
    "out",
    "bin",
    "obj",
    "target",
    "__pycache__",
    "*.min.js",
    "*.min.css"
};

const QStringList SettingsDialog::DEFAULT_KEYWORDS = {
    "TODO",
    "FIXME",
    "XXX",
    "HACK",
    "NOTE",
    "BUG",
    "OPTIMIZE",
    "REVIEW",
    "WARNING",
    "DEPRECATED"
};

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Settings");
    setMinimumWidth(500);
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Ignore patterns group
    QGroupBox* ignoreGroup = new QGroupBox("Ignore Patterns");
    QVBoxLayout* ignoreLayout = new QVBoxLayout(ignoreGroup);
    
    m_ignorePatternsEdit = new QListWidget();
    m_ignorePatternsEdit->setMaximumHeight(120);
    ignoreLayout->addWidget(m_ignorePatternsEdit);
    
    QHBoxLayout* ignoreButtonLayout = new QHBoxLayout();
    QPushButton* addIgnoreBtn = new QPushButton("Add");
    QPushButton* removeIgnoreBtn = new QPushButton("Remove");
    ignoreButtonLayout->addWidget(addIgnoreBtn);
    ignoreButtonLayout->addWidget(removeIgnoreBtn);
    ignoreButtonLayout->addStretch();
    ignoreLayout->addLayout(ignoreButtonLayout);
    
    m_useGitignoreCheck = new QCheckBox("Also use .gitignore files");
    ignoreLayout->addWidget(m_useGitignoreCheck);
    
    mainLayout->addWidget(ignoreGroup);
    
    connect(addIgnoreBtn, &QPushButton::clicked, this, &SettingsDialog::onAddIgnorePattern);
    connect(removeIgnoreBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveIgnorePattern);
    
    // Keywords group
    QGroupBox* keywordGroup = new QGroupBox("Keywords");
    QVBoxLayout* keywordLayout = new QVBoxLayout(keywordGroup);
    
    m_keywordsEdit = new QListWidget();
    m_keywordsEdit->setMaximumHeight(120);
    keywordLayout->addWidget(m_keywordsEdit);
    
    QHBoxLayout* keywordButtonLayout = new QHBoxLayout();
    QPushButton* addKeywordBtn = new QPushButton("Add");
    QPushButton* removeKeywordBtn = new QPushButton("Remove");
    keywordButtonLayout->addWidget(addKeywordBtn);
    keywordButtonLayout->addWidget(removeKeywordBtn);
    keywordButtonLayout->addStretch();
    keywordLayout->addLayout(keywordButtonLayout);
    
    mainLayout->addWidget(keywordGroup);
    
    connect(addKeywordBtn, &QPushButton::clicked, this, &SettingsDialog::onAddKeyword);
    connect(removeKeywordBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveKeyword);
    
    // Context group
    QGroupBox* contextGroup = new QGroupBox("Context");
    QHBoxLayout* contextLayout = new QHBoxLayout(contextGroup);
    
    QLabel* contextLabel = new QLabel("Lines of context:");
    m_contextLinesSpin = new QSpinBox();
    m_contextLinesSpin->setRange(0, 10);
    m_contextLinesSpin->setValue(2);
    contextLayout->addWidget(contextLabel);
    contextLayout->addWidget(m_contextLinesSpin);
    contextLayout->addStretch();
    
    mainLayout->addWidget(contextGroup);
    
    // Parser method group
    QGroupBox* parserGroup = new QGroupBox("Extraction Method");
    QHBoxLayout* parserLayout = new QHBoxLayout(parserGroup);
    
    QLabel* parserLabel = new QLabel("Comment extraction method:");
    m_parserMethodCombo = new QComboBox();
    m_parserMethodCombo->addItem("Lexer-based (Fast)", static_cast<int>(ParserMethod::Lexer));
    m_parserMethodCombo->addItem("Tree-sitter (Accurate)", static_cast<int>(ParserMethod::TreeSitter));
    m_parserMethodCombo->setToolTip("Lexer-based: Fast but may have edge cases in complex constructs.\n"
                                    "Tree-sitter: More accurate parsing but requires additional setup.");
    parserLayout->addWidget(parserLabel);
    parserLayout->addWidget(m_parserMethodCombo);
    parserLayout->addStretch();
    
    mainLayout->addWidget(parserGroup);
    
    // Backup group
    QGroupBox* backupGroup = new QGroupBox("Backups");
    QVBoxLayout* backupLayout = new QVBoxLayout(backupGroup);
    
    m_backupsEnabledCheck = new QCheckBox("Create backups before modifying files");
    m_backupsEnabledCheck->setChecked(true);
    backupLayout->addWidget(m_backupsEnabledCheck);
    
    QHBoxLayout* backupDirLayout = new QHBoxLayout();
    QLabel* backupDirLabel = new QLabel("Backup directory:");
    m_backupDirEdit = new QLineEdit();
    m_backupDirEdit->setPlaceholderText("(default: .comment-explorer-backups)");
    QPushButton* browseBtn = new QPushButton("Browse...");
    backupDirLayout->addWidget(backupDirLabel);
    backupDirLayout->addWidget(m_backupDirEdit);
    backupDirLayout->addWidget(browseBtn);
    backupLayout->addLayout(backupDirLayout);
    
    mainLayout->addWidget(backupGroup);
    
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseBackupDir);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* resetBtn = new QPushButton("Reset to Defaults");
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addStretch();
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    buttonLayout->addWidget(buttonBox);
    
    mainLayout->addLayout(buttonLayout);
    
    connect(resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QStringList SettingsDialog::ignorePatterns() const {
    QStringList patterns;
    for (int i = 0; i < m_ignorePatternsEdit->count(); i++) {
        patterns.append(m_ignorePatternsEdit->item(i)->text());
    }
    return patterns;
}

void SettingsDialog::setIgnorePatterns(const QStringList& patterns) {
    m_ignorePatternsEdit->clear();
    m_ignorePatternsEdit->addItems(patterns);
}

bool SettingsDialog::useGitignore() const {
    return m_useGitignoreCheck->isChecked();
}

void SettingsDialog::setUseGitignore(bool use) {
    m_useGitignoreCheck->setChecked(use);
}

int SettingsDialog::contextLines() const {
    return m_contextLinesSpin->value();
}

void SettingsDialog::setContextLines(int lines) {
    m_contextLinesSpin->setValue(lines);
}

bool SettingsDialog::backupsEnabled() const {
    return m_backupsEnabledCheck->isChecked();
}

void SettingsDialog::setBackupsEnabled(bool enabled) {
    m_backupsEnabledCheck->setChecked(enabled);
}

QString SettingsDialog::backupDirectory() const {
    return m_backupDirEdit->text();
}

void SettingsDialog::setBackupDirectory(const QString& path) {
    m_backupDirEdit->setText(path);
}

QStringList SettingsDialog::keywords() const {
    QStringList keywords;
    for (int i = 0; i < m_keywordsEdit->count(); i++) {
        keywords.append(m_keywordsEdit->item(i)->text());
    }
    return keywords;
}

void SettingsDialog::setKeywords(const QStringList& keywords) {
    m_keywordsEdit->clear();
    m_keywordsEdit->addItems(keywords);
}

SettingsDialog::ParserMethod SettingsDialog::parserMethod() const {
    return static_cast<ParserMethod>(m_parserMethodCombo->currentData().toInt());
}

void SettingsDialog::setParserMethod(ParserMethod method) {
    int index = m_parserMethodCombo->findData(static_cast<int>(method));
    if (index >= 0) {
        m_parserMethodCombo->setCurrentIndex(index);
    }
}

void SettingsDialog::loadSettings() {
    QSettings settings;
    
    settings.beginGroup("FileCrawler");
    QStringList patterns = settings.value("ignorePatterns", DEFAULT_IGNORE_PATTERNS).toStringList();
    setIgnorePatterns(patterns);
    setUseGitignore(settings.value("useGitignore", true).toBool());
    settings.endGroup();
    
    settings.beginGroup("Extractor");
    setContextLines(settings.value("contextLines", 2).toInt());
    QStringList keywords = settings.value("keywords", DEFAULT_KEYWORDS).toStringList();
    setKeywords(keywords);
    int parserMethodInt = settings.value("parserMethod", static_cast<int>(ParserMethod::TreeSitter)).toInt();
    setParserMethod(static_cast<ParserMethod>(parserMethodInt));
    settings.endGroup();
    
    settings.beginGroup("FileWriter");
    setBackupsEnabled(settings.value("backupsEnabled", true).toBool());
    setBackupDirectory(settings.value("backupDirectory", "").toString());
    settings.endGroup();
}

void SettingsDialog::saveSettings() {
    QSettings settings;
    
    settings.beginGroup("FileCrawler");
    settings.setValue("ignorePatterns", ignorePatterns());
    settings.setValue("useGitignore", useGitignore());
    settings.endGroup();
    
    settings.beginGroup("Extractor");
    settings.setValue("contextLines", contextLines());
    settings.setValue("keywords", keywords());
    settings.setValue("parserMethod", static_cast<int>(parserMethod()));
    settings.endGroup();
    
    settings.beginGroup("FileWriter");
    settings.setValue("backupsEnabled", backupsEnabled());
    settings.setValue("backupDirectory", backupDirectory());
    settings.endGroup();
}

void SettingsDialog::onAddIgnorePattern() {
    bool ok;
    QString pattern = QInputDialog::getText(this, "Add Ignore Pattern",
        "Enter pattern (e.g., *.log, node_modules):", QLineEdit::Normal, "", &ok);
    if (ok && !pattern.isEmpty()) {
        m_ignorePatternsEdit->addItem(pattern);
    }
}

void SettingsDialog::onRemoveIgnorePattern() {
    QListWidgetItem* item = m_ignorePatternsEdit->currentItem();
    if (item) {
        delete item;
    }
}

void SettingsDialog::onAddKeyword() {
    bool ok;
    QString keyword = QInputDialog::getText(this, "Add Keyword",
        "Enter keyword (e.g., TODO, FIXME):", QLineEdit::Normal, "", &ok);
    if (ok && !keyword.isEmpty()) {
        m_keywordsEdit->addItem(keyword.toUpper());
    }
}

void SettingsDialog::onRemoveKeyword() {
    QListWidgetItem* item = m_keywordsEdit->currentItem();
    if (item) {
        delete item;
    }
}

void SettingsDialog::onBrowseBackupDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Directory",
        m_backupDirEdit->text());
    if (!dir.isEmpty()) {
        m_backupDirEdit->setText(dir);
    }
}

void SettingsDialog::onResetDefaults() {
    setIgnorePatterns(DEFAULT_IGNORE_PATTERNS);
    setUseGitignore(true);
    setContextLines(2);
    setBackupsEnabled(true);
    setBackupDirectory("");
    setKeywords(DEFAULT_KEYWORDS);
    setParserMethod(ParserMethod::TreeSitter);
}
