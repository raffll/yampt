#include "converter.hpp"
#include "ui_converter.h"

//----------------------------------------------------------
Converter::Converter(Config &config, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Converter),
    config(&config)
{
    ui->setupUi(this);
    ui->textEdit->setReadOnly(true);
    ui->lineEdit_input->setText(this->config->getInputPath());
    ui->lineEdit_input->setCursorPosition(QTextCursor::Start);
    ui->lineEdit_output->setText(this->config->getOutputPath());
    ui->lineEdit_output->setCursorPosition(QTextCursor::Start);
    resetFileList();
}

//----------------------------------------------------------
Converter::~Converter()
{
    delete ui;
}

//----------------------------------------------------------
void Converter::resetFileList()
{
    file_path_list.clear();
    QDirIterator it(this->config->getInputPath(), QStringList() << "*.esm" << "*.esp", QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        file_path_list << it.next();
    }
    updateFileListOutput();
}

//----------------------------------------------------------
void Converter::updateFileListOutput()
{
    QStringListModel *model = new QStringListModel();
    model->setStringList(file_path_list);
    ui->listView->setModel(model);
}

//----------------------------------------------------------
void Converter::updateLogStandardOutput()
{
    ui->textEdit->moveCursor(QTextCursor::End);
    ui->textEdit->textCursor().insertText(process->readAll());
    ui->textEdit->moveCursor(QTextCursor::End);
}

//----------------------------------------------------------
void Converter::on_pushButton_add_clicked()
{
    file_path_list += QFileDialog::getOpenFileNames(this,
                                                    "Select one or more esm/esp files to open",
                                                    QCoreApplication::applicationDirPath(),
                                                    "esm/esp files (*.esm *.esp)");
    file_path_list.removeDuplicates();
    updateFileListOutput();
}

//----------------------------------------------------------
void Converter::on_pushButton_reset_clicked()
{
    resetFileList();
}

//----------------------------------------------------------
void Converter::on_pushButton_run_clicked()
{
    process = new QProcess();
    if(process)
    {
        process->setEnvironment(QProcess::systemEnvironment());
        process->setProcessChannelMode(QProcess::MergedChannels);
        process->start(config->getExecPath(),
                       QStringList() << "--convert"
                       << "-f" << file_path_list
                       << "-d" << config->getDictPath()
                       << "-o" << config->getOutputPath());
        process->waitForStarted();
        QObject::connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(updateLogStandardOutput()));
    }
}
