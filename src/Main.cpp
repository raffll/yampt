#include "UserInterface.hpp"

//----------------------------------------------------------
int main(int argc, char *argv[])
{
    std::vector<std::string> arg;
    for(int i = 0; i < argc; i++)
    {
        arg.push_back(argv[i]);
    }
    UserInterface ui(arg);
}
