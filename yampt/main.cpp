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
    catch (const std::exception & e)
    {
        tools_t::addLog("Error: " + std::string(e.what()) + "\r\n");
    }
    catch (...)
    {
        tools_t::addLog("UNKNOWN error!\r\n");
    }

    tools_t::writeText(tools_t::getLog(), "yampt.log");
    return tools_t::hasError() ? 1 : 0;
}
