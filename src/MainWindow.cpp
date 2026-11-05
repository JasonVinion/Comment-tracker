#include "MainWindow.h"
#include "CommentModel.h"
#include "DiffPreview.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    connect(&m_scanWatcher, &QFutureWatcherBase::progressValueChanged, this,
            [this](int files) { m_statusLabel->setText(tr("Scanning: %1 files examined...").arg(files)); });
    connect(&m_scanWatcher, &QFutureWatcherBase::finished, this, &MainWindow::onScanFinished);
    connect(&m_extractWatcher, &QFutureWatcherBase::progressRangeChanged, m_progress, &QProgressBar::setRange);
    connect(&m_extractWatcher, &QFutureWatcherBase::progressValueChanged, m_progress, &QProgressBar::setValue);
    connect(&m_extractWatcher, &QFutureWatcherBase::finished, this, &MainWindow::onExtractionFinished);
    m_flushTimer.setInterval(150);  // batch table inserts instead of one per file
    connect(&m_flushTimer, &QTimer::timeout, this, &MainWindow::flushExtracted);
    connect(m_model, &CommentModel::selectionChanged, this, [this](int selected, int total) {
        m_countsLabel->setText(tr("Selected: %1 / %2").arg(selected).arg(total));
        updateActions();
    });
    updateActions();
}

MainWindow::~MainWindow() { stopBackgroundWork(); }

QAction* MainWindow::makeAction(QMenu* menu, const QString& text, void (MainWindow::*slot)(), const QKeySequence& shortcut) {
    QAction* action = menu->addAction(text, this, slot);
    action->setShortcut(shortcut);
    return action;
}

