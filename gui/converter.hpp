#ifndef CONVERTER_H
#define CONVERTER_H

#include <QDebug>
#include <QWidget>
#include <QProcess>
#include <QFileDialog>
#include <QStringList>
#include <QStringListModel>
#include <QDirIterator>

#include "config.hpp"

namespace Ui {
class Converter;
}

class Converter : public QWidget
{
    Q_OBJECT

public:
    explicit Converter(Config &config, QWidget *parent = nullptr);
    ~Converter();

private slots:
    void resetFileList();
    void updateFileListOutput();
    void updateLogStandardOutput();
    void on_pushButton_add_clicked();
    void on_pushButton_reset_clicked();
    void on_pushButton_run_clicked();

private:
    Ui::Converter *ui;
    Config *config;
    QProcess *process;

    QStringList file_path_list;
};

#endif // CONVERTER_H
