#include "expensefilterproxymodel.h"

ExpenseFilterProxyModel::ExpenseFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void ExpenseFilterProxyModel::setCategoryFilter(const QString &categoryKey)
{
    if (m_categoryKey == categoryKey)
        return;
    m_categoryKey = categoryKey;
    invalidateFilter();
}

void ExpenseFilterProxyModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    m_searchText = text;
    invalidateFilter();
}

bool ExpenseFilterProxyModel::filterAcceptsRow(int sourceRow,
                                               const QModelIndex &sourceParent) const
{
    QAbstractItemModel *source = sourceModel();
    if (!source)
        return false;

    if (!m_categoryKey.isEmpty()) {
        const QModelIndex categoryIndex =
            source->index(sourceRow, ExpenseModel::ColumnCategory, sourceParent);
        if (categoryIndex.data(ExpenseModel::CategoryKeyRole).toString() != m_categoryKey)
            return false;
    }

    if (!m_searchText.isEmpty()) {
        const QModelIndex commentIndex =
            source->index(sourceRow, ExpenseModel::ColumnComment, sourceParent);
        const QString comment = commentIndex.data(Qt::DisplayRole).toString();
        if (comment.isEmpty())
            return false;
        if (!comment.contains(m_searchText, Qt::CaseInsensitive))
            return false;
    }

    return true;
}

bool ExpenseFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.column()) {
    case ExpenseModel::ColumnDate:
        return left.data(ExpenseModel::SortRole).toDate()
               < right.data(ExpenseModel::SortRole).toDate();

    case ExpenseModel::ColumnCategory:
        // порядок задан явно: Транспорт, Питание, Офис, Прочее
        return left.data(ExpenseModel::SortRole).toInt()
               < right.data(ExpenseModel::SortRole).toInt();

    case ExpenseModel::ColumnAmount:
        // сравнение по числу копеек, а не по тексту ячейки
        return left.data(ExpenseModel::SortRole).toLongLong()
               < right.data(ExpenseModel::SortRole).toLongLong();

    default:
        break;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