void MainWindow::setupUi() {
    // Menus; the most common actions are also on the toolbar.
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* openAct = makeAction(fileMenu, tr("&Open Folder..."), &MainWindow::selectFolder, QKeySequence::Open);
    fileMenu->addSeparator();
    makeAction(fileMenu, tr("&Export Comments..."), &MainWindow::exportComments);
    m_harvestAct = makeAction(fileMenu, tr("&Harvest Special Comments..."), &MainWindow::harvestSpecialComments);
    makeAction(fileMenu, tr("Export Audit &Log..."), &MainWindow::exportAuditLog);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, this, &QWidget::close);
    QAction* settingsAct = makeAction(menuBar()->addMenu(tr("&Edit")), tr("&Settings..."), &MainWindow::openSettings);
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    m_rescanAct = makeAction(viewMenu, tr("&Rescan"), &MainWindow::rescan, QKeySequence::Refresh);
    m_cancelAct = makeAction(viewMenu, tr("&Cancel Scan"), &MainWindow::cancelScan, Qt::Key_Escape);
    makeAction(menuBar()->addMenu(tr("&Help")), tr("&About"), &MainWindow::showAbout);

    QToolBar* toolbar = addToolBar(tr("Main"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    openAct->setIconText(tr("Select Folder"));
    m_harvestAct->setIconText(tr("Harvest Comments"));
    m_cancelAct->setIconText(tr("Cancel"));
    toolbar->addActions({openAct, m_rescanAct, settingsAct, m_harvestAct, m_cancelAct});
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel(tr("Root: ")));
    toolbar->addWidget(m_pathLabel = new QLabel(tr("(none selected)")));

    // Left: filters, comment table, selection buttons.
    auto* left = new QWidget;
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto* filterRow = new QHBoxLayout;
    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Search comments..."));
    m_search->setClearButtonEnabled(true);
    m_keywordCombo = new QComboBox;
    m_keywordCombo->addItem(tr("All Keywords"));
    m_keywordCombo->addItems(AppSettings::load().keywords);
    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(tr("All Languages"));
    m_regexCheck = new QCheckBox(tr("Regex"));
    auto* customBtn = new QPushButton(tr("Custom Filter..."));
    auto* clearBtn = new QPushButton(tr("Clear"));
    filterRow->addWidget(m_search, 1);
    for (QWidget* w : std::initializer_list<QWidget*>{m_keywordCombo, m_languageCombo, m_regexCheck, customBtn, clearBtn})
        filterRow->addWidget(w);
    leftLayout->addLayout(filterRow);

    m_model = new CommentModel(this);
    m_proxy = new CommentFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_table = new QTableView;
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(-1, Qt::AscendingOrder);  // keep file order until the user sorts
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(m_table->fontMetrics().height() + 6);
    const int widths[] = {200, 50, 60, 80, 400, 80};
    for (int col = 0; col < CommentModel::ColCount; ++col) m_table->setColumnWidth(col, widths[col]);
    leftLayout->addWidget(m_table);

    auto* selectionRow = new QHBoxLayout;
    auto* selectAllBtn = new QPushButton(tr("Select All"));
    selectAllBtn->setToolTip(tr("Select every comment that matches the current filters"));
    auto* deselectAllBtn = new QPushButton(tr("Deselect All"));
    m_deleteBtn = new QPushButton(tr("Delete Selected..."));
    m_deleteBtn->setToolTip(tr("Preview the removal of all checked comments"));
    selectionRow->addWidget(selectAllBtn);
    selectionRow->addWidget(deselectAllBtn);
    selectionRow->addStretch();
    selectionRow->addWidget(m_deleteBtn);
    leftLayout->addLayout(selectionRow);

    // Right: preview and apply/discard.
    auto* right = new QWidget;
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_preview = new DiffPreview);
    auto* actionRow = new QHBoxLayout;
    actionRow->addStretch();
    actionRow->addWidget(m_applyBtn = new QPushButton(tr("Apply Changes")));
    actionRow->addWidget(m_discardBtn = new QPushButton(tr("Discard")));
    rightLayout->addLayout(actionRow);

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setSizes({600, 400});
    auto* central = new QWidget;
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(5, 5, 5, 5);
    centralLayout->addWidget(splitter);
    setCentralWidget(central);

    statusBar()->addWidget(m_statusLabel = new QLabel(tr("Ready")), 1);
    statusBar()->addPermanentWidget(m_countsLabel = new QLabel);
    statusBar()->addPermanentWidget(m_progress = new QProgressBar);
    m_progress->setMaximumWidth(200);
    m_progress->setVisible(false);

    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::updateFilter);
    connect(m_keywordCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateFilter);
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateFilter);
    connect(m_regexCheck, &QCheckBox::toggled, this, &MainWindow::updateFilter);
    connect(customBtn, &QPushButton::clicked, this, &MainWindow::editCustomFilter);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearFilters);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &MainWindow::showCurrentComment);
    connect(selectAllBtn, &QPushButton::clicked, this, &MainWindow::selectVisible);
    connect(deselectAllBtn, &QPushButton::clicked, this, &MainWindow::deselectAll);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::previewDeletion);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::applyChanges);
    connect(m_discardBtn, &QPushButton::clicked, this, &MainWindow::discardChanges);
}

// ---------------------------------------------------------------- scanning

void MainWindow::selectFolder() {
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Folder to Scan"),
                                                          m_rootPath.isEmpty() ? QDir::homePath() : m_rootPath);
    if (dir.isEmpty()) return;
    m_rootPath = QDir(dir).absolutePath();
    m_pathLabel->setText(QDir::toNativeSeparators(m_rootPath));
    rescan();
}

void MainWindow::rescan() {
    if (m_rootPath.isEmpty()) return;
    if (!QDir(m_rootPath).exists()) {
        QMessageBox::warning(this, tr("Scan Error"), tr("Folder does not exist: %1").arg(m_rootPath));
        return;
    }
    stopBackgroundWork();
    m_model->clear();
    m_pendingPlan.clear();
    m_preview->clear();

    const AppSettings settings = AppSettings::load();
    m_extractor.setKeywords(settings.keywords);
    m_extractor.setParserMethod(CommentExtractor::ParserMethod(settings.parserMethod));
    const QString keyword = m_keywordCombo->currentText();
    m_keywordCombo->blockSignals(true);
    m_keywordCombo->clear();
    m_keywordCombo->addItem(tr("All Keywords"));
    m_keywordCombo->addItems(settings.keywords);
    m_keywordCombo->setCurrentIndex(std::max(0, m_keywordCombo->findText(keyword)));
    m_keywordCombo->blockSignals(false);
    updateFilter();

    FileCrawler::Options options;
    options.rootPath = m_rootPath;
    options.ignorePatterns = settings.ignorePatterns;
    options.useGitignore = settings.useGitignore;
    setBusy(true);
    m_progress->setRange(0, 0);
    m_statusLabel->setText(tr("Scanning..."));
    m_scanHandled = false;
    m_scanWatcher.setFuture(FileCrawler::scan(options));
}

