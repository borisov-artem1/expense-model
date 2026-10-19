#ifndef EXPENSEMODEL_H
#define EXPENSEMODEL_H

#include <QAbstractTableModel>

#include "expense.h"

class ExpenseModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColumnDate = 0,
        ColumnCategory,
        ColumnAmount,
        ColumnComment,
        ColumnCount
    };

    enum Role {
        IdRole = Qt::UserRole + 1,
        CategoryKeyRole,
        KopeksRole,
        SortRole
    };

    explicit ExpenseModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setExpenses(const QVector<Expense> &expenses);
    const QVector<Expense> &expenses() const { return m_expenses; }
    const Expense &expenseAt(int row) const { return m_expenses.at(row); }

    void addExpense(const Expense &expense);
    void updateExpense(int row, const Expense &expense);
    void removeExpense(int row);

private:
    QVector<Expense> m_expenses;
};

#endif // EXPENSEMODEL_H
