#include "expensemodel.h"

ExpenseModel::ExpenseModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ExpenseModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_expenses.size();
}

int ExpenseModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ExpenseModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_expenses.size())
        return QVariant();

    const Expense &expense = m_expenses.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColumnDate:     return expense.spentOn.toString(QStringLiteral("dd.MM.yyyy"));
        case ColumnCategory: return ExpenseCategory::displayName(expense.category);
        case ColumnAmount:   return ExpenseStorage::formatAmount(expense.amountKopeks);
        case ColumnComment:  return expense.comment;
        default:             break;
        }
        break;

    case Qt::TextAlignmentRole:
        if (index.column() == ColumnAmount)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        if (index.column() == ColumnDate)
            return int(Qt::AlignCenter);
        break;

    case Qt::ToolTipRole:
        return expense.comment.isEmpty() ? QVariant() : QVariant(expense.comment);

    case IdRole:
        return expense.id;

    case CategoryKeyRole:
        return expense.category;

    case KopeksRole:
        return expense.amountKopeks;

    case SortRole:
        switch (index.column()) {
        case ColumnDate:     return expense.spentOn;
        case ColumnCategory: return ExpenseCategory::rank(expense.category);
        case ColumnAmount:   return expense.amountKopeks;
        case ColumnComment:  return expense.comment;
        default:             break;
        }
        break;

    default:
        break;
    }

    return QVariant();
}

QVariant ExpenseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case ColumnDate:     return QStringLiteral("Дата");
    case ColumnCategory: return QStringLiteral("Категория");
    case ColumnAmount:   return QStringLiteral("Сумма");
    case ColumnComment:  return QStringLiteral("Комментарий");
    default:             break;
    }
    return QVariant();
}

void ExpenseModel::setExpenses(const QVector<Expense> &expenses)
{
    beginResetModel();
    m_expenses = expenses;
    endResetModel();
}

void ExpenseModel::addExpense(const Expense &expense)
{
    const int row = m_expenses.size();
    beginInsertRows(QModelIndex(), row, row);
    m_expenses.append(expense);
    endInsertRows();
}

void ExpenseModel::updateExpense(int row, const Expense &expense)
{
    if (row < 0 || row >= m_expenses.size())
        return;

    m_expenses[row] = expense;
    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
}

void ExpenseModel::removeExpense(int row)
{
    if (row < 0 || row >= m_expenses.size())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_expenses.remove(row);
    endRemoveRows();
}
