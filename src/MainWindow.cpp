#include "MainWindow.h"
#include "FileCrawler.h"
#include "CommentExtractor.h"
#include "EditPlanner.h"
#include "FileWriter.h"
#include "SettingsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QSettings>
#include <QtConcurrent>
#include <QApplication>
#include <QClipboard>
#include <QTextStream>
#include <QStatusBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_fileCrawler(std::make_unique<FileCrawler>())
    , m_extractor(std::make_unique<CommentExtractor>())
    , m_editPlanner(std::make_unique<EditPlanner>())
    , m_fileWriter(std::make_unique<FileWriter>()) {
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();
    updateActionStates();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    
    // Main content splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    
    // Left panel - Comment table
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Filter bar
    QHBoxLayout* filterLayout = new QHBoxLayout();
    
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search comments...");
    m_searchEdit->setClearButtonEnabled(true);
    filterLayout->addWidget(m_searchEdit, 1);
    
    m_keywordFilterCombo = new QComboBox();
    m_keywordFilterCombo->addItem("All Keywords");
    m_keywordFilterCombo->addItem("TODO");
    m_keywordFilterCombo->addItem("FIXME");
    m_keywordFilterCombo->addItem("HACK");
    m_keywordFilterCombo->addItem("NOTE");
    m_keywordFilterCombo->addItem("BUG");
    filterLayout->addWidget(m_keywordFilterCombo);
    
    m_languageFilterCombo = new QComboBox();
    m_languageFilterCombo->addItem("All Languages");
    filterLayout->addWidget(m_languageFilterCombo);
    
    m_regexCheck = new QCheckBox("Regex");
    filterLayout->addWidget(m_regexCheck);

    m_customFilterBtn = new QPushButton("Custom Filter...");
    filterLayout->addWidget(m_customFilterBtn);
    
    m_clearFiltersBtn = new QPushButton("Clear");
    filterLayout->addWidget(m_clearFiltersBtn);
    
    leftLayout->addLayout(filterLayout);
    
    // Comment table
    m_commentTable = new QTableView();
    m_commentModel = new CommentModel(this);
    m_filterProxy = new CommentFilterProxy(this);
    m_filterProxy->setSourceModel(m_commentModel);
    m_commentTable->setModel(m_filterProxy);
    
    m_commentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_commentTable->setAlternatingRowColors(true);
    m_commentTable->setSortingEnabled(true);
    m_commentTable->horizontalHeader()->setStretchLastSection(true);
    m_commentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_commentTable->verticalHeader()->setVisible(false);
    
    // Set column widths
    m_commentTable->setColumnWidth(CommentModel::ColFile, 200);
    m_commentTable->setColumnWidth(CommentModel::ColLine, 50);
    m_commentTable->setColumnWidth(CommentModel::ColType, 60);
    m_commentTable->setColumnWidth(CommentModel::ColKeywords, 80);
    m_commentTable->setColumnWidth(CommentModel::ColLanguage, 80);
    
    leftLayout->addWidget(m_commentTable);
    
    // Selection buttons
    QHBoxLayout* selectionLayout = new QHBoxLayout();
    QPushButton* selectAllBtn = new QPushButton("Select All");
    QPushButton* deselectAllBtn = new QPushButton("Deselect All");
    m_deleteSelectedBtn = new QPushButton("Delete Selected");
    m_deleteSelectedBtn->setEnabled(false);
    
    selectionLayout->addWidget(selectAllBtn);
    selectionLayout->addWidget(deselectAllBtn);
    selectionLayout->addStretch();
    selectionLayout->addWidget(m_deleteSelectedBtn);
    
    leftLayout->addLayout(selectionLayout);
    
    connect(selectAllBtn, &QPushButton::clicked, this, &MainWindow::onSelectAll);
    connect(deselectAllBtn, &QPushButton::clicked, this, &MainWindow::onDeselectAll);
    
    m_mainSplitter->addWidget(leftPanel);
    
    // Right panel - Preview
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    m_diffPreview = new DiffPreview(this);
    rightLayout->addWidget(m_diffPreview);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_previewBtn = new QPushButton("Preview Changes");
    m_applyBtn = new QPushButton("Apply Changes");
    m_discardBtn = new QPushButton("Discard");
    
    m_previewBtn->setEnabled(false);
    m_applyBtn->setEnabled(false);
    m_discardBtn->setEnabled(false);
    
    actionLayout->addWidget(m_previewBtn);
    actionLayout->addWidget(m_applyBtn);
    actionLayout->addWidget(m_discardBtn);
    
    rightLayout->addLayout(actionLayout);
    
    m_mainSplitter->addWidget(rightPanel);
    
    // Set splitter sizes
    m_mainSplitter->setSizes({600, 400});
    
    mainLayout->addWidget(m_mainSplitter);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* openAction = fileMenu->addAction("&Open Folder...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onSelectFolder);
    
    fileMenu->addSeparator();
    
    QAction* exportAction = fileMenu->addAction("&Export Comments...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportComments);
    
    QAction* harvestAction = fileMenu->addAction("&Harvest Special Comments...");
    connect(harvestAction, &QAction::triggered, this, &MainWindow::onHarvestSpecialComments);
    
    QAction* exportLogAction = fileMenu->addAction("Export Audit &Log...");
    connect(exportLogAction, &QAction::triggered, this, &MainWindow::onExportAuditLog);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // Edit menu
    QMenu* editMenu = menuBar->addMenu("&Edit");
    
    QAction* settingsAction = editMenu->addAction("&Settings...");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    
    QAction* rescanAction = viewMenu->addAction("&Rescan");
    rescanAction->setShortcut(QKeySequence::Refresh);
    connect(rescanAction, &QAction::triggered, this, &MainWindow::onRescan);
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar() {
    QToolBar* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    
    m_selectFolderBtn = new QPushButton("Select Folder");
    toolbar->addWidget(m_selectFolderBtn);
    
    m_rescanBtn = new QPushButton("Rescan");
    m_rescanBtn->setEnabled(false);
    toolbar->addWidget(m_rescanBtn);

    m_settingsBtn = new QPushButton("Settings");
    toolbar->addWidget(m_settingsBtn);
    
    m_harvestBtn = new QPushButton("Harvest Comments");
    m_harvestBtn->setEnabled(false);
    toolbar->addWidget(m_harvestBtn);
    connect(m_harvestBtn, &QPushButton::clicked, this, &MainWindow::onHarvestSpecialComments);
    
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setEnabled(false);
    toolbar->addWidget(m_cancelBtn);
    
    toolbar->addSeparator();
    
    QLabel* rootLabel = new QLabel("Root: ");
    toolbar->addWidget(rootLabel);
    
    QLabel* pathLabel = new QLabel("(none selected)");
    pathLabel->setObjectName("pathLabel");
    toolbar->addWidget(pathLabel);
}

void MainWindow::setupStatusBar() {
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel, 1);
    
    m_countsLabel = new QLabel();
    statusBar()->addPermanentWidget(m_countsLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::setupConnections() {
    // Toolbar buttons
    connect(m_selectFolderBtn, &QPushButton::clicked, this, &MainWindow::onSelectFolder);
    connect(m_rescanBtn, &QPushButton::clicked, this, &MainWindow::onRescan);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::onCancelScan);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettings);
    
    // FileCrawler signals
    connect(m_fileCrawler.get(), &FileCrawler::fileFound, this, &MainWindow::onFileFound);
    connect(m_fileCrawler.get(), &FileCrawler::progress, this, &MainWindow::onScanProgress);
    connect(m_fileCrawler.get(), &FileCrawler::scanComplete, this, &MainWindow::onScanComplete);
    connect(m_fileCrawler.get(), &FileCrawler::error, this, &MainWindow::onScanError);
    
    // Comment table selection
    connect(m_commentTable->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onCommentSelected);
    
    // Filter controls
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_keywordFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onKeywordFilterChanged);
    connect(m_languageFilterCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onLanguageFilterChanged);
    connect(m_regexCheck, &QCheckBox::toggled, this, &MainWindow::onRegexToggled);
    connect(m_clearFiltersBtn, &QPushButton::clicked, this, &MainWindow::onClearFilters);
    connect(m_customFilterBtn, &QPushButton::clicked, this, &MainWindow::onCustomFilter);
    
    // Action buttons
    connect(m_deleteSelectedBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSelected);
    connect(m_previewBtn, &QPushButton::clicked, this, &MainWindow::onPreviewDeletion);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyChanges);
    connect(m_discardBtn, &QPushButton::clicked, this, &MainWindow::onDiscardChanges);
    
    // Model signals
    connect(m_commentModel, &CommentModel::selectionChanged,
            this, [this](int selected, int total) {
                m_deleteSelectedBtn->setEnabled(selected > 0);
                m_countsLabel->setText(QString("Selected: %1 / %2").arg(selected).arg(total));
            });
}

