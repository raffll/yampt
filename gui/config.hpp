#ifndef CONFIG_H
#define CONFIG_H

#include <QCoreApplication>
#include <QString>

class Config
{
public:
    QString getExecPath() { return exec_path; }
    QString getDictPath() { return dict_path; }
    QString getInputPath() { return input_path; }
    QString getOutputPath() { return output_path; }

    void setExecPath(QString exec_path);
    void setDictPath(QString dict_path);
    void setInputPath(QString input_path);
    void setOutputPath(QString output_path);

    Config();
    ~Config();

private:
    QString exec_path;
    QString dict_path;
    QString input_path;
    QString output_path;
};

#endif // CONFIG_H
