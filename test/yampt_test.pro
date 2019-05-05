TEMPLATE = app
TARGET = yampt_test
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

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
    ../src/esmreader.cpp \
    ../src/config.cpp \
    ../src/dictcreator.cpp \
    ../src/dictreader.cpp \
    ../src/dictmerger.cpp \
    ../src/scriptparser.cpp \
    test_main.cpp \
    test_tools.cpp \
    test_esmreader.cpp \
    test_dictcreator.cpp \
    test_dictreader.cpp \
    test_scriptparser.cpp

HEADERS += \
    catch.hpp \
    ../src/esmreader.hpp \
    ../src/config.hpp \
    ../src/dictcreator.hpp \
    ../src/dictreader.hpp \
    ../src/dictmerger.hpp \
    ../src/scriptparser.hpp
