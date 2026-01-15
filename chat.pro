QT += core gui network sql widgets

CONFIG += c++17

TARGET = chat
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    logindialog.cpp \
    chatwindow.cpp \
    contactlistwidget.cpp \
    databasemanager.cpp \
    networkmanager.cpp \
    messagemodel.cpp \
    heartbeatthread.cpp

HEADERS += \
    mainwindow.h \
    logindialog.h \
    chatwindow.h \
    contactlistwidget.h \
    databasemanager.h \
    networkmanager.h \
    messagemodel.h \
    heartbeatthread.h

FORMS += \
    mainwindow.ui \
    logindialog.ui \
    chatwindow.ui

RESOURCES += \
    resources.qrc
