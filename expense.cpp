#include "expense.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>

#include <cmath>

namespace {

const int kCategoryCount = 4;
const char *const kCategoryKeys[kCategoryCount]    = { "Transport", "Food", "Office", "Other" };
const char *const kCategoryDisplay[kCategoryCount] = { "Транспорт", "Питание", "Офис", "Прочее" };

const int kMaxCommentLength = 300;

void setError(QString *error, const QString &text)
{
    if (error)
        *error = text;
}

bool isKnownCategoryKey(const QString &key)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        if (key == QLatin1String(kCategoryKeys[i]))
            return true;
    }
    return false;
}

bool parseExpenseObject(const QJsonObject &obj, int index, Expense &out, QString *error)
{
    const QString prefix = QStringLiteral("Запись №%1: ").arg(index + 1);

    // ---- id -----------------------------------------------------------------
    if (!obj.contains(QStringLiteral("id"))) {
        setError(error, prefix + QStringLiteral("отсутствует обязательное поле id."));
        return false;
    }
    const QJsonValue idValue = obj.value(QStringLiteral("id"));
    if (!idValue.isString()) {
        setError(error, prefix + QStringLiteral("поле id должно быть строкой."));
        return false;
    }
    const QString id = idValue.toString();
    if (id.isEmpty()) {
        setError(error, prefix + QStringLiteral("поле id не может быть пустым."));
        return false;
    }

    // ---- spentOn ------------------------------------------------------------
    if (!obj.contains(QStringLiteral("spentOn"))) {
        setError(error, prefix + QStringLiteral("отсутствует обязательное поле spentOn."));
        return false;
    }
    const QJsonValue spentOnValue = obj.value(QStringLiteral("spentOn"));
    if (!spentOnValue.isString()) {
        setError(error, prefix + QStringLiteral("поле spentOn должно быть строкой."));
        return false;
    }
    const QDate spentOn = QDate::fromString(spentOnValue.toString(),
                                            QStringLiteral("yyyy-MM-dd"));
    if (!spentOn.isValid()) {
        setError(error, prefix + QStringLiteral("поле spentOn должно быть датой в формате yyyy-MM-dd."));
        return false;
    }
    if (spentOn > QDate::currentDate()) {
        setError(error, prefix + QStringLiteral("дата расхода позже сегодняшней."));
        return false;
    }

    // ---- category -----------------------------------------------------------
    if (!obj.contains(QStringLiteral("category"))) {
        setError(error, prefix + QStringLiteral("отсутствует обязательное поле category."));
        return false;
    }
    const QJsonValue categoryValue = obj.value(QStringLiteral("category"));
    if (!categoryValue.isString()) {
        setError(error, prefix + QStringLiteral("поле category должно быть строкой."));
        return false;
    }
    const QString category = categoryValue.toString();
    if (!isKnownCategoryKey(category)) {
        setError(error, prefix + QStringLiteral("неизвестная категория «%1».").arg(category));
        return false;
    }

    // ---- amountKopeks -------------------------------------------------------
    if (!obj.contains(QStringLiteral("amountKopeks"))) {
        setError(error, prefix + QStringLiteral("отсутствует обязательное поле amountKopeks."));
        return false;
    }
    const QJsonValue amountValue = obj.value(QStringLiteral("amountKopeks"));
    if (!amountValue.isDouble()) {
        setError(error, prefix + QStringLiteral("поле amountKopeks должно быть числом, а не строкой."));
        return false;
    }
    const double amountRaw = amountValue.toDouble();
    if (!qIsFinite(amountRaw) || amountRaw != std::floor(amountRaw)) {
        setError(error, prefix + QStringLiteral("поле amountKopeks должно быть целым числом."));
        return false;
    }
    if (amountRaw < double(ExpenseStorage::MinAmountKopeks)
        || amountRaw > double(ExpenseStorage::MaxAmountKopeks)) {
        setError(error, prefix + QStringLiteral("сумма вне допустимого диапазона "
                                                "(от 1 до 100000000 копеек)."));
        return false;
    }

    // ---- comment -------------------------------------------
    QString comment;
    if (obj.contains(QStringLiteral("comment"))) {
        const QJsonValue commentValue = obj.value(QStringLiteral("comment"));
        if (!commentValue.isString()) {
            setError(error, prefix + QStringLiteral("поле comment должно быть строкой."));
            return false;
        }
        comment = commentValue.toString();
        if (comment.length() > kMaxCommentLength) {
            setError(error, prefix + QStringLiteral("комментарий длиннее %1 символов.")
                                         .arg(kMaxCommentLength));
            return false;
        }
    }

    // ---- createdAt ----------------------------------------------------------
    if (!obj.contains(QStringLiteral("createdAt"))) {
        setError(error, prefix + QStringLiteral("отсутствует обязательное поле createdAt."));
        return false;
    }
    const QJsonValue createdValue = obj.value(QStringLiteral("createdAt"));
    if (!createdValue.isString()) {
        setError(error, prefix + QStringLiteral("поле createdAt должно быть строкой."));
        return false;
    }
    const QDateTime createdAt = QDateTime::fromString(createdValue.toString(),
                                                      QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    if (!createdAt.isValid()) {
        setError(error, prefix + QStringLiteral("поле createdAt должно быть в формате "
                                                "yyyy-MM-ddTHH:mm:ss."));
        return false;
    }

    out.id           = id;
    out.spentOn      = spentOn;
    out.category     = category;
    out.amountKopeks = qint64(amountRaw);
    out.comment      = comment;
    out.createdAt    = createdAt;
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// ExpenseCategory
// ---------------------------------------------------------------------------

QStringList ExpenseCategory::keys()
{
    QStringList list;
    for (int i = 0; i < kCategoryCount; ++i)
        list << QString::fromLatin1(kCategoryKeys[i]);
    return list;
}

QString ExpenseCategory::displayName(const QString &key)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        if (key == QLatin1String(kCategoryKeys[i]))
            return QString::fromUtf8(kCategoryDisplay[i]);
    }
    return key;
}

