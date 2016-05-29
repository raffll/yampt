#ifndef CREATOR_HPP
#define CREATOR_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <array>
#include <map>

#include "tools.hpp"
#include "esmtools.hpp"

using namespace std;

class creator : public tools
{
public:
	void makeDict();
	void writeDict();

	creator() {}
	creator(const char* path_base);
	creator(const char* path_base, const char* path_extd);

protected:
	esmtools base;
	int counter;

private:
	void printLog(int i);

	void makeDictCell();
	void makeDictGmst();
	void makeDictFnam();
	void makeDictDesc();
	void makeDictBook();
	void makeDictFact();
	void makeDictIndx();
	void makeDictDial();
	void makeDictInfo();
	void makeDictScpt();

	esmtools extd;
	esmtools *esm_ptr;

	array<dict_t, 10> dict_created;
};

#endif
