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
    src/UserInterface.cpp \
    src/config.cpp \
    src/dictcreator.cpp \
    src/dictmerger.cpp \
    src/dictreader.cpp \
    src/esmconverter.cpp \
    src/esmreader.cpp \
    src/esmtools.cpp \
    src/main.cpp \
    src/scriptparser.cpp \
    src/scriptparser_test.cpp \
    src/userinterface.cpp


HEADERS += \
    src/Config.hpp \
    src/DictCreator.hpp \
    src/DictMerger.hpp \
    src/DictReader.hpp \
    src/EsmConverter.hpp \
    src/EsmReader.hpp \
    src/UserInterface.hpp \
    src/config.hpp \
    src/dictcreator.hpp \
    src/dictmerger.hpp \
    src/dictreader.hpp \
    src/esmconverter.hpp \
    src/esmreader.hpp \
    src/esmtools.hpp \
    src/scriptparser.hpp \
    src/scriptparser_test.hpp \
    src/userinterface.hpp

DISTFILES += \
    CHANGELOG.md \
    README.md \
    LICENSE.txt \
    CHANGELOG.md \
    README.md \
    LICENSE.txt