void MainWindow::cancelScan() {
    m_scanWatcher.cancel();
    m_extractWatcher.cancel();
}

void MainWindow::stopBackgroundWork() {
    cancelScan();
    m_scanWatcher.waitForFinished();
    m_extractWatcher.waitForFinished();
    m_scanHandled = m_extractHandled = true;
    m_flushTimer.stop();
    QMutexLocker lock(&m_inboxMutex);
    m_inbox = {};
}

void MainWindow::onScanFinished() {
    if (m_scanHandled || !m_scanWatcher.isFinished()) return;
    m_scanHandled = true;
    const QFuture<FileCrawler::Result> future = m_scanWatcher.future();
    if (future.isCanceled() || future.resultCount() == 0) {
        setBusy(false);
        m_statusLabel->setText(tr("Scan cancelled"));
        return;
    }
    const FileCrawler::Result result = future.result();
    m_statusLabel->setText(tr("Scan complete: %1 files, %2 code files. Extracting comments...")
                               .arg(result.totalFiles).arg(result.files.size()));
    m_files = result.files;
    m_progress->setRange(0, int(m_files.size()));
    m_progress->setValue(0);

    // Parse files on all cores; results are collected in the inbox and shown in batches.
    const CommentExtractor* extractor = &m_extractor;
    const QString prefix = m_rootPath.endsWith(u'/') ? m_rootPath : m_rootPath + u'/';
    m_extractHandled = false;
    m_extractWatcher.setFuture(QtConcurrent::map(m_files, [this, extractor, prefix](const QString& file) {
        QVector<Comment> comments =
            extractor->extractFromFile(file, file.startsWith(prefix) ? file.mid(prefix.size()) : file);
        if (comments.isEmpty()) return;
        QMutexLocker lock(&m_inboxMutex);
        m_inbox.append(std::move(comments));
    }));
    m_flushTimer.start();
}

void MainWindow::flushExtracted() {
    QVector<Comment> batch;
    {
        QMutexLocker lock(&m_inboxMutex);
        batch.swap(m_inbox);
    }
    m_model->addComments(batch);
}

void MainWindow::onExtractionFinished() {
    if (m_extractHandled || !m_extractWatcher.isFinished()) return;
    m_extractHandled = true;
    m_flushTimer.stop();
    flushExtracted();
    m_files = {};
    setBusy(false);
    const int count = m_model->totalCount();
    m_statusLabel->setText(m_extractWatcher.isCanceled() ? tr("Cancelled: %1 comments found so far").arg(count)
                                                         : tr("Found %1 comments").arg(count));
    const QString language = m_languageCombo->currentText();
    m_languageCombo->blockSignals(true);
    m_languageCombo->clear();
    m_languageCombo->addItem(tr("All Languages"));
    m_languageCombo->addItems(m_model->languages());
    m_languageCombo->setCurrentIndex(std::max(0, m_languageCombo->findText(language)));
    m_languageCombo->blockSignals(false);
    updateFilter();
}

// ---------------------------------------------------------------- browsing

void MainWindow::showCurrentComment() {
    const Comment* c = m_model->commentAt(m_proxy->mapToSource(m_table->currentIndex()).row());
    if (!c) return m_preview->clear();
    // While a deletion is pending for this file, show the whole pending result.
    const auto pending = m_pendingPlan.constFind(c->filePath);
    showPreview(c->filePath, pending != m_pendingPlan.cend() ? *pending : QVector<EditRange>{EditPlanner::rangeFor(*c)},
                c->language, c->startLine);
}

