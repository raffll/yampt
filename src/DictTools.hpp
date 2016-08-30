#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include "Config.hpp"

using namespace std;

class DictTools
{
public:
	void writeDict(const array<map<string, string>, 11> &dict, string name);
	int getSize(const array<map<string, string>, 11> &dict);

	DictTools();

};

#endif
