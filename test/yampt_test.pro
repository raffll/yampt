TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

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
    ../src/esmreader.cpp \
    ../src/config.cpp \
    test_main.cpp \
    test_tools.cpp \
    test_esmreader.cpp

HEADERS += \
    ../src/esmreader.hpp \
    ../src/config.hpp