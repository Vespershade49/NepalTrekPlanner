QT       += core testlib
QT       -= gui

CONFIG   += console testcase c++17
CONFIG   -= app_bundle

TEMPLATE = app
TARGET   = tests

INCLUDEPATH += ..

SOURCES += test_routeoptimizer.cpp
