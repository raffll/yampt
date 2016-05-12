#include "tools.hpp"

using namespace std;

//----------------------------------------------------------
void tools::setFileName(string &file_name)
{
	string slash = "\\/";
	file_name = file_name.substr(file_name.find_last_of(slash) + 1);
}
