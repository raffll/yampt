#include "includes.hpp"
#include "tools.hpp"
#include "userinterface.hpp"

int main(int argc, char * argv[])
{
    try
    {
        std::vector<std::string> arg;
        for (int i = 0; i < argc; i++)
        {
            arg.push_back(argv[i]);
        }
        UserInterface ui(arg);
    }
    catch (...)
    {
        Tools::addLog("Unknown error!\r\n");
    }

    Tools::writeText(Tools::getLog(), "yampt.log");
}
