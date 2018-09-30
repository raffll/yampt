TEMPLATE = app
TARGET = yampt_gui
QT += core gui widgets
#CONFIG += static
QMAKE_LFLAGS += -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

SOURCES += \
    config.cpp \
    converter.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    config.hpp \
    converter.hpp \
    mainwindow.hpp

FORMS += \
    converter.ui \
    mainwindow.ui