QString ExpenseCategory::keyForDisplay(const QString &displayName)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        if (displayName == QString::fromUtf8(kCategoryDisplay[i]))
            return QString::fromLatin1(kCategoryKeys[i]);
    }
    return QString();
}

int ExpenseCategory::rank(const QString &key)
{
    for (int i = 0; i < kCategoryCount; ++i) {
        if (key == QLatin1String(kCategoryKeys[i]))
            return i;
    }
    return kCategoryCount;
}

// ---------------------------------------------------------------------------
// ExpenseStorage
// ---------------------------------------------------------------------------

QString ExpenseStorage::generateId(const QVector<Expense> &existing)
{
    QSet<QString> used;
    for (int i = 0; i < existing.size(); ++i)
        used.insert(existing.at(i).id);

    qint64 candidate = 1;
    while (used.contains(QString::number(candidate)))
        ++candidate;
    return QString::number(candidate);
}

QString ExpenseStorage::formatAmount(qint64 kopeks)
{
    const bool negative = kopeks < 0;
    const qint64 absolute = negative ? -kopeks : kopeks;

    const qint64 rubles = absolute / 100;
    const qint64 rest   = absolute % 100;

    QString result = QString::number(rubles)
                     + QLatin1Char(',')
                     + QString::number(rest).rightJustified(2, QLatin1Char('0'));

    if (negative)
        result.prepend(QLatin1Char('-'));

    return result;
}

