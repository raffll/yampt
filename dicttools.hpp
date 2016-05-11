#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <array>

#include "tools.hpp"

using namespace std;

class dicttools : public tools
{
private:

public:
	bool is_loaded;
	vector<string> dict;
	bool getStatus();
	void printStatus();
	void printDict();

	dicttools();
	dicttools(const char* path, int i);
};

#endif
