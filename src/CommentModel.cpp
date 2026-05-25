#include "CommentModel.h"
#include <QColor>

CommentModel::CommentModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

int CommentModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_comments.size();
}

int CommentModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant CommentModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_comments.size()) {
        return QVariant();
    }
    
    const Comment& comment = m_comments[index.row()];
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColFile:
                return comment.relativePath.isEmpty() ? comment.filePath : comment.relativePath;
            case ColLine:
                return comment.startLine;
            case ColType:
                return comment.typeString();
            case ColKeywords:
                return comment.keywordsString();
            case ColPreview:
                return comment.preview(80);
            case ColLanguage:
                return comment.language;
        }
    } else if (role == Qt::CheckStateRole && index.column() == ColFile) {
        return comment.isSelected ? Qt::Checked : Qt::Unchecked;
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == ColPreview) {
            return comment.cleanText;
        } else if (index.column() == ColFile) {
            return comment.filePath;
        }
    } else if (role == Qt::UserRole) {
        return comment.id;
    } else if (role == Qt::BackgroundRole) {
        if (!comment.keywords.isEmpty()) {
            // Highlight rows with keywords
            if (comment.keywords.contains("TODO")) {
                return QColor(255, 255, 200);  // Light yellow
            } else if (comment.keywords.contains("FIXME") || comment.keywords.contains("BUG")) {
                return QColor(255, 220, 220);  // Light red
            } else if (comment.keywords.contains("HACK")) {
                return QColor(255, 230, 200);  // Light orange
            }
        }
    }
    
    return QVariant();
}

QVariant CommentModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    
    switch (section) {
        case ColFile: return "File";
        case ColLine: return "Line";
        case ColType: return "Type";
        case ColKeywords: return "Keywords";
        case ColPreview: return "Comment";
        case ColLanguage: return "Language";
    }
    
    return QVariant();
}

Qt::ItemFlags CommentModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    
    if (index.column() == ColFile) {
        flags |= Qt::ItemIsUserCheckable;
    }
    
    return flags;
}

bool CommentModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_comments.size()) {
        return false;
    }
    
    if (role == Qt::CheckStateRole && index.column() == ColFile) {
        m_comments[index.row()].isSelected = (value.toInt() == Qt::Checked);
        emit dataChanged(index, index, {role});
        emit selectionChanged(selectedCount(), totalCount());
        return true;
    }
    
    return false;
}

void CommentModel::addComment(const Comment& comment) {
    beginInsertRows(QModelIndex(), m_comments.size(), m_comments.size());
    m_comments.append(comment);
    endInsertRows();
}

void CommentModel::addComments(const QVector<Comment>& comments) {
    if (comments.isEmpty()) return;
    
    beginInsertRows(QModelIndex(), m_comments.size(), m_comments.size() + comments.size() - 1);
    m_comments.append(comments);
    endInsertRows();
}

void CommentModel::clear() {
    beginResetModel();
    m_comments.clear();
    endResetModel();
}

const Comment* CommentModel::commentAt(int row) const {
    if (row >= 0 && row < m_comments.size()) {
        return &m_comments[row];
    }
    return nullptr;
}

const Comment* CommentModel::commentById(int id) const {
    for (const Comment& c : m_comments) {
        if (c.id == id) {
            return &c;
        }
    }
    return nullptr;
}

QVector<Comment*> CommentModel::selectedComments() {
    QVector<Comment*> selected;
    for (Comment& c : m_comments) {
        if (c.isSelected) {
            selected.append(&c);
        }
    }
    return selected;
}

void CommentModel::setCommentSelected(int row, bool selected) {
    if (row >= 0 && row < m_comments.size()) {
        m_comments[row].isSelected = selected;
        QModelIndex index = this->index(row, ColFile);
        emit dataChanged(index, index, {Qt::CheckStateRole});
        emit selectionChanged(selectedCount(), totalCount());
    }
}

void CommentModel::selectAll() {
    if (m_comments.isEmpty()) {
        emit selectionChanged(0, 0);
        return;
    }
    for (Comment& c : m_comments) {
        c.isSelected = true;
    }
    emit dataChanged(index(0, ColFile), index(m_comments.size() - 1, ColFile), {Qt::CheckStateRole});
    emit selectionChanged(selectedCount(), totalCount());
}

void CommentModel::deselectAll() {
    if (m_comments.isEmpty()) {
        emit selectionChanged(0, 0);
        return;
    }
    for (Comment& c : m_comments) {
        c.isSelected = false;
    }
    emit dataChanged(index(0, ColFile), index(m_comments.size() - 1, ColFile), {Qt::CheckStateRole});
    emit selectionChanged(selectedCount(), totalCount());
}

