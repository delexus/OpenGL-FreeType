
QT += core gui

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets

TARGET = OpenGL-FreeType

LIBS += -lfreetype

SOURCES += main.cpp openglwindow.cpp
HEADERS += openglwindow.h

RESOURCES += \
    shaders.qrc

OTHER_FILES +=