void MainWindow::showPreview(const QString& filePath, const QVector<EditRange>& ranges, const QString& language,
                             int focusLine) {
    const EditResult result = EditPlanner::preview(filePath, ranges);
    if (!result.ok()) {
        m_preview->clear();
        m_statusLabel->setText(result.errorMessage);
        return;
    }
    m_preview->showDiff(result.original.text, result.modifiedContent, language, ranges, focusLine);
}

void MainWindow::updateFilter() {
    CommentFilterProxy::Filter filter;
    filter.search = m_search->text();
    filter.regex = m_regexCheck->isChecked();
    if (m_keywordCombo->currentIndex() > 0) filter.keyword = m_keywordCombo->currentText();
    if (m_languageCombo->currentIndex() > 0) filter.language = m_languageCombo->currentText();
    filter.path = m_customPath;
    filter.type = m_customType;
    m_proxy->setFilter(filter);
}

void MainWindow::editCustomFilter() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Custom Filter"));
    auto* form = new QFormLayout(&dialog);
    auto* pathEdit = new QLineEdit(m_customPath);
    form->addRow(tr("File path contains:"), pathEdit);
    auto* languageCombo = new QComboBox;
    for (int i = 0; i < m_languageCombo->count(); ++i) languageCombo->addItem(m_languageCombo->itemText(i));
    languageCombo->setCurrentIndex(m_languageCombo->currentIndex());
    form->addRow(tr("Language:"), languageCombo);
    auto* typeCombo = new QComboBox;
    typeCombo->addItems({tr("All Types"), "Line", "Block", "Doc", "Inline"});
    typeCombo->setCurrentIndex(std::max(0, typeCombo->findText(m_customType)));
    form->addRow(tr("Comment type:"), typeCombo);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    m_customPath = pathEdit->text().trimmed();
    m_customType = typeCombo->currentIndex() > 0 ? typeCombo->currentText() : QString();
    m_languageCombo->setCurrentIndex(languageCombo->currentIndex());  // also refreshes the filter
    updateFilter();
}

void MainWindow::clearFilters() {
    m_customPath.clear();
    m_customType.clear();
    for (QWidget* w : std::initializer_list<QWidget*>{m_search, m_keywordCombo, m_languageCombo, m_regexCheck})
        w->blockSignals(true);
    m_search->clear();
    m_keywordCombo->setCurrentIndex(0);
    m_languageCombo->setCurrentIndex(0);
    m_regexCheck->setChecked(false);
    for (QWidget* w : std::initializer_list<QWidget*>{m_search, m_keywordCombo, m_languageCombo, m_regexCheck})
        w->blockSignals(false);
    updateFilter();
}

void MainWindow::selectVisible() {
    if (m_proxy->rowCount() == m_model->totalCount()) return m_model->setSelected({}, true);
    QVector<int> rows;
    rows.reserve(m_proxy->rowCount());
    for (int r = 0; r < m_proxy->rowCount(); ++r) rows << m_proxy->mapToSource(m_proxy->index(r, 0)).row();
    m_model->setSelected(rows, true);
}

void MainWindow::deselectAll() { m_model->setSelected({}, false); }

// ---------------------------------------------------------------- deleting

void MainWindow::previewDeletion() {
    const QVector<const Comment*> selected = m_model->selectedComments();
    if (selected.isEmpty()) return;
    m_pendingPlan = EditPlanner::plan(selected);
    const Comment* first = selected.first();
    showPreview(first->filePath, m_pendingPlan.value(first->filePath), first->language, first->startLine);
    m_statusLabel->setText(tr("Preview: %1 comment(s) in %2 file(s) would be removed. Click a row to review a file.")
                               .arg(selected.size()).arg(m_pendingPlan.size()));
    updateActions();
}

