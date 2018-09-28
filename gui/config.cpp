#include "config.hpp"

//----------------------------------------------------------
Config::Config() :
    exec_path("yampt"),
    dict_path("dict_base/*.xml"),
    input_path("input"),
    output_path("output")
{

}

//----------------------------------------------------------
Config::~Config()
{

}

//----------------------------------------------------------
void Config::setExecPath(QString exec_path)
{
    this->exec_path = exec_path;
}

//----------------------------------------------------------
void Config::setDictPath(QString dict_path)
{
    this->dict_path = dict_path;
}

//----------------------------------------------------------
void Config::setInputPath(QString input_path)
{
    this->exec_path = input_path;
}

//----------------------------------------------------------
void Config::setOutputPath(QString output_path)
{
    this->dict_path = output_path;
}
