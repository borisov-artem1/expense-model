#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("BorisovTestTask"));
    QCoreApplication::setApplicationName(QStringLiteral("ExpenseJournal"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));

    MainWindow window;
    window.show();

    return app.exec();
}
