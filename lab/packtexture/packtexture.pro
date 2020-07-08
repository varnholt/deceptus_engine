QT += core gui
CONFIG += c++11 console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = packtexture
TEMPLATE = app

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    packtexture.cpp \
    texturelabel.cpp \
    quad.cpp

HEADERS += \
        mainwindow.h \
    packtexture.h \
    texturelabel.h \
    quad.h

FORMS += \
        mainwindow.ui
