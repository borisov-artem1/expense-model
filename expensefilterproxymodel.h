#ifndef EXPENSEFILTERPROXYMODEL_H
#define EXPENSEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "expensemodel.h"

class ExpenseFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit ExpenseFilterProxyModel(QObject *parent = nullptr);

    void setCategoryFilter(const QString &categoryKey);
    QString categoryFilter() const { return m_categoryKey; }

    void setSearchText(const QString &text);
    QString searchText() const { return m_searchText; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_categoryKey;
    QString m_searchText;
};

#endif // EXPENSEFILTERPROXYMODEL_H
