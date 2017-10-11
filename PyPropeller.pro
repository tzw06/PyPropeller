#-------------------------------------------------
#
# Project created by QtCreator 2017-08-09T14:23:04
#
#-------------------------------------------------

QT       += core gui widgets xml opengl

TARGET = PyPropeller
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG(debug, debug|release) {
        DEFINES += _DEBUG
        DESTDIR = bin/debug
        OBJECTS_DIR = debug
        MOC_DIR = debug
        RCC_DIR = debug
}
else {
        DESTDIR = bin/release
        OBJECTS_DIR = release
        MOC_DIR = release
        RCC_DIR = release
}

SRC = ../gui/PyPropeller

win32 {
    INCLUDEPATH += "E:\SDK\OpenSceneGraph\include"
    LIBS += -L"E:\SDK\OpenSceneGraph\lib" -losg$${SUFFIX} -losgDB$${SUFFIX} -losgGA$${SUFFIX} -losgText$${SUFFIX} -losgUtil$${SUFFIX} -losgViewer$${SUFFIX} -lOpenThreads$${SUFFIX}
}
mac {
    QMAKE_LFLAGS += -F/usr/local/lib
    INCLUDEPATH += /usr/local/include
    LIBS += -framework osg
    LIBS += -framework osgUtil
    LIBS += -framework osgText
    LIBS += -framework osgDB
    LIBS += -framework osgGA
    LIBS += -framework osgSim
    LIBS += -framework osgViewer
    LIBS += -framework osgQt
}

SOURCES += \
        $${SRC}\inspector.cpp \
        $${SRC}\main.cpp \
        $${SRC}\mainwindow.cpp \
        $${SRC}\methodview.cpp \
        $${SRC}\adapter.cpp \
        $${SRC}\modelview.cpp \
        $${SRC}\manipulator.cpp \
        $${SRC}\messenger.cpp \
        $${SRC}\console.cpp

HEADERS += \
        $${SRC}\inspector.h \
        $${SRC}\mainwindow.h \
        $${SRC}\methodview.h \
        $${SRC}\adapter.h \
        $${SRC}\modelview.h \
        $${SRC}\manipulator.h \
        $${SRC}\messenger.h \
        $${SRC}\console.h

RESOURCES += $${SRC}\mainframe.qrc

OTHER_FILES+= propeller.rc
RC_FILE = propeller.rc


