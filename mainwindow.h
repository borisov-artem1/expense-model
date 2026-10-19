#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "expense.h"

class QAction;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QTableView;

class ExpenseFilterProxyModel;
class ExpenseModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewDocument();
    void onOpenDocument();
    void onSaveDocument();
    void onSaveDocumentAs();

    void onAddExpense();
    void onEditExpense();
    void onDeleteExpense();
    void onRowDoubleClicked(const QModelIndex &index);
    void onSelectionChanged();
    void onHeaderClicked(int column);

    void onCategoryFilterChanged();
    void onSearchTextChanged(const QString &text);

    void updateStatusBar();
    void updateWindowTitle();
    void updateActions();
    void offerToOpenLastFile();

private:
    void createActions();
    void createMenus();
    void createCentralWidget();
    void createStatusBar();

    bool confirmSaveChanges();
    bool saveDocument();
    bool saveDocumentAs();
    bool saveToFile(const QString &path);
    bool openFile(const QString &path);

    void markModified();
    int  selectedSourceRow() const;
    void selectRowBySource(int sourceRow);

    ExpenseModel            *m_model;
    ExpenseFilterProxyModel *m_proxy;

    QTableView *m_table;
    QComboBox  *m_categoryFilter;
    QLineEdit  *m_searchEdit;
    QLabel     *m_statusLabel;

    QAction *m_actionNew;
    QAction *m_actionOpen;
    QAction *m_actionSave;
    QAction *m_actionSaveAs;
    QAction *m_actionExit;
    QAction *m_actionAdd;
    QAction *m_actionEdit;
    QAction *m_actionDelete;

    QString m_currentPath;
    bool    m_modified;

    int            m_sortColumn;
    Qt::SortOrder  m_sortOrder;
};

#endif // MAINWINDOW_H
