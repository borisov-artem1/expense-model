#include "mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "expensedialog.h"
#include "expensefilterproxymodel.h"
#include "expensemodel.h"

namespace {
const char *const kLastFileKey = "lastFile";
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_model(new ExpenseModel(this))
    , m_proxy(new ExpenseFilterProxyModel(this))
    , m_table(nullptr)
    , m_categoryFilter(nullptr)
    , m_searchEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_modified(false)
    , m_sortColumn(-1)
    , m_sortOrder(Qt::AscendingOrder)
{
    m_proxy->setSourceModel(m_model);

    createActions();
    createMenus();
    createCentralWidget();
    createStatusBar();

    connect(m_model, &QAbstractItemModel::modelReset,
            this, &MainWindow::updateStatusBar);
    connect(m_proxy, &QAbstractItemModel::modelReset,
            this, &MainWindow::updateStatusBar);
    connect(m_proxy, &QAbstractItemModel::rowsInserted,
            this, &MainWindow::updateStatusBar);
    connect(m_proxy, &QAbstractItemModel::rowsRemoved,
            this, &MainWindow::updateStatusBar);
    connect(m_proxy, &QAbstractItemModel::layoutChanged,
            this, &MainWindow::updateStatusBar);

    updateWindowTitle();
    updateStatusBar();
    updateActions();

    QTimer::singleShot(0, this, &MainWindow::offerToOpenLastFile);
}

void MainWindow::createActions()
{
    m_actionNew = new QAction(QStringLiteral("Создать"), this);
    m_actionNew->setShortcut(QKeySequence::New);
    connect(m_actionNew, &QAction::triggered, this, &MainWindow::onNewDocument);

    m_actionOpen = new QAction(QStringLiteral("Открыть..."), this);
    m_actionOpen->setShortcut(QKeySequence::Open);
    connect(m_actionOpen, &QAction::triggered, this, &MainWindow::onOpenDocument);

    m_actionSave = new QAction(QStringLiteral("Сохранить"), this);
    m_actionSave->setShortcut(QKeySequence::Save);
    connect(m_actionSave, &QAction::triggered, this, &MainWindow::onSaveDocument);

    m_actionSaveAs = new QAction(QStringLiteral("Сохранить как..."), this);
    m_actionSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(m_actionSaveAs, &QAction::triggered, this, &MainWindow::onSaveDocumentAs);

    m_actionExit = new QAction(QStringLiteral("Выход"), this);
    m_actionExit->setShortcut(QKeySequence::Quit);
    connect(m_actionExit, &QAction::triggered, this, &MainWindow::close);

    m_actionAdd = new QAction(QStringLiteral("Добавить"), this);
    m_actionAdd->setShortcut(QKeySequence(Qt::Key_Insert));
    connect(m_actionAdd, &QAction::triggered, this, &MainWindow::onAddExpense);

    m_actionEdit = new QAction(QStringLiteral("Изменить"), this);
    m_actionEdit->setShortcut(QKeySequence(Qt::Key_F2));
    connect(m_actionEdit, &QAction::triggered, this, &MainWindow::onEditExpense);

    m_actionDelete = new QAction(QStringLiteral("Удалить"), this);
    m_actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(m_actionDelete, &QAction::triggered, this, &MainWindow::onDeleteExpense);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("Файл"));
    fileMenu->addAction(m_actionNew);
    fileMenu->addAction(m_actionOpen);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionSave);
    fileMenu->addAction(m_actionSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actionExit);

    QMenu *expenseMenu = menuBar()->addMenu(QStringLiteral("Расходы"));
    expenseMenu->addAction(m_actionAdd);
    expenseMenu->addAction(m_actionEdit);
    expenseMenu->addAction(m_actionDelete);
}

