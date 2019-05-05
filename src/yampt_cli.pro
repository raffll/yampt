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
    INCLUDEPATH += "C:\boost\include\boost-1_70"
    LIBS += -L"C:\boost\lib"
    LIBS += \
        "C:\boost\lib\libboost_system-mgw73-mt-x64-1_70.a" \
        "C:\boost\lib\libboost_filesystem-mgw73-mt-x64-1_70.a"
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
