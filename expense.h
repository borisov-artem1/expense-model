#ifndef EXPENSE_H
#define EXPENSE_H

#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

struct Expense
{
    QString   id;
    QDate     spentOn;
    QString   category;
    qint64    amountKopeks;
    QString   comment;
    QDateTime createdAt;

    Expense() : amountKopeks(0) {}
};

namespace ExpenseCategory
{
QStringList keys();
QString     displayName(const QString &key);
QString     keyForDisplay(const QString &displayName);
int         rank(const QString &key);
}

namespace ExpenseStorage
{
const qint64 MinAmountKopeks = 1;
const qint64 MaxAmountKopeks = 100000000;

QString generateId(const QVector<Expense> &existing);

QString formatAmount(qint64 kopeks);

bool parseAmountText(const QString &text, qint64 *kopeks, QString *error);

QJsonObject toJson(const Expense &expense);
QByteArray  saveDocument(const QVector<Expense> &list);
bool        loadDocument(const QByteArray &data, QVector<Expense> &out, QString *error);
}

#endif // EXPENSE_H
