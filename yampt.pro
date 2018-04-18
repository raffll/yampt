TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_LFLAGS = -s -static

SOURCES += \
    src/config.cpp \
    src/dictcreator.cpp \
    src/dictmerger.cpp \
    src/dictreader.cpp \
    src/esmconverter.cpp \
    src/esmreader.cpp \
    src/esmtools.cpp \
    src/main.cpp \
    src/scriptparser.cpp \
    src/userinterface.cpp


HEADERS += \
    src/config.hpp \
    src/dictcreator.hpp \
    src/dictmerger.hpp \
    src/dictreader.hpp \
    src/esmconverter.hpp \
    src/esmreader.hpp \
    src/esmtools.hpp \
    src/scriptparser.hpp \
    src/userinterface.hpp

DISTFILES += \
    CHANGELOG.md \
    README.md \
    LICENSE.txt