void MainWindow::onSelectFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder to Scan",
        m_rootPath.isEmpty() ? QDir::homePath() : m_rootPath);
    
    if (!dir.isEmpty()) {
        m_rootPath = dir;
        
        // Update path label
        QLabel* pathLabel = findChild<QLabel*>("pathLabel");
        if (pathLabel) {
            pathLabel->setText(m_rootPath);
        }
        
        m_rescanBtn->setEnabled(true);
        onRescan();
    }
}

void MainWindow::onRescan() {
    if (m_rootPath.isEmpty()) {
        return;
    }
    
    // Clear existing data
    m_commentModel->clear();
    m_filesToProcess.clear();
    m_currentFileIndex = 0;
    m_pendingEdits.clear();
    m_diffPreview->clear();
    
    // Update UI state
    m_isScanning = true;
    m_cancelBtn->setEnabled(true);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);  // Indeterminate
    m_statusLabel->setText("Scanning...");
    
    // Load settings
    QSettings settings;
    settings.beginGroup("FileCrawler");
    QStringList patterns = settings.value("ignorePatterns").toStringList();
    bool useGitignore = settings.value("useGitignore", true).toBool();
    settings.endGroup();
    
    // Configure and start crawler
    m_fileCrawler->clearIgnorePatterns();
    m_fileCrawler->addIgnorePatterns(patterns);
    m_fileCrawler->setUseGitignore(useGitignore);
    m_fileCrawler->setRootPath(m_rootPath);
    m_fileCrawler->startScan();
}

