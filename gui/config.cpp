#include "config.hpp"

//----------------------------------------------------------
Config::Config() :
    exec_path("/home/braindead/Projekty/C++/build-yampt-Desktop-Release/yampt"),
    dict_path("/home/braindead/Dokumenty/Dropbox/Morrowind/dict_base/ENtoPL+PatchPLv2.0.xml"),
    input_path("/home/braindead/.local/share/openmw"),
    output_path("/home/braindead/Dokumenty/Dropbox/Morrowind/output")
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