bool ExpenseStorage::parseAmountText(const QString &text, qint64 *kopeks, QString *error)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        setError(error, QStringLiteral("Введите сумму расхода."));
        return false;
    }

    QString normalized = trimmed;
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));

    int dotCount = 0;
    for (int i = 0; i < normalized.size(); ++i) {
        const QChar c = normalized.at(i);
        if (c == QLatin1Char('.')) {
            ++dotCount;
            continue;
        }
        if (!c.isDigit()) {
            setError(error, QStringLiteral("Сумма должна содержать только цифры и разделитель "
                                           "(запятую или точку)."));
            return false;
        }
    }
    if (dotCount > 1) {
        setError(error, QStringLiteral("В сумме допускается только один разделитель."));
        return false;
    }

    const int dotPos = normalized.indexOf(QLatin1Char('.'));
    QString integerPart;
    QString fractionPart;
    if (dotPos < 0) {
        integerPart = normalized;
    } else {
        integerPart  = normalized.left(dotPos);
        fractionPart = normalized.mid(dotPos + 1);
    }

    if (fractionPart.size() > 2) {
        setError(error, QStringLiteral("После разделителя допускается не более двух знаков."));
        return false;
    }
    if (integerPart.isEmpty())
        integerPart = QStringLiteral("0");

    bool ok = false;
    const qint64 rubles = integerPart.toLongLong(&ok);
    if (!ok) {
        setError(error, QStringLiteral("Слишком большое значение суммы."));
        return false;
    }
    if (rubles > MaxAmountKopeks / 100) {
        setError(error, QStringLiteral("Сумма вне допустимого диапазона: "
                                       "от 0,01 до 1000000,00 руб."));
        return false;
    }

    while (fractionPart.size() < 2)
        fractionPart.append(QLatin1Char('0'));

    qint64 rest = 0;
    if (!fractionPart.isEmpty()) {
        rest = fractionPart.toLongLong(&ok);
        if (!ok) {
            setError(error, QStringLiteral("Некорректная дробная часть суммы."));
            return false;
        }
    }

    const qint64 total = rubles * 100 + rest;
    if (total < MinAmountKopeks) {
        setError(error, QStringLiteral("Сумма должна быть не меньше 0,01 руб."));
        return false;
    }
    if (total > MaxAmountKopeks) {
        setError(error, QStringLiteral("Сумма не должна превышать 1000000,00 руб."));
        return false;
    }

    if (kopeks)
        *kopeks = total;
    return true;
}

QJsonObject ExpenseStorage::toJson(const Expense &expense)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("id"),
               expense.id);
    obj.insert(QStringLiteral("spentOn"),
               expense.spentOn.toString(QStringLiteral("yyyy-MM-dd")));
    obj.insert(QStringLiteral("category"),
               expense.category);
    obj.insert(QStringLiteral("amountKopeks"),
               static_cast<double>(expense.amountKopeks));
    if (!expense.comment.isEmpty())
        obj.insert(QStringLiteral("comment"), expense.comment);
    obj.insert(QStringLiteral("createdAt"),
               expense.createdAt.toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss")));
    return obj;
}

QByteArray ExpenseStorage::saveDocument(const QVector<Expense> &list)
{
    QJsonArray array;
    for (int i = 0; i < list.size(); ++i)
        array.append(toJson(list.at(i)));

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("expenses"), array);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool ExpenseStorage::loadDocument(const QByteArray &data, QVector<Expense> &out, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        setError(error, QStringLiteral("Файл не является корректным JSON: %1 (позиция %2).")
                            .arg(parseError.errorString())
                            .arg(parseError.offset));
        return false;
    }
    if (!document.isObject()) {
        setError(error, QStringLiteral("Корень файла должен быть объектом JSON."));
        return false;
    }

    const QJsonObject root = document.object();

    if (!root.contains(QStringLiteral("version"))) {
        setError(error, QStringLiteral("В файле отсутствует поле version."));
        return false;
    }
    const QJsonValue versionValue = root.value(QStringLiteral("version"));
    if (!versionValue.isDouble() || versionValue.toInt(-1) != 1) {
        setError(error, QStringLiteral("Поле version должно быть равно 1."));
        return false;
    }

    if (!root.contains(QStringLiteral("expenses"))) {
        setError(error, QStringLiteral("В файле отсутствует массив expenses."));
        return false;
    }
    const QJsonValue expensesValue = root.value(QStringLiteral("expenses"));
    if (!expensesValue.isArray()) {
        setError(error, QStringLiteral("Поле expenses должно быть массивом."));
        return false;
    }
    const QJsonArray array = expensesValue.toArray();

    QVector<Expense> result;
    result.reserve(array.size());

    QSet<QString> usedIds;

    for (int i = 0; i < array.size(); ++i) {
        if (!array.at(i).isObject()) {
            setError(error, QStringLiteral("Элемент expenses[%1] не является объектом.").arg(i));
            return false;
        }

        Expense expense;
        QString itemError;
        if (!parseExpenseObject(array.at(i).toObject(), i, expense, &itemError)) {
            setError(error, itemError);
            return false;
        }

        if (usedIds.contains(expense.id)) {
            setError(error, QStringLiteral("Запись №%1: повторяющийся id «%2».")
                                .arg(i + 1)
                                .arg(expense.id));
            return false;
        }
        usedIds.insert(expense.id);

        result.append(expense);
    }

    out = result;
    return true;
}
