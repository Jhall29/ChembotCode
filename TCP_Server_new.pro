#-------------------------------------------------
#
# Project created by QtCreator 2020-12-07T00:16:19
#
#-------------------------------------------------

QT       += core gui serialport\  #for USB connect to VEX
            network \   #for ethernet connect to GUI

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TCP_Server_new
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
   ../Desktop/qt-everywhere-src-5.15.1/qtserialport/dist/changes-5.15.1
