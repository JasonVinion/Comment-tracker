#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QFutureWatcher>
#include <QMainWindow>
#include <QMutex>
#include <QTimer>
#include "CommentExtractor.h"
#include "EditPlanner.h"
#include "FileCrawler.h"
#include "FileWriter.h"

class CommentFilterProxy;
class CommentModel;
class DiffPreview;
class QAction;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QMenu;
class QProgressBar;
class QPushButton;
class QTableView;

/// Main window: folder scanning, the comment table with filters, the
/// before/after preview and the delete / apply / discard workflow.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    QAction* makeAction(QMenu* menu, const QString& text, void (MainWindow::*slot)(), const QKeySequence& shortcut = {});

    // Scanning: the crawler finds files, then they are parsed in parallel.
    void selectFolder();
    void rescan();
    void cancelScan();
    void stopBackgroundWork();
    void onScanFinished();
    void onExtractionFinished();
    void flushExtracted();

    // Browsing and filtering
    void showCurrentComment();
    void showPreview(const QString& filePath, const QVector<EditRange>& ranges, const QString& language, int focusLine);
    void updateFilter();
    void editCustomFilter();
    void clearFilters();
    void selectVisible();
    void deselectAll();

    // Deleting
    void previewDeletion();
    void applyChanges();
    void discardChanges();

    // Other actions
    void openSettings();
    void exportComments();
    void exportAuditLog();
    void harvestSpecialComments();
    void showAbout();

    void setBusy(bool busy);
    void updateActions();
    QString relativePath(const QString& filePath) const;

    CommentModel* m_model;
    CommentFilterProxy* m_proxy;
    QTableView* m_table;
    DiffPreview* m_preview;
    QLineEdit* m_search;
    QComboBox* m_keywordCombo;
    QComboBox* m_languageCombo;
    QCheckBox* m_regexCheck;
    QPushButton* m_deleteBtn;
    QPushButton* m_applyBtn;
    QPushButton* m_discardBtn;
    QAction* m_rescanAct;
    QAction* m_cancelAct;
    QAction* m_harvestAct;
    QLabel* m_pathLabel;
    QLabel* m_statusLabel;
    QLabel* m_countsLabel;
    QProgressBar* m_progress;

    CommentExtractor m_extractor;
    FileWriter m_writer;
    QString m_rootPath;
    QString m_customPath, m_customType;
    EditPlan m_pendingPlan;
    bool m_busy = false;

    QFutureWatcher<FileCrawler::Result> m_scanWatcher;
    QFutureWatcher<void> m_extractWatcher;
    bool m_scanHandled = true, m_extractHandled = true;  // guards against late signals from cancelled runs
    QStringList m_files;          // files being parsed (must outlive the extraction)
    QMutex m_inboxMutex;          // parsed comments waiting to be shown
    QVector<Comment> m_inbox;
    QTimer m_flushTimer;
};

#endif // MAIN_WINDOW_H
