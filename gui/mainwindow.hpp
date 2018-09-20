#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "converter.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Config config;
};

#endif // MAINWINDOW_H