void MainWindow::onCancelScan() {
    m_fileCrawler->cancel();
    m_isScanning = false;
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Scan cancelled");
}

void MainWindow::onFileFound(const QString& filePath) {
    m_filesToProcess.append(filePath);
}

void MainWindow::onScanProgress(int filesScanned, int codeFilesFound, const QString& currentPath) {
    m_statusLabel->setText(QString("Scanning: %1 files found (%2 code files)...")
                          .arg(filesScanned).arg(codeFilesFound));
}

void MainWindow::onScanComplete(int totalFiles, int codeFiles) {
    m_statusLabel->setText(QString("Scan complete: %1 files, %2 code files. Extracting comments...")
                          .arg(totalFiles).arg(codeFiles));
    
    // Start extraction
    m_progressBar->setRange(0, m_filesToProcess.size());
    m_progressBar->setValue(0);
    
    startExtraction(m_filesToProcess);
}

void MainWindow::onScanError(const QString& message) {
    QMessageBox::warning(this, "Scan Error", message);
    m_isScanning = false;
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);
}

void MainWindow::startExtraction(const QStringList& files) {
    if (files.isEmpty()) {
        onExtractionComplete();
        return;
    }
    
    // Load extractor settings
    QSettings settings;
    settings.beginGroup("Extractor");
    int contextLines = settings.value("contextLines", 2).toInt();
    QStringList keywords = settings.value("keywords").toStringList();
    int parserMethodInt = settings.value("parserMethod", 0).toInt();
    settings.endGroup();
    
    m_extractor->setContextLines(contextLines);
    if (!keywords.isEmpty()) {
        m_extractor->setKeywords(keywords);
    }
    m_extractor->setParserMethod(static_cast<CommentExtractor::ParserMethod>(parserMethodInt));
    
    // Process files in a background thread
    m_extractionFuture = QtConcurrent::run([this, files]() {
        QDir rootDir(m_rootPath);
        
        for (int i = 0; i < files.size(); i++) {
            if (!m_isScanning) break;
            
            const QString& filePath = files[i];
            QString relativePath = rootDir.relativeFilePath(filePath);
            
            QVector<Comment> comments;
            try {
                comments = m_extractor->extractFromFile(filePath, relativePath);
            } catch (const std::exception& e) {
                // Log error but continue with other files
                qWarning() << "Failed to extract from" << filePath << ":" << e.what();
            } catch (...) {
                qWarning() << "Unknown error extracting from" << filePath;
            }
            
            // Add to model on main thread
            QMetaObject::invokeMethod(this, [this, comments, i, total = files.size()]() {
                m_commentModel->addComments(comments);
                m_progressBar->setValue(i + 1);
                
                if (i + 1 >= total) {
                    onExtractionComplete();
                }
            }, Qt::QueuedConnection);
        }
    });
}