void MainWindow::createCentralWidget()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ---- строка фильтра и поиска -------------------------------------------
    QHBoxLayout *filterLayout = new QHBoxLayout;

    filterLayout->addWidget(new QLabel(QStringLiteral("Категория:"), central));

    m_categoryFilter = new QComboBox(central);
    m_categoryFilter->addItem(QStringLiteral("Все"), QString());
    const QStringList keys = ExpenseCategory::keys();
    for (int i = 0; i < keys.size(); ++i)
        m_categoryFilter->addItem(ExpenseCategory::displayName(keys.at(i)), keys.at(i));
    filterLayout->addWidget(m_categoryFilter);

    filterLayout->addSpacing(16);
    filterLayout->addWidget(new QLabel(QStringLiteral("Поиск по комментарию:"), central));

    m_searchEdit = new QLineEdit(central);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setPlaceholderText(QStringLiteral("подстрока комментария"));
    filterLayout->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(filterLayout);

    // ---- таблица ------------------------------------------------------------
    m_table = new QTableView(central);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false);   // сортировку обрабатываем сами
    m_table->setWordWrap(false);
    m_table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->verticalHeader()->setVisible(false);

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionsClickable(true);
    header->setSortIndicatorShown(false);
    header->setSectionResizeMode(ExpenseModel::ColumnDate,     QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ExpenseModel::ColumnCategory, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ExpenseModel::ColumnAmount,   QHeaderView::ResizeToContents);
    header->setSectionResizeMode(ExpenseModel::ColumnComment,  QHeaderView::Stretch);

    mainLayout->addWidget(m_table);

    setCentralWidget(central);
    resize(900, 540);

    connect(m_table, &QTableView::doubleClicked,
            this, &MainWindow::onRowDoubleClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(header, &QHeaderView::sectionClicked,
            this, &MainWindow::onHeaderClicked);
    connect(m_categoryFilter, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onCategoryFilterChanged()));
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);
}

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::markModified()
{
    if (!m_modified) {
        m_modified = true;
        updateWindowTitle();
    }
}

int MainWindow::selectedSourceRow() const
{
    if (!m_table || !m_table->selectionModel())
        return -1;

    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return -1;

    const QModelIndex sourceIndex = m_proxy->mapToSource(rows.first());
    return sourceIndex.isValid() ? sourceIndex.row() : -1;
}

void MainWindow::selectRowBySource(int sourceRow)
{
    const QModelIndex proxyIndex = m_proxy->mapFromSource(
        m_model->index(sourceRow, ExpenseModel::ColumnDate));
    if (!proxyIndex.isValid())
        return;

    m_table->selectionModel()->setCurrentIndex(
        proxyIndex,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_table->scrollTo(proxyIndex);
}

void MainWindow::updateActions()
{
    const bool hasSelection = selectedSourceRow() >= 0;
    m_actionEdit->setEnabled(hasSelection);
    m_actionDelete->setEnabled(hasSelection);
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_currentPath.isEmpty()
                             ? QStringLiteral("Без имени")
                             : QFileInfo(m_currentPath).fileName();
    const QString mark = m_modified ? QStringLiteral(" *") : QString();

    setWindowTitle(QStringLiteral("Журнал расходов — %1%2").arg(name, mark));
}

void MainWindow::updateStatusBar()
{
    const int total = m_model->rowCount();
    const int shown = m_proxy->rowCount();

    qint64 sum = 0;
    for (int row = 0; row < shown; ++row) {
        const QModelIndex index = m_proxy->index(row, ExpenseModel::ColumnAmount);
        sum += index.data(ExpenseModel::KopeksRole).toLongLong();
    }

    m_statusLabel->setText(QStringLiteral("Всего записей: %1   |   Показано: %2   |   "
                                          "Сумма показанных: %3 руб.")
                               .arg(total)
                               .arg(shown)
                               .arg(ExpenseStorage::formatAmount(sum)));
}

bool MainWindow::confirmSaveChanges()
{
    if (!m_modified)
        return true;

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Несохранённые изменения"));
    box.setText(QStringLiteral("Документ содержит несохранённые изменения."));
    box.setInformativeText(QStringLiteral("Сохранить их перед продолжением?"));

    QPushButton *saveButton    = box.addButton(QStringLiteral("Сохранить"),
                                            QMessageBox::AcceptRole);
    QPushButton *discardButton = box.addButton(QStringLiteral("Не сохранять"),
                                               QMessageBox::DestructiveRole);
    box.addButton(QStringLiteral("Отмена"), QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);
    box.exec();

    if (box.clickedButton() == saveButton)
        return saveDocument();
    if (box.clickedButton() == discardButton)
        return true;
    return false;
}

void MainWindow::onNewDocument()
{
    if (!confirmSaveChanges())
        return;

    m_model->setExpenses(QVector<Expense>());
    m_currentPath.clear();
    m_modified = false;

    updateWindowTitle();
    updateStatusBar();
    updateActions();
}

void MainWindow::onOpenDocument()
{
    if (!confirmSaveChanges())
        return;

    const QString startDir = m_currentPath.isEmpty()
                                 ? QDir::homePath()
                                 : QFileInfo(m_currentPath).absolutePath();

    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Открыть файл расходов"),
        startDir,
        QStringLiteral("Файлы JSON (*.json);;Все файлы (*)"));

    if (path.isEmpty())
        return;

    openFile(path);
}

