#include "main_window.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(240, 240, 240));
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, QColor(240, 240, 240));
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::Highlight, QColor(70, 130, 200));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(palette);

    main_window_t window;
    window.show();

    return app.exec();
}