void MainWindow::onExtractionComplete() {
    m_isScanning = false;
    m_cancelBtn->setEnabled(false);
    m_progressBar->setVisible(false);
    
    int count = m_commentModel->totalCount();
    m_statusLabel->setText(QString("Found %1 comments").arg(count));
    m_countsLabel->setText(QString("Selected: 0 / %1").arg(count));
    
    // Populate language filter
    m_languageFilterCombo->clear();
    m_languageFilterCombo->addItem("All Languages");
    QSet<QString> languages;
    for (int i = 0; i < m_commentModel->totalCount(); i++) {
        const Comment* c = m_commentModel->commentAt(i);
        if (c) {
            languages.insert(c->language);
        }
    }
    QStringList langList = languages.values();
    langList.sort();
    m_languageFilterCombo->addItems(langList);
    
    updateActionStates();
}

void MainWindow::onCommentSelected(const QModelIndex& current, const QModelIndex& previous) {
    Q_UNUSED(previous);
    
    if (!current.isValid()) {
        m_diffPreview->clear();
        return;
    }
    
    // Get the source model index
    QModelIndex sourceIndex = m_filterProxy->mapToSource(current);
    const Comment* comment = m_commentModel->commentAt(sourceIndex.row());
    
    if (comment) {
        // Read file content
        QFile file(comment->filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            m_diffPreview->setOriginal(content, comment->language);
            
            // Generate preview of deletion
            QVector<Comment> toDelete;
            toDelete.append(*comment);
            
            auto edits = m_editPlanner->planEdits(toDelete);
            if (edits.contains(comment->filePath)) {
                EditResult result = m_editPlanner->previewEdits(comment->filePath, edits[comment->filePath]);
                m_diffPreview->setModified(result.modifiedContent);
            }

            m_diffPreview->highlightComment(*comment);
        }
    }
    
    updateActionStates();
}

void MainWindow::onSearchTextChanged(const QString& text) {
    m_filterProxy->setSearchText(text);
}

void MainWindow::onKeywordFilterChanged() {
    QString keyword = m_keywordFilterCombo->currentText();
    if (keyword == "All Keywords") {
        m_filterProxy->setKeywordFilter({});
    } else {
        m_filterProxy->setKeywordFilter({keyword});
    }
}

void MainWindow::onLanguageFilterChanged(const QString& language) {
    if (language == "All Languages") {
        m_filterProxy->setLanguageFilter({});
    } else {
        m_filterProxy->setLanguageFilter({language});
    }
}

void MainWindow::onRegexToggled(bool checked) {
    m_filterProxy->setUseRegex(checked);
}

void MainWindow::onClearFilters() {
    m_searchEdit->clear();
    m_keywordFilterCombo->setCurrentIndex(0);
    m_languageFilterCombo->setCurrentIndex(0);
    m_regexCheck->setChecked(false);
    m_filterProxy->clearFilters();
    m_customPathFilter.clear();
    m_customTypeFilter.clear();
    m_filterProxy->setPathFilter(QString());
    m_filterProxy->setTypeFilter({});
}

