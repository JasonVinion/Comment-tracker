#ifndef COMMENT_MODEL_H
#define COMMENT_MODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include "Comment.h"

/**
 * @brief Table model for displaying comments in the main view
 */
class CommentModel : public QAbstractTableModel {
    Q_OBJECT
    
public:
    enum Column {
        ColFile = 0,
        ColLine,
        ColType,
        ColKeywords,
        ColPreview,
        ColLanguage,
        ColCount
    };
    
    explicit CommentModel(QObject* parent = nullptr);
    ~CommentModel() = default;
    
    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    
    /**
     * @brief Add a single comment
     */
    void addComment(const Comment& comment);
    
    /**
     * @brief Add multiple comments
     */
    void addComments(const QVector<Comment>& comments);
    
    /**
     * @brief Clear all comments
     */
    void clear();
    
    /**
     * @brief Get comment at index
     */
    const Comment* commentAt(int row) const;
    
    /**
     * @brief Get comment by ID
     */
    const Comment* commentById(int id) const;
    
    /**
     * @brief Get all comments
     */
    const QVector<Comment>& comments() const { return m_comments; }
    
    /**
     * @brief Get selected comments
     */
    QVector<Comment*> selectedComments();
    
    /**
     * @brief Select/deselect comment
     */
    void setCommentSelected(int row, bool selected);
    
    /**
     * @brief Select all comments
     */
    void selectAll();
    
    /**
     * @brief Deselect all comments
     */
    void deselectAll();
    
    /**
     * @brief Remove comments by ID
     */
    void removeComments(const QVector<int>& ids);
    
    /**
     * @brief Get total count
     */
    int totalCount() const { return m_comments.size(); }
    
    /**
     * @brief Get selected count
     */
    int selectedCount() const;
    
signals:
    void selectionChanged(int selectedCount, int totalCount);
    
private:
    QVector<Comment> m_comments;
};

/**
 * @brief Filter proxy for searching and filtering comments
 */
class CommentFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
    
public:
    explicit CommentFilterProxy(QObject* parent = nullptr);
    
    /**
     * @brief Set text search filter
     */
    void setSearchText(const QString& text);
    
    /**
     * @brief Set keyword filter (TODO, FIXME, etc.)
     */
    void setKeywordFilter(const QStringList& keywords);
    
    /**
     * @brief Set language filter
     */
    void setLanguageFilter(const QStringList& languages);
    
    /**
     * @brief Set path filter
     */
    void setPathFilter(const QString& pathSubstring);
    
    /**
     * @brief Enable regex search
     */
    void setUseRegex(bool use);

    /**
     * @brief Set comment type filter (Line, Block, Doc, Inline)
     */
    void setTypeFilter(const QStringList& types);
    
    /**
     * @brief Clear all filters
     */
    void clearFilters();
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    
private:
    QString m_searchText;
    QRegularExpression m_searchRegex;
    QStringList m_keywordFilter;
    QStringList m_languageFilter;
    QString m_pathFilter;
    QStringList m_typeFilter;
    bool m_useRegex = false;
};

#endif // COMMENT_MODEL_H
