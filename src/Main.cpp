#include "UserInterface.hpp"

using namespace std;

int main(int argc, char *argv[])
{
	vector<string> arg;
	for(int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	UserInterface ui(arg);
}
