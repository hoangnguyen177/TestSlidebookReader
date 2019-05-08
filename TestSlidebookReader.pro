QT += core
QT -= gui

QMAKE_CXXFLAGS += -std=c++0x -Wcpp
TARGET = TestSlidebookReader

SUBDIRS += lib
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib/ -lrdkafka++ -lSlideBook6Reader -ljsoncpp -lfmt -ltiff

HEADERS +=

