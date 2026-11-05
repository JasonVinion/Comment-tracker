#include "SettingsDialog.h"
#include "CommentExtractor.h"
#include "FileCrawler.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>

AppSettings AppSettings::defaults() {
    AppSettings s;
    s.ignorePatterns = FileCrawler::defaultIgnorePatterns();
    s.keywords = CommentExtractor::defaultKeywords();
    return s;
}

AppSettings AppSettings::load() {
    const AppSettings d = defaults();
    QSettings q;
    AppSettings s;
    s.ignorePatterns = q.value("FileCrawler/ignorePatterns", d.ignorePatterns).toStringList();
    s.useGitignore = q.value("FileCrawler/useGitignore", d.useGitignore).toBool();
    s.contextLines = q.value("Extractor/contextLines", d.contextLines).toInt();
    s.keywords = q.value("Extractor/keywords", d.keywords).toStringList();
    s.parserMethod = q.value("Extractor/parserMethod", d.parserMethod).toInt();
    s.backupsEnabled = q.value("FileWriter/backupsEnabled", d.backupsEnabled).toBool();
    s.backupDirectory = q.value("FileWriter/backupDirectory", d.backupDirectory).toString();
    return s;
}

void AppSettings::save() const {
    QSettings q;
    q.setValue("FileCrawler/ignorePatterns", ignorePatterns);
    q.setValue("FileCrawler/useGitignore", useGitignore);
    q.setValue("Extractor/contextLines", contextLines);
    q.setValue("Extractor/keywords", keywords);
    q.setValue("Extractor/parserMethod", parserMethod);
    q.setValue("FileWriter/backupsEnabled", backupsEnabled);
    q.setValue("FileWriter/backupDirectory", backupDirectory);
}

namespace {
/// A titled list with Add/Remove buttons.
QListWidget* addListEditor(QWidget* dialog, QBoxLayout* layout, const QString& title, const QString& prompt,
                           bool upperCase, QWidget* extra = nullptr) {
    auto* group = new QGroupBox(title);
    auto* box = new QVBoxLayout(group);
    auto* list = new QListWidget;
    list->setMaximumHeight(120);
    box->addWidget(list);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(QObject::tr("Add"));
    auto* remove = new QPushButton(QObject::tr("Remove"));
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    box->addLayout(buttons);
    if (extra) box->addWidget(extra);
    layout->addWidget(group);

    QObject::connect(add, &QPushButton::clicked, dialog, [=] {
        const QString text = QInputDialog::getText(dialog, title, prompt).trimmed();
        if (!text.isEmpty()) list->addItem(upperCase ? text.toUpper() : text);
    });
    QObject::connect(remove, &QPushButton::clicked, list, [list] { delete list->currentItem(); });
    return list;
}

QStringList items(const QListWidget* list) {
    QStringList out;
    for (int i = 0; i < list->count(); ++i) out << list->item(i)->text();
    return out;
}
} // namespace

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Settings"));
    setMinimumWidth(500);
    auto* layout = new QVBoxLayout(this);

    m_useGitignore = new QCheckBox(tr("Also use .gitignore files"));
    m_ignorePatterns = addListEditor(this, layout, tr("Ignore Patterns"),
                                     tr("Enter pattern (e.g., *.log, node_modules):"), false, m_useGitignore);
    m_keywords = addListEditor(this, layout, tr("Keywords"), tr("Enter keyword (e.g., TODO, FIXME):"), true);

    auto* options = new QGroupBox(tr("Options"));
    auto* form = new QFormLayout(options);
    m_contextLines = new QSpinBox;
    m_contextLines->setRange(0, 10);
    form->addRow(tr("Harvest context lines:"), m_contextLines);
    m_parserMethod = new QComboBox;
    m_parserMethod->addItem(tr("Lexer-based (Fast)"), int(CommentExtractor::ParserMethod::Lexer));
    m_parserMethod->addItem(tr("Tree-sitter (Accurate)"), int(CommentExtractor::ParserMethod::TreeSitter));
    m_parserMethod->setToolTip(tr("Tree-sitter is used for C/C++, Python, JavaScript and JSON; "
                                  "other languages, and code it cannot parse, use the lexer."));
    form->addRow(tr("Extraction method:"), m_parserMethod);
    layout->addWidget(options);

    auto* backups = new QGroupBox(tr("Backups"));
    auto* backupLayout = new QVBoxLayout(backups);
    m_backupsEnabled = new QCheckBox(tr("Create backups before modifying files"));
    backupLayout->addWidget(m_backupsEnabled);
    auto* dirRow = new QHBoxLayout;
    m_backupDir = new QLineEdit;
    m_backupDir->setPlaceholderText(tr("(default: .comment-explorer-backups next to each file)"));
    auto* browse = new QPushButton(tr("Browse..."));
    dirRow->addWidget(m_backupDir);
    dirRow->addWidget(browse);
    backupLayout->addLayout(dirRow);
    layout->addWidget(backups);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Backup Directory"), m_backupDir->text());
        if (!dir.isEmpty()) m_backupDir->setText(dir);
    });

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                                         QDialogButtonBox::RestoreDefaults);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { values().save(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
            [this] { setValues(AppSettings::defaults()); });

    setValues(AppSettings::load());
}

void SettingsDialog::setValues(const AppSettings& s) {
    m_ignorePatterns->clear();
    m_ignorePatterns->addItems(s.ignorePatterns);
    m_useGitignore->setChecked(s.useGitignore);
    m_keywords->clear();
    m_keywords->addItems(s.keywords);
    m_contextLines->setValue(s.contextLines);
    m_parserMethod->setCurrentIndex(std::max(0, m_parserMethod->findData(s.parserMethod)));
    m_backupsEnabled->setChecked(s.backupsEnabled);
    m_backupDir->setText(s.backupDirectory);
}

AppSettings SettingsDialog::values() const {
    AppSettings s;
    s.ignorePatterns = items(m_ignorePatterns);
    s.useGitignore = m_useGitignore->isChecked();
    s.keywords = items(m_keywords);
    s.contextLines = m_contextLines->value();
    s.parserMethod = m_parserMethod->currentData().toInt();
    s.backupsEnabled = m_backupsEnabled->isChecked();
    s.backupDirectory = m_backupDir->text().trimmed();
    return s;
}
