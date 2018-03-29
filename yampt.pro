TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_LFLAGS = -s -static

SOURCES += \
    src/Config.cpp \
    src/DictCreator.cpp \
    src/DictMerger.cpp \
    src/DictReader.cpp \
    src/EsmConverter.cpp \
    src/EsmReader.cpp \
    src/Main.cpp \
    src/UserInterface.cpp


HEADERS += \
    src/Config.hpp \
    src/DictCreator.hpp \
    src/DictMerger.hpp \
    src/DictReader.hpp \
    src/EsmConverter.hpp \
    src/EsmReader.hpp \
    src/UserInterface.hpp

DISTFILES += \
    CHANGELOG.md \
    README.md \
    LICENSE.txt

