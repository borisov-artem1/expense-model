QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = ExpenseJournal
TEMPLATE = app

CONFIG  += c++11

msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    expense.cpp \
    expensemodel.cpp \
    expensefilterproxymodel.cpp \
    expensedialog.cpp \
    mainwindow.cpp

HEADERS += \
    expense.h \
    expensemodel.h \
    expensefilterproxymodel.h \
    expensedialog.h \
    mainwindow.h

DISTFILES += \
    sample_expenses_Borisov.json

TRANSLATIONS += \
    test_project_en_150.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
