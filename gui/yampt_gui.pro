TEMPLATE = app
TARGET = yampt_gui

unix {
    QT += core gui widgets
}

win32 {
    QMAKE_LFLAGS += -s -static
    LIBS += \
        /usr/i686-w64-mingw32/lib/libQt5Widgets.a \
        /usr/i686-w64-mingw32/lib/libQt5Gui.a \
        /usr/i686-w64-mingw32/lib/libQt5Core.a
}

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