void CommentModel::removeComments(const QVector<int>& ids) {
    QSet<int> idsToRemove(ids.begin(), ids.end());
    
    beginResetModel();
    m_comments.erase(
        std::remove_if(m_comments.begin(), m_comments.end(),
            [&idsToRemove](const Comment& c) { return idsToRemove.contains(c.id); }),
        m_comments.end()
    );
    endResetModel();
}

int CommentModel::selectedCount() const {
    int count = 0;
    for (const Comment& c : m_comments) {
        if (c.isSelected) count++;
    }
    return count;
}

// CommentFilterProxy implementation

CommentFilterProxy::CommentFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void CommentFilterProxy::setSearchText(const QString& text) {
    m_searchText = text;
    if (m_useRegex) {
        m_searchRegex.setPattern(text);
        m_searchRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    invalidateFilter();
}

void CommentFilterProxy::setKeywordFilter(const QStringList& keywords) {
    m_keywordFilter = keywords;
    invalidateFilter();
}

void CommentFilterProxy::setLanguageFilter(const QStringList& languages) {
    m_languageFilter = languages;
    invalidateFilter();
}

void CommentFilterProxy::setPathFilter(const QString& pathSubstring) {
    m_pathFilter = pathSubstring;
    invalidateFilter();
}

void CommentFilterProxy::setUseRegex(bool use) {
    m_useRegex = use;
    if (use && !m_searchText.isEmpty()) {
        m_searchRegex.setPattern(m_searchText);
        m_searchRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    }
    invalidateFilter();
}

void CommentFilterProxy::setTypeFilter(const QStringList& types) {
    m_typeFilter = types;
    invalidateFilter();
}

void CommentFilterProxy::clearFilters() {
    m_searchText.clear();
    m_searchRegex = QRegularExpression();
    m_keywordFilter.clear();
    m_languageFilter.clear();
    m_pathFilter.clear();
    m_typeFilter.clear();
    invalidateFilter();
}

bool CommentFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    QModelIndex fileIndex = sourceModel()->index(sourceRow, CommentModel::ColFile, sourceParent);
    QModelIndex keywordsIndex = sourceModel()->index(sourceRow, CommentModel::ColKeywords, sourceParent);
    QModelIndex previewIndex = sourceModel()->index(sourceRow, CommentModel::ColPreview, sourceParent);
    QModelIndex languageIndex = sourceModel()->index(sourceRow, CommentModel::ColLanguage, sourceParent);
    QModelIndex typeIndex = sourceModel()->index(sourceRow, CommentModel::ColType, sourceParent);
    
    QString filePath = sourceModel()->data(fileIndex).toString();
    QString keywords = sourceModel()->data(keywordsIndex).toString();
    QString preview = sourceModel()->data(previewIndex).toString();
    QString language = sourceModel()->data(languageIndex).toString();
    QString type = sourceModel()->data(typeIndex).toString();
    
    // Path filter
    if (!m_pathFilter.isEmpty()) {
        if (!filePath.contains(m_pathFilter, Qt::CaseInsensitive)) {
            return false;
        }

    // Type filter
    if (!m_typeFilter.isEmpty()) {
        bool matchesType = false;
        for (const QString& typeFilter : m_typeFilter) {
            if (type.compare(typeFilter, Qt::CaseInsensitive) == 0) {
                matchesType = true;
                break;
            }
        }
        if (!matchesType) {
            return false;
        }
    }
    }
    
    // Language filter
    if (!m_languageFilter.isEmpty()) {
        if (!m_languageFilter.contains(language, Qt::CaseInsensitive)) {
            return false;
        }
    }
    
    // Keyword filter
    if (!m_keywordFilter.isEmpty()) {
        bool hasKeyword = false;
        for (const QString& kw : m_keywordFilter) {
            if (keywords.contains(kw, Qt::CaseInsensitive)) {
                hasKeyword = true;
                break;
            }
        }
        if (!hasKeyword) {
            return false;
        }
    }
    
    // Text search
    if (!m_searchText.isEmpty()) {
        if (m_useRegex) {
            if (!m_searchRegex.isValid()) {
                return true;  // Invalid regex, don't filter
            }
            return m_searchRegex.match(preview).hasMatch() ||
                   m_searchRegex.match(filePath).hasMatch();
        } else {
            return preview.contains(m_searchText, Qt::CaseInsensitive) ||
                   filePath.contains(m_searchText, Qt::CaseInsensitive);
        }
    }
    
    return true;
}
