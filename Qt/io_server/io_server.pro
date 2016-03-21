#-------------------------------------------------
#
# Project created by QtCreator 2015-12-31T06:56:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = io_server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pump_report.cpp \
    user_variables.cpp \
    config_form.cpp

HEADERS  += mainwindow.h \
    pump_report.h \
    user_variables.h \
    config_form.h

FORMS    += mainwindow.ui \
    config_form.ui
