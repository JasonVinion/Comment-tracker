#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QTreeView>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSplitter>
#include <QThread>
#include <QFutureWatcher>
#include <QFuture>
#include <memory>
#include <atomic>

#include "Comment.h"
#include "CommentModel.h"
#include "DiffPreview.h"

class FileCrawler;
class CommentExtractor;
class EditPlanner;
class FileWriter;
class SettingsDialog;

/**
 * @brief Main application window
 * 
 * Provides the primary user interface for:
 * - Folder selection and scanning
 * - Comment list display and filtering
 * - Preview panes for before/after deletion
 * - Action buttons for deletion and application
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    // File operations
    void onSelectFolder();
    void onRescan();
    void onCancelScan();
    
    // Scan callbacks
    void onFileFound(const QString& filePath);
    void onScanProgress(int filesScanned, int codeFilesFound, const QString& currentPath);
    void onScanComplete(int totalFiles, int codeFiles);
    void onScanError(const QString& message);
    
    // Comment extraction
    void onExtractionComplete();
    
    // Selection and filtering
    void onCommentSelected(const QModelIndex& current, const QModelIndex& previous);
    void onSearchTextChanged(const QString& text);
    void onKeywordFilterChanged();
    void onLanguageFilterChanged(const QString& language);
    void onRegexToggled(bool checked);
    void onClearFilters();
    void onCustomFilter();
    
    // Deletion operations
    void onDeleteSelected();
    void onPreviewDeletion();
    void onApplyChanges();
    void onDiscardChanges();
    
    // Menu actions
    void onSettings();
    void onExportComments();
    void onExportAuditLog();
    void onAbout();
    
    // Selection
    void onSelectAll();
    void onDeselectAll();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    
    void updateStatusBar();
    void updatePreview();
    void updateActionStates();
    
    void startExtraction(const QStringList& files);
    void processNextFile();
    
    // UI Components
    QSplitter* m_mainSplitter;
    QSplitter* m_rightSplitter;
    
    // Left panel - file tree (optional)
    QTreeView* m_fileTree;
    
    // Center panel - comment table
    QTableView* m_commentTable;
    CommentModel* m_commentModel;
    CommentFilterProxy* m_filterProxy;
    
    // Right panel - preview
    DiffPreview* m_diffPreview;
    
    // Toolbar components
    QPushButton* m_selectFolderBtn;
    QPushButton* m_rescanBtn;
    QPushButton* m_settingsBtn;
    QPushButton* m_cancelBtn;
    QLineEdit* m_searchEdit;
    QComboBox* m_keywordFilterCombo;
    QComboBox* m_languageFilterCombo;
    QCheckBox* m_regexCheck;
    QPushButton* m_clearFiltersBtn;
    QPushButton* m_customFilterBtn;
    
    // Action buttons
    QPushButton* m_deleteSelectedBtn;
    QPushButton* m_previewBtn;
    QPushButton* m_applyBtn;
    QPushButton* m_discardBtn;
    
    // Status bar
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QLabel* m_countsLabel;
    
    // Core components
    std::unique_ptr<FileCrawler> m_fileCrawler;
    std::unique_ptr<CommentExtractor> m_extractor;
    std::unique_ptr<EditPlanner> m_editPlanner;
    std::unique_ptr<FileWriter> m_fileWriter;
    
    // State
    QString m_rootPath;
    QStringList m_filesToProcess;
    int m_currentFileIndex = 0;
    std::atomic<bool> m_isScanning{false};
    bool m_hasPendingChanges = false;
    QString m_customPathFilter;
    QString m_customTypeFilter;
    
    // Settings
    SettingsDialog* m_settingsDialog = nullptr;
    
    // Pending edits
    QMap<QString, EditResult> m_pendingEdits;

    // Future for background tasks
    QFuture<void> m_extractionFuture;
};

#endif // MAIN_WINDOW_H
