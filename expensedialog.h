#ifndef EXPENSEDIALOG_H
#define EXPENSEDIALOG_H

#include <QDialog>

#include "expense.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

class ExpenseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExpenseDialog(QWidget *parent = nullptr);
    ExpenseDialog(const Expense &expense, QWidget *parent = nullptr);

    Expense expense() const { return m_expense; }

public slots:
    void accept() override;

private slots:
    void updateCommentCounter();

private:
    void setupUi();
    bool validate();
    void reportError(QWidget *field, const QString &message);

    Expense m_expense;
    bool    m_isNew;

    QDateEdit      *m_dateEdit;
    QComboBox      *m_categoryCombo;
    QLineEdit      *m_amountEdit;
    QPlainTextEdit *m_commentEdit;
    QLabel         *m_commentCounter;
    QLineEdit      *m_createdAtEdit;
};

#endif // EXPENSEDIALOG_H
