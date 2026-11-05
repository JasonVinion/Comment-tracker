#ifndef COMMENT_MODEL_H
#define COMMENT_MODEL_H

#include <QAbstractTableModel>
#include <QRegularExpression>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QVector>
#include "Comment.h"

/// Table of extracted comments with a checkbox per row.
class CommentModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { ColFile, ColLine, ColType, ColKeywords, ColPreview, ColLanguage, ColCount };

    using QAbstractTableModel::QAbstractTableModel;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    void addComments(const QVector<Comment>& comments);
    void clear();
    /// Drops every comment of the given files and appends the fresh ones (after files changed on disk).
    void replaceFiles(const QSet<QString>& filePaths, const QVector<Comment>& fresh);

    const Comment* commentAt(int row) const { return row >= 0 && row < m_comments.size() ? &m_comments[row] : nullptr; }
    const QVector<Comment>& comments() const { return m_comments; }
    QVector<const Comment*> selectedComments() const;
    QStringList languages() const;

    /// Checks/unchecks the given rows, or every row when `rows` is empty.
    void setSelected(const QVector<int>& rows, bool selected);
    int selectedCount() const { return m_selectedCount; }
    int totalCount() const { return int(m_comments.size()); }

signals:
    void selectionChanged(int selectedCount, int totalCount);

private:
    QVector<Comment> m_comments;
    int m_selectedCount = 0;
};

/// Filters the comment table by text, keyword, language, path and comment type.
class CommentFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    struct Filter {
        QString search;
        bool regex = false;
        QString keyword, language, path, type;  // empty = any
    };

    using QSortFilterProxyModel::QSortFilterProxyModel;
    void setFilter(const Filter& filter);
    const Filter& filter() const { return m_filter; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    Filter m_filter;
    QRegularExpression m_regex;
};

#endif // COMMENT_MODEL_H
