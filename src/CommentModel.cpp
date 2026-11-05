#include "CommentModel.h"
#include <QColor>

int CommentModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : totalCount(); }
int CommentModel::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : ColCount; }

QVariant CommentModel::data(const QModelIndex& index, int role) const {
    const Comment* c = commentAt(index.row());
    if (!c) return {};
    switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
                case ColFile: return c->relativePath.isEmpty() ? c->filePath : c->relativePath;
                case ColLine: return c->startLine;
                case ColType: return c->typeString();
                case ColKeywords: return c->keywordsString();
                case ColPreview: return c->preview(80);
                case ColLanguage: return c->language;
            }
            break;
        case Qt::CheckStateRole:
            if (index.column() == ColFile) return c->isSelected ? Qt::Checked : Qt::Unchecked;
            break;
        case Qt::ToolTipRole:
            if (index.column() == ColPreview) return c->cleanText();
            if (index.column() == ColFile) return c->filePath;
            break;
        case Qt::UserRole:
            return c->id;
        case Qt::BackgroundRole:
            if (c->keywords.contains("TODO", Qt::CaseInsensitive)) return QColor(255, 255, 200);
            if (c->keywords.contains("FIXME", Qt::CaseInsensitive) || c->keywords.contains("BUG", Qt::CaseInsensitive))
                return QColor(255, 220, 220);
            if (c->keywords.contains("HACK", Qt::CaseInsensitive)) return QColor(255, 230, 200);
            break;
        case Qt::ForegroundRole:  // keep text readable on the light highlight colours in dark themes
            if (data(index, Qt::BackgroundRole).isValid()) return QColor(Qt::black);
            break;
    }
    return {};
}

QVariant CommentModel::headerData(int section, Qt::Orientation orientation, int role) const {
    static const char* const titles[] = {"File", "Line", "Type", "Keywords", "Comment", "Language"};
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= ColCount) return {};
    return tr(titles[section]);
}

Qt::ItemFlags CommentModel::flags(const QModelIndex& index) const {
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    return index.column() == ColFile ? f | Qt::ItemIsUserCheckable : f;
}

bool CommentModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role != Qt::CheckStateRole || index.column() != ColFile || !commentAt(index.row())) return false;
    setSelected({index.row()}, value.toInt() == Qt::Checked);
    return true;
}

void CommentModel::addComments(const QVector<Comment>& comments) {
    if (comments.isEmpty()) return;
    beginInsertRows({}, totalCount(), totalCount() + int(comments.size()) - 1);
    m_comments.append(comments);
    endInsertRows();
    emit selectionChanged(m_selectedCount, totalCount());
}

void CommentModel::clear() {
    beginResetModel();
    m_comments = {};  // release the memory, not just the elements
    m_selectedCount = 0;
    endResetModel();
    emit selectionChanged(0, 0);
}

void CommentModel::replaceFiles(const QSet<QString>& filePaths, const QVector<Comment>& fresh) {
    beginResetModel();
    m_comments.removeIf([&](const Comment& c) { return filePaths.contains(c.filePath); });
    m_comments.append(fresh);
    m_selectedCount = int(std::count_if(m_comments.cbegin(), m_comments.cend(), [](const Comment& c) { return c.isSelected; }));
    endResetModel();
    emit selectionChanged(m_selectedCount, totalCount());
}

QVector<const Comment*> CommentModel::selectedComments() const {
    QVector<const Comment*> out;
    out.reserve(m_selectedCount);
    for (const Comment& c : m_comments)
        if (c.isSelected) out << &c;
    return out;
}

QStringList CommentModel::languages() const {
    QSet<QString> set;
    for (const Comment& c : m_comments) set.insert(c.language);
    QStringList list(set.begin(), set.end());
    list.sort(Qt::CaseInsensitive);
    return list;
}

void CommentModel::setSelected(const QVector<int>& rows, bool selected) {
    if (m_comments.isEmpty()) return;
    auto apply = [&](Comment& c) {
        if (c.isSelected != selected) m_selectedCount += selected ? 1 : -1;
        c.isSelected = selected;
    };
    if (rows.isEmpty()) {
        for (Comment& c : m_comments) apply(c);
        emit dataChanged(index(0, ColFile), index(totalCount() - 1, ColFile), {Qt::CheckStateRole});
    } else {
        for (int row : rows) {
            if (row < 0 || row >= totalCount()) continue;
            apply(m_comments[row]);
            emit dataChanged(index(row, ColFile), index(row, ColFile), {Qt::CheckStateRole});
        }
    }
    emit selectionChanged(m_selectedCount, totalCount());
}

void CommentFilterProxy::setFilter(const Filter& filter) {
    m_filter = filter;
    m_regex = QRegularExpression(filter.regex ? filter.search : QString(), QRegularExpression::CaseInsensitiveOption);
    invalidateFilter();
}

bool CommentFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex&) const {
    const Comment* c = static_cast<const CommentModel*>(sourceModel())->commentAt(sourceRow);
    if (!c) return false;
    const Filter& f = m_filter;
    const QString& path = c->relativePath.isEmpty() ? c->filePath : c->relativePath;
    if (!f.path.isEmpty() && !path.contains(f.path, Qt::CaseInsensitive)) return false;
    if (!f.type.isEmpty() && c->typeString() != f.type) return false;
    if (!f.language.isEmpty() && c->language != f.language) return false;
    if (!f.keyword.isEmpty() && !c->keywords.contains(f.keyword, Qt::CaseInsensitive)) return false;
    if (f.search.isEmpty()) return true;
    if (f.regex)  // an invalid (half-typed) regex filters nothing
        return !m_regex.isValid() || m_regex.match(c->rawText).hasMatch() || m_regex.match(path).hasMatch();
    return c->cleanView().contains(f.search, Qt::CaseInsensitive) || path.contains(f.search, Qt::CaseInsensitive);
}