void MainWindow::applyChanges() {
    if (m_pendingPlan.isEmpty()) return;
    if (QMessageBox::question(this, tr("Apply Changes"),
                              tr("This will modify %1 file(s). Continue?").arg(m_pendingPlan.size())) != QMessageBox::Yes)
        return;

    const AppSettings settings = AppSettings::load();
    m_writer.setBackupsEnabled(settings.backupsEnabled);
    m_writer.setBackupDirectory(settings.backupDirectory);
    QSet<QString> changed;
    QStringList failures;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (auto it = m_pendingPlan.cbegin(); it != m_pendingPlan.cend(); ++it) {
        QString error;
        if (m_writer.apply(it.key(), it.value(), &error)) changed.insert(it.key());
        else failures << relativePath(it.key()) + ": " + error;
    }
    // Re-parse the modified files so the table's line numbers match the files again.
    QVector<Comment> fresh;
    for (const QString& file : changed) fresh += m_extractor.extractFromFile(file, relativePath(file));
    m_model->replaceFiles(changed, fresh);
    QApplication::restoreOverrideCursor();

    m_pendingPlan.clear();
    m_preview->clear();
    m_statusLabel->setText(tr("Applied changes to %1 file(s)").arg(changed.size()));
    if (!failures.isEmpty())
        QMessageBox::warning(this, tr("Some Files Were Not Modified"), failures.join(u'\n'));
    updateActions();
}

void MainWindow::discardChanges() {
    m_pendingPlan.clear();
    m_preview->clear();
    m_model->setSelected({}, false);
    m_statusLabel->setText(tr("Changes discarded"));
}

// ---------------------------------------------------------------- other actions

void MainWindow::openSettings() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted && !m_rootPath.isEmpty() &&
        QMessageBox::question(this, tr("Settings Saved"), tr("Rescan the folder with the new settings?")) == QMessageBox::Yes)
        rescan();
}

void MainWindow::exportComments() {
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Comments"), QDir::homePath() + "/comments.csv",
                                                          tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"), tr("Could not open file for writing."));
        return;
    }
    const auto quoted = [](QString s) { return u'"' + s.replace(u'"', QLatin1String("\"\"")) + u'"'; };
    QTextStream out(&file);
    out << "File,Line,Type,Keywords,Comment,Language\n";
    for (const Comment& c : m_model->comments())
        out << quoted(c.relativePath) << ',' << c.startLine << ',' << c.typeString() << ',' << quoted(c.keywordsString())
            << ',' << quoted(c.cleanText()) << ',' << quoted(c.language) << '\n';
    m_statusLabel->setText(tr("Comments exported to %1").arg(fileName));
}

void MainWindow::exportAuditLog() {
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Export Audit Log"), QDir::homePath() + "/audit-log.txt",
                                                          tr("Text Files (*.txt);;All Files (*)"));
    if (fileName.isEmpty()) return;
    if (m_writer.exportAuditLog(fileName)) m_statusLabel->setText(tr("Audit log exported to %1").arg(fileName));
    else QMessageBox::warning(this, tr("Export Error"), tr("Could not export audit log."));
}

