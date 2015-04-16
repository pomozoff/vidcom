#-------------------------------------------------
#
# Project created by QtCreator 2015-04-06T22:29:47
#
#-------------------------------------------------

QT       += core gui concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++11

TARGET    = vidcom
TEMPLATE  = app


SOURCES  += main.cpp\
            mainwindow.cpp \
            videocontainer.cpp

HEADERS  += mainwindow.h \
            videocontainer.h

FORMS    += mainwindow.ui

INCLUDEPATH += $$PWD/../ffmpeg/include

LIBS        += -L$$PWD/../ffmpeg/lib -lstdc++ -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