void MainWindow::onCustomFilter() {
    QDialog dialog(this);
    dialog.setWindowTitle("Custom Filter");

    QFormLayout* formLayout = new QFormLayout(&dialog);

    QLineEdit* pathEdit = new QLineEdit(m_customPathFilter, &dialog);
    formLayout->addRow("File path contains:", pathEdit);

    QComboBox* languageCombo = new QComboBox(&dialog);
    languageCombo->addItem("All Languages");
    for (int i = 1; i < m_languageFilterCombo->count(); ++i) {
        languageCombo->addItem(m_languageFilterCombo->itemText(i));
    }
    languageCombo->setCurrentText(m_languageFilterCombo->currentText());
    formLayout->addRow("Language:", languageCombo);

    QComboBox* typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("All Types");
    typeCombo->addItem("Line");
    typeCombo->addItem("Block");
    typeCombo->addItem("Doc");
    typeCombo->addItem("Inline");
    if (!m_customTypeFilter.isEmpty()) {
        typeCombo->setCurrentText(m_customTypeFilter);
    }
    formLayout->addRow("Comment type:", typeCombo);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    formLayout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_customPathFilter = pathEdit->text().trimmed();
    m_filterProxy->setPathFilter(m_customPathFilter);

    QString selectedLanguage = languageCombo->currentText();
    if (selectedLanguage == "All Languages") {
        m_filterProxy->setLanguageFilter({});
        m_languageFilterCombo->setCurrentIndex(0);
    } else {
        m_filterProxy->setLanguageFilter({selectedLanguage});
        m_languageFilterCombo->setCurrentText(selectedLanguage);
    }

    QString selectedType = typeCombo->currentText();
    if (selectedType == "All Types") {
        m_customTypeFilter.clear();
        m_filterProxy->setTypeFilter({});
    } else {
        m_customTypeFilter = selectedType;
        m_filterProxy->setTypeFilter({selectedType});
    }
}

void MainWindow::onDeleteSelected() {
    QVector<Comment*> selected = m_commentModel->selectedComments();
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No Selection", "Please select comments to delete.");
        return;
    }
    
    // Convert to value vector
    QVector<Comment> toDelete;
    for (Comment* c : selected) {
        toDelete.append(*c);
    }
    
    // Plan edits
    m_pendingEdits.clear();
    auto editsByFile = m_editPlanner->planEdits(toDelete);
    
    // Generate previews
    for (auto it = editsByFile.begin(); it != editsByFile.end(); ++it) {
        EditResult result = m_editPlanner->previewEdits(it.key(), it.value());
        m_pendingEdits[it.key()] = result;
    }
    
    m_hasPendingChanges = !m_pendingEdits.isEmpty();
    
    // Show preview for first file
    if (!m_pendingEdits.isEmpty()) {
        auto first = m_pendingEdits.begin();
        QString language;
        for (const Comment& c : toDelete) {
            if (c.filePath == first.key()) {
                language = c.language;
                break;
            }
        }
        m_diffPreview->setFromEditResult(first.value(), language);
    }
    
    updateActionStates();
    
    m_statusLabel->setText(QString("Preview: %1 file(s) would be modified")
                          .arg(m_pendingEdits.size()));
}

void MainWindow::onPreviewDeletion() {
    onDeleteSelected();
}

