#include "expensedialog.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const int kMaxCommentLength = 300;
}

ExpenseDialog::ExpenseDialog(QWidget *parent)
    : QDialog(parent)
    , m_isNew(true)
{
    m_expense.createdAt = QDateTime::currentDateTime();
    setupUi();
}

ExpenseDialog::ExpenseDialog(const Expense &expense, QWidget *parent)
    : QDialog(parent)
    , m_expense(expense)
    , m_isNew(false)
{
    setupUi();
}

void ExpenseDialog::setupUi()
{
    setWindowTitle(m_isNew ? QStringLiteral("Новый расход")
                           : QStringLiteral("Изменение расхода"));
    setModal(true);

    // ---- date ---------------------------------------------------------------
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_dateEdit->setDate(m_expense.spentOn.isValid() ? m_expense.spentOn
                                                    : QDate::currentDate());
    m_dateEdit->setMaximumDate(QDate::currentDate());

    // ---- category ----------------------------------------------------------
    m_categoryCombo = new QComboBox(this);
    const QStringList keys = ExpenseCategory::keys();
    for (int i = 0; i < keys.size(); ++i)
        m_categoryCombo->addItem(ExpenseCategory::displayName(keys.at(i)), keys.at(i));

    const int categoryIndex = m_categoryCombo->findData(m_expense.category);
    m_categoryCombo->setCurrentIndex(categoryIndex >= 0 ? categoryIndex : 0);

    // ---- sum --------------------------------------------------------------
    m_amountEdit = new QLineEdit(this);
    m_amountEdit->setPlaceholderText(QStringLiteral("например, 1860,50"));
    if (m_expense.amountKopeks > 0)
        m_amountEdit->setText(ExpenseStorage::formatAmount(m_expense.amountKopeks));

    // ---- comment --------------------------------------------------------
    m_commentEdit = new QPlainTextEdit(this);
    m_commentEdit->setPlainText(m_expense.comment);
    m_commentEdit->setTabChangesFocus(true);
    m_commentEdit->setFixedHeight(80);

    m_commentCounter = new QLabel(this);

    // ---- createdAt --------------------------------------
    m_createdAtEdit = new QLineEdit(this);
    m_createdAtEdit->setReadOnly(true);
    m_createdAtEdit->setText(m_expense.createdAt.toString(QStringLiteral("dd.MM.yyyy HH:mm:ss")));

    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("Дата:"), m_dateEdit);
    form->addRow(QStringLiteral("Категория:"), m_categoryCombo);
    form->addRow(QStringLiteral("Сумма, руб.:"), m_amountEdit);
    form->addRow(QStringLiteral("Комментарий:"), m_commentEdit);
    form->addRow(QString(), m_commentCounter);
    form->addRow(QStringLiteral("Создана:"), m_createdAtEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    QPushButton *saveButton   = buttons->addButton(QStringLiteral("Сохранить"),
                                                 QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttons->addButton(QStringLiteral("Отмена"),
                                                   QDialogButtonBox::RejectRole);

    connect(saveButton,   &QPushButton::clicked, this, &ExpenseDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &ExpenseDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_commentEdit, &QPlainTextEdit::textChanged,
            this, &ExpenseDialog::updateCommentCounter);
    updateCommentCounter();

    resize(460, 340);
    m_dateEdit->setFocus();
}

void ExpenseDialog::updateCommentCounter()
{
    const int length = m_commentEdit->toPlainText().length();
    m_commentCounter->setText(QStringLiteral("Символов: %1 из %2")
                                  .arg(length)
                                  .arg(kMaxCommentLength));
    m_commentCounter->setStyleSheet(length > kMaxCommentLength
                                        ? QStringLiteral("color: #c0392b;")
                                        : QStringLiteral("color: gray;"));
}

void ExpenseDialog::reportError(QWidget *field, const QString &message)
{
    QMessageBox::warning(this, QStringLiteral("Проверка данных"), message);
    if (field)
        field->setFocus();
}

bool ExpenseDialog::validate()
{
    const QDate date = m_dateEdit->date();
    if (!date.isValid()) {
        reportError(m_dateEdit, QStringLiteral("Укажите дату расхода."));
        return false;
    }
    if (date > QDate::currentDate()) {
        reportError(m_dateEdit,
                    QStringLiteral("Дата расхода не может быть позже сегодняшней."));
        return false;
    }

    const QString categoryKey = m_categoryCombo->currentData().toString();
    if (categoryKey.isEmpty()) {
        reportError(m_categoryCombo, QStringLiteral("Выберите категорию."));
        return false;
    }

    qint64 kopeks = 0;
    QString amountError;
    if (!ExpenseStorage::parseAmountText(m_amountEdit->text(), &kopeks, &amountError)) {
        reportError(m_amountEdit, amountError);
        return false;
    }

    const QString comment = m_commentEdit->toPlainText();
    if (comment.length() > kMaxCommentLength) {
        reportError(m_commentEdit,
                    QStringLiteral("Комментарий не должен превышать %1 символов (сейчас %2).")
                        .arg(kMaxCommentLength)
                        .arg(comment.length()));
        return false;
    }

    m_expense.spentOn      = date;
    m_expense.category     = categoryKey;
    m_expense.amountKopeks = kopeks;
    m_expense.comment      = comment;

    return true;
}

void ExpenseDialog::accept()
{
    if (!validate())
        return;
    QDialog::accept();
}