void MainWindow::onSaveDocument()
{
    saveDocument();
}

void MainWindow::onSaveDocumentAs()
{
    saveDocumentAs();
}

bool MainWindow::saveDocument()
{
    if (m_currentPath.isEmpty())
        return saveDocumentAs();
    return saveToFile(m_currentPath);
}

bool MainWindow::saveDocumentAs()
{
    QString suggested = m_currentPath;
    if (suggested.isEmpty())
        suggested = QDir::homePath() + QDir::separator() + QStringLiteral("expenses.json");

    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Сохранить как"),
        suggested,
        QStringLiteral("Файлы JSON (*.json);;Все файлы (*)"));

    if (path.isEmpty())
        return false;

    return saveToFile(path);
}

bool MainWindow::saveToFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this,
                              QStringLiteral("Ошибка сохранения"),
                              QStringLiteral("Не удалось открыть файл для записи:\n%1\n\n%2")
                                  .arg(path, file.errorString()));
        return false;
    }

    const QByteArray data = ExpenseStorage::saveDocument(m_model->expenses());
    const qint64 written = file.write(data);
    if (written != data.size()) {
        QMessageBox::critical(this,
                              QStringLiteral("Ошибка сохранения"),
                              QStringLiteral("Не удалось записать данные в файл:\n%1\n\n%2")
                                  .arg(path, file.errorString()));
        file.close();
        return false;
    }
    file.close();

    m_currentPath = path;
    m_modified = false;

    QSettings().setValue(QLatin1String(kLastFileKey), path);

    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("Файл сохранён: %1").arg(path), 4000);
    return true;
}

bool MainWindow::openFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this,
                              QStringLiteral("Ошибка открытия"),
                              QStringLiteral("Не удалось прочитать файл:\n%1\n\n%2")
                                  .arg(path, file.errorString()));
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QVector<Expense> expenses;
    QString error;
    if (!ExpenseStorage::loadDocument(data, expenses, &error)) {
        QMessageBox::critical(this,
                              QStringLiteral("Ошибка открытия"),
                              QStringLiteral("Файл не удалось загрузить:\n%1\n\n%2").arg(path, error));
        return false;
    }

    m_model->setExpenses(expenses);
    m_currentPath = path;
    m_modified = false;

    QSettings().setValue(QLatin1String(kLastFileKey), path);

    updateWindowTitle();
    updateStatusBar();
    updateActions();
    return true;
}