void MainWindow::onApplyChanges() {
    if (m_pendingEdits.isEmpty()) {
        return;
    }
    
    int reply = QMessageBox::question(this, "Apply Changes",
        QString("This will modify %1 file(s). Continue?").arg(m_pendingEdits.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Load settings
    QSettings settings;
    settings.beginGroup("FileWriter");
    bool backupsEnabled = settings.value("backupsEnabled", true).toBool();
    QString backupDir = settings.value("backupDirectory").toString();
    settings.endGroup();
    
    m_fileWriter->setBackupsEnabled(backupsEnabled);
    if (!backupDir.isEmpty()) {
        m_fileWriter->setBackupDirectory(backupDir);
    }
    
    // Apply edits
    int successCount = m_fileWriter->applyAllEdits(m_pendingEdits);
    
    // Remove deleted comments from model
    QVector<int> deletedIds;
    for (const auto& result : m_pendingEdits) {
        for (const auto& edit : result.appliedEdits) {
            deletedIds.append(edit.commentId);
        }
    }
    m_commentModel->removeComments(deletedIds);
    
    // Clear pending changes
    m_pendingEdits.clear();
    m_hasPendingChanges = false;
    m_diffPreview->clear();
    
    // Update status
    m_statusLabel->setText(QString("Applied changes to %1 file(s)").arg(successCount));
    
    updateActionStates();
}

void MainWindow::onDiscardChanges() {
    m_pendingEdits.clear();
    m_hasPendingChanges = false;
    m_diffPreview->clear();
    m_commentModel->deselectAll();
    m_statusLabel->setText("Changes discarded");
    updateActionStates();
}

void MainWindow::onSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    
    m_settingsDialog->loadSettings();
    m_settingsDialog->exec();
}

void MainWindow::onExportComments() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Comments",
        QDir::homePath() + "/comments.csv", "CSV Files (*.csv);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Error", "Could not open file for writing.");
        return;
    }
    
    QTextStream out(&file);
    
    // Header
    out << "File,Line,Type,Keywords,Comment,Language\n";
    
    // Data
    for (int i = 0; i < m_commentModel->totalCount(); i++) {
        const Comment* c = m_commentModel->commentAt(i);
        if (c) {
            // Escape CSV fields
            QString comment = c->cleanText;
            comment.replace("\"", "\"\"");
            
            out << "\"" << c->relativePath << "\","
                << c->startLine << ","
                << c->typeString() << ","
                << "\"" << c->keywordsString() << "\","
                << "\"" << comment << "\","
                << c->language << "\n";
        }
    }
    
    file.close();
    m_statusLabel->setText("Comments exported to " + fileName);
}

void MainWindow::onExportAuditLog() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Audit Log",
        QDir::homePath() + "/audit-log.txt", "Text Files (*.txt);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        if (m_fileWriter->exportAuditLog(fileName)) {
            m_statusLabel->setText("Audit log exported to " + fileName);
        } else {
            QMessageBox::warning(this, "Export Error", "Could not export audit log.");
        }
    }
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Comment Explorer & Cleaner",
        "<h2>Comment Explorer & Cleaner</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A professional tool for exploring and safely removing code comments "
        "across multiple programming languages.</p>"
        "<p>Features:</p>"
        "<ul>"
        "<li>Support for 30+ programming languages</li>"
        "<li>Accurate comment extraction</li>"
        "<li>Safe deletion with preview</li>"
        "<li>Automatic backups</li>"
        "<li>Audit logging</li>"
        "</ul>");
}

void MainWindow::onSelectAll() {
    m_commentModel->selectAll();
}

void MainWindow::onDeselectAll() {
    m_commentModel->deselectAll();
}

void MainWindow::updateStatusBar() {
    int total = m_commentModel->totalCount();
    int selected = m_commentModel->selectedCount();
    m_countsLabel->setText(QString("Selected: %1 / %2").arg(selected).arg(total));
}

void MainWindow::updatePreview() {
    QModelIndex current = m_commentTable->currentIndex();
    if (current.isValid()) {
        onCommentSelected(current, QModelIndex());
    }
}

void MainWindow::updateActionStates() {
    bool hasComments = m_commentModel->totalCount() > 0;
    bool hasSelection = m_commentModel->selectedCount() > 0;
    bool hasPending = m_hasPendingChanges;
    
    m_deleteSelectedBtn->setEnabled(hasSelection);
    m_previewBtn->setEnabled(hasSelection);
    m_applyBtn->setEnabled(hasPending);
    m_discardBtn->setEnabled(hasPending);
    m_harvestBtn->setEnabled(hasComments);
}

