TEMPLATE = app
TARGET = svg-export-tests
QT += core gui widgets testlib
CONFIG += console testcase release no_batch
CONFIG -= app_bundle debug debug_and_release
OBJECTS_DIR = obj
MOC_DIR = moc
INCLUDEPATH += .. ../Depends/heif/include
DEFINES += _CRT_SECURE_NO_WARNINGS
SOURCES += svg_export_tests.cpp \
    ../ImageConverterEngine.cpp \
    ../ImageConverterWorker.cpp \
    ../ImageConverterPanel.cpp \
    ../HeicDecoder.cpp \
    ../HeicEncoder.cpp
HEADERS += ../ImageConverterEngine.h \
    ../ImageConverterWorker.h \
    ../ImageConverterPanel.h \
    ../HeicDecoder.h \
    ../HeicEncoder.h
win32:LIBS += -L$$PWD/../Depends/heif/lib/x64 -lheif -lpsapi