void MainWindow::harvestSpecialComments() {
    if (m_model->totalCount() == 0) {
        QMessageBox::information(this, tr("No Comments"), tr("Please select a folder and scan it first."));
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Harvest Special Comments"),
                                                          QDir::homePath() + "/special_comments_harvest.txt",
                                                          tr("Text Files (*.txt);;All Files (*)"));
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Error"), tr("Could not open file for writing."));
        return;
    }

    static const QStringList terms = {"todo", "fix", "hack", "fix later", "partial implmentation",
                                      "partial implementation", "simple implmentation", "simple implementation",
                                      "example", "do later", "unstable", "xxx", "bug", "deprecated", "warning",
                                      "optimize", "review"};
    const auto isSpecial = [](const Comment* c) {
        return std::any_of(terms.begin(), terms.end(), [c](const QString& t) { return c->rawText.contains(t, Qt::CaseInsensitive); });
    };
    const int contextLines = AppSettings::load().contextLines;
    const QString rule(80, u'=');
    const QString thinRule(80, u'-');

    QMap<QString, QVector<const Comment*>> byFile;
    for (const Comment& c : m_model->comments()) byFile[c.filePath] << &c;

    QTextStream out(&file);
    out << rule << "\nCOMMENT TRACKER - SPECIAL COMMENTS HARVEST REPORT\n"
        << "Generated on: " << QDateTime::currentDateTime().toString(Qt::ISODate) << '\n'
        << "Keywords searched: " << terms.join(", ") << '\n' << rule << "\n\n";

    int groups = 0, harvested = 0;
    for (auto it = byFile.begin(); it != byFile.end(); ++it) {
        QVector<const Comment*>& comments = it.value();
        std::sort(comments.begin(), comments.end(), [](const Comment* a, const Comment* b) {
            return std::pair(a->startLine, a->startColumn) < std::pair(b->startLine, b->startColumn);
        });
        QStringList lines;  // file content, loaded only if something in it is harvested
        const auto context = [&](int first, int last) {  // 1-based inclusive line range
            if (lines.isEmpty()) {
                TextFile text;
                if (text.read(it.key())) lines = text.text.split(u'\n');
            }
            first = std::max(first, 1);
            last = std::min(last, int(lines.size()));
            return first > last ? QString() : lines.mid(first - 1, last - first + 1).join(u'\n');
        };

        // Group comments that are at most two lines apart; report groups containing a special comment.
        for (qsizetype i = 0, j = 0; i < comments.size(); i = j) {
            for (j = i + 1; j < comments.size() && comments[j]->startLine - comments[j - 1]->endLine <= 3; ++j) {}
            if (std::none_of(comments.begin() + i, comments.begin() + j, isSpecial)) continue;
            const Comment& first = *comments[i];
            const Comment& last = *comments[j - 1];
            ++groups;
            harvested += int(j - i);
            out << thinRule << "\nFILE: " << first.relativePath << " (" << first.language << ")\n"
                << "LINE RANGE: " << first.startLine << " - " << last.endLine << '\n' << thinRule << '\n';
            if (const QString before = context(first.startLine - contextLines, first.startLine - 1); !before.isEmpty())
                out << "--- CODE CONTEXT BEFORE ---\n" << before << "\n\n";
            out << "--- COMMENTS ---\n";
            for (qsizetype k = i; k < j; ++k)
                out << "[Line " << comments[k]->startLine << " - " << comments[k]->endLine << " | Type: "
                    << comments[k]->typeString() << "]" << (isSpecial(comments[k]) ? " [MATCH]" : " [CONTEXT COMMENT]")
                    << '\n' << comments[k]->rawText << "\n\n";
            if (const QString after = context(last.endLine + 1, last.endLine + contextLines); !after.isEmpty())
                out << "--- CODE CONTEXT AFTER ---\n" << after << '\n';
            out << '\n';
        }
    }
    out << rule << "\nSUMMARY:\nTotal special comment groups harvested: " << groups << '\n'
        << "Total comments included (matching + surrounding): " << harvested << '\n' << rule << '\n';

    m_statusLabel->setText(tr("Harvested %1 comments into %2").arg(harvested).arg(fileName));
    QMessageBox::information(this, tr("Harvest Complete"),
                             tr("Successfully harvested %1 comments (including context) into:\n%2").arg(harvested).arg(fileName));
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About Comment Explorer & Cleaner"),
                       tr("<h2>Comment Explorer &amp; Cleaner</h2><p>Version %1</p>"
                          "<p>Explore and safely remove code comments across 30+ programming languages.</p>"
                          "<ul><li>Lexer and Tree-sitter based extraction</li><li>Safe deletion with preview</li>"
                          "<li>Automatic backups</li><li>Audit logging</li></ul>")
                           .arg(QCoreApplication::applicationVersion()));
}

// ---------------------------------------------------------------- helpers

void MainWindow::setBusy(bool busy) {
    m_busy = busy;
    m_progress->setVisible(busy);
    updateActions();
}

void MainWindow::updateActions() {
    const bool pending = !m_pendingPlan.isEmpty();
    m_rescanAct->setEnabled(!m_rootPath.isEmpty());
    m_cancelAct->setEnabled(m_busy);
    m_harvestAct->setEnabled(!m_busy && m_model->totalCount() > 0);
    m_deleteBtn->setEnabled(!m_busy && m_model->selectedCount() > 0);
    m_applyBtn->setEnabled(!m_busy && pending);
    m_discardBtn->setEnabled(pending);
}

QString MainWindow::relativePath(const QString& filePath) const { return QDir(m_rootPath).relativeFilePath(filePath); }
