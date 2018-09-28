TEMPLATE = app
TARGET = yampt
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_LFLAGS = -s -static

unix {
    LIBS += \
        -lboost_system \
        -lboost_filesystem
}

win32 {
    LIBS += \
        /usr/i686-w64-mingw32/lib/libboost_system-mt.a \
        /usr/i686-w64-mingw32/lib/libboost_filesystem-mt.a
}

SOURCES += \
    config.cpp \
    dictcreator.cpp \
    dictmerger.cpp \
    dictreader.cpp \
    esmconverter.cpp \
    esmreader.cpp \
    esmtools.cpp \
    main.cpp \
    scriptparser.cpp \
    userinterface.cpp

HEADERS += \
    config.hpp \
    dictcreator.hpp \
    dictmerger.hpp \
    dictreader.hpp \
    esmconverter.hpp \
    esmreader.hpp \
    esmtools.hpp \
    scriptparser.hpp \
    userinterface.hpp