void MainWindow::onHarvestSpecialComments() {
    if (m_commentModel->totalCount() == 0) {
        QMessageBox::information(this, "No Comments", "There are no comments scanned yet. Please select a folder and scan first.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Harvest Special Comments",
        QDir::homePath() + "/special_comments_harvest.txt", "Text Files (*.txt);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Error", "Could not open file for writing.");
        return;
    }
    
    QTextStream out(&file);
    
    // Group comments by file path
    QMap<QString, QVector<Comment>> commentsByFile;
    const auto& allComments = m_commentModel->comments();
    for (const Comment& c : allComments) {
        commentsByFile[c.filePath].append(c);
    }
    
    // Define special keywords list
    static const QStringList matchTerms = {
        "todo", "fix", "hack", "fix later", 
        "partial implmentation", "partial implementation",
        "simple implmentation", "simple implementation",
        "example", "do later", "unstable",
        "xxx", "bug", "deprecated", "warning", "optimize", "review"
    };
    
    auto isSpecial = [](const Comment& comment) -> bool {
        QString cleanTextLower = comment.cleanText.toLower();
        QString rawTextLower = comment.rawText.toLower();
        
        for (const QString& term : matchTerms) {
            if (cleanTextLower.contains(term) || rawTextLower.contains(term)) {
                return true;
            }
        }
        return false;
    };
    
    int exportedGroupsCount = 0;
    int exportedCommentsCount = 0;
    
    out << "================================================================================\n";
    out << "COMMENT TRACKER - SPECIAL COMMENTS HARVEST REPORT\n";
    out << "Generated on: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "Keywords searched: " << matchTerms.join(", ") << "\n";
    out << "================================================================================\n\n";
    
    for (auto it = commentsByFile.begin(); it != commentsByFile.end(); ++it) {
        QVector<Comment> fileComments = it.value();
        
        // Sort by start line
        std::sort(fileComments.begin(), fileComments.end(), [](const Comment& a, const Comment& b) {
            if (a.startLine != b.startLine) {
                return a.startLine < b.startLine;
            }
            return a.startColumn < b.startColumn;
        });
        
        // Partition comments into groups where the gap between consecutive comments is <= 2 lines
        QVector<QVector<Comment>> groups;
        if (!fileComments.isEmpty()) {
            QVector<Comment> currentGroup;
            currentGroup.append(fileComments[0]);
            
            for (int i = 1; i < fileComments.size(); ++i) {
                const Comment& prev = currentGroup.last();
                const Comment& curr = fileComments[i];
                
                // Gap in lines
                int gap = qMax(0, curr.startLine - prev.endLine - 1);
                if (gap <= 2) {
                    currentGroup.append(curr);
                } else {
                    groups.append(currentGroup);
                    currentGroup.clear();
                    currentGroup.append(curr);
                }
            }
            if (!currentGroup.isEmpty()) {
                groups.append(currentGroup);
            }
        }
        
        // Export groups that contain at least one special comment
        for (const auto& group : groups) {
            bool hasSpecial = false;
            for (const Comment& c : group) {
                if (isSpecial(c)) {
                    hasSpecial = true;
                    break;
                }
            }
            
            if (!hasSpecial) {
                continue;
            }
            
            exportedGroupsCount++;
            exportedCommentsCount += group.size();
            
            const Comment& firstC = group.first();
            const Comment& lastC = group.last();
            
            out << "--------------------------------------------------------------------------------\n";
            out << "FILE: " << firstC.relativePath << " (" << firstC.language << ")\n";
            out << "LINE RANGE: " << firstC.startLine << " - " << lastC.endLine << "\n";
            out << "--------------------------------------------------------------------------------\n";
            
            if (!firstC.contextBefore.isEmpty()) {
                out << "--- CODE CONTEXT BEFORE ---\n";
                out << firstC.contextBefore << "\n\n";
            }
            
            out << "--- COMMENTS ---\n";
            for (const Comment& c : group) {
                QString matchedIndicator = isSpecial(c) ? " [MATCH]" : " [CONTEXT COMMENT]";
                out << "[Line " << c.startLine << " - " << c.endLine << " | Type: " << c.typeString() << "]" << matchedIndicator << "\n";
                out << c.rawText << "\n\n";
            }
            
            if (!lastC.contextAfter.isEmpty()) {
                out << "--- CODE CONTEXT AFTER ---\n";
                out << lastC.contextAfter << "\n";
            }
            
            out << "\n";
        }
    }
    
    out << "================================================================================\n";
    out << "SUMMARY:\n";
    out << "Total special comment groups harvested: " << exportedGroupsCount << "\n";
    out << "Total comments included (matching + surrounding): " << exportedCommentsCount << "\n";
    out << "================================================================================\n";
    
    file.close();
    m_statusLabel->setText(QString("Harvested %1 comments into %2").arg(exportedCommentsCount).arg(fileName));
    QMessageBox::information(this, "Harvest Complete", 
        QString("Successfully harvested %1 comments (including context) into:\n%2").arg(exportedCommentsCount).arg(fileName));
}