void MainWindow::offerToOpenLastFile()
{
    QSettings settings;
    const QString path = settings.value(QLatin1String(kLastFileKey)).toString();
    if (path.isEmpty())
        return;

    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this,
                             QStringLiteral("Файл не найден"),
                             QStringLiteral("Последний файл больше не существует:\n%1\n\n"
                                            "Будет создан пустой документ «Без имени».").arg(path));
        return;
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QStringLiteral("Открыть последний файл"));
    box.setText(QStringLiteral("Открыть последний файл?"));
    box.setInformativeText(path);

    QPushButton *openButton = box.addButton(QStringLiteral("Открыть"),
                                            QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("Отмена"), QMessageBox::RejectRole);
    box.setDefaultButton(openButton);
    box.exec();

    if (box.clickedButton() != openButton)
        return;

    openFile(path);
}

void MainWindow::onAddExpense()
{
    ExpenseDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    Expense expense = dialog.expense();
    expense.id = ExpenseStorage::generateId(m_model->expenses());

    m_model->addExpense(expense);

    markModified();
    updateStatusBar();
    updateActions();
    selectRowBySource(m_model->rowCount() - 1);
}

void MainWindow::onEditExpense()
{
    const int row = selectedSourceRow();
    if (row < 0)
        return;

    const Expense original = m_model->expenseAt(row);

    ExpenseDialog dialog(original, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    Expense updated = dialog.expense();
    updated.id        = original.id;
    updated.createdAt = original.createdAt;

    m_model->updateExpense(row, updated);

    markModified();
    updateStatusBar();
    updateActions();
}

void MainWindow::onDeleteExpense()
{
    const int row = selectedSourceRow();
    if (row < 0)
        return;

    const Expense expense = m_model->expenseAt(row);

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(QStringLiteral("Удаление записи"));
    box.setText(QStringLiteral("Удалить выбранную запись?"));
    box.setInformativeText(QStringLiteral("%1, %2, %3 руб.%4")
                               .arg(expense.spentOn.toString(QStringLiteral("dd.MM.yyyy")),
                                    ExpenseCategory::displayName(expense.category),
                                    ExpenseStorage::formatAmount(expense.amountKopeks),
                                    expense.comment.isEmpty() ? QString()
                                                              : QStringLiteral("\n") + expense.comment));

    QPushButton *deleteButton = box.addButton(QStringLiteral("Удалить"),
                                              QMessageBox::DestructiveRole);
    box.addButton(QStringLiteral("Отмена"), QMessageBox::RejectRole);
    box.setDefaultButton(deleteButton);
    box.exec();

    if (box.clickedButton() != deleteButton)
        return;

    m_model->removeExpense(row);

    markModified();
    updateStatusBar();
    updateActions();
}

void MainWindow::onRowDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    onEditExpense();
}

void MainWindow::onSelectionChanged()
{
    updateActions();
}

void MainWindow::onCategoryFilterChanged()
{
    m_proxy->setCategoryFilter(m_categoryFilter->currentData().toString());
    updateStatusBar();
    updateActions();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_proxy->setSearchText(text);
    updateStatusBar();
    updateActions();
}

void MainWindow::onHeaderClicked(int column)
{
    if (column != ExpenseModel::ColumnDate
        && column != ExpenseModel::ColumnCategory
        && column != ExpenseModel::ColumnAmount) {
        return;   // по комментарию сортировки нет
    }

    Qt::SortOrder order = Qt::AscendingOrder;
    if (m_sortColumn == column && m_sortOrder == Qt::AscendingOrder)
        order = Qt::DescendingOrder;

    m_sortColumn = column;
    m_sortOrder  = order;

    m_proxy->sort(column, order);

    QHeaderView *header = m_table->horizontalHeader();
    header->setSortIndicatorShown(true);
    header->setSortIndicator(column, order);

    updateStatusBar();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmSaveChanges())
        event->accept();
    else
        event->ignore();
}
