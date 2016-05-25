#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <vector>

#include "tools.hpp"

using namespace std;

class esmtools : public tools
{
public:
	void readFile(const char* path);
	void resetRec();
	void setNextRec();
	void setRecContent();
	void setPriSubRec(const char* id);
	void setSecSubRec(const char* id);

	string dialType();
	bool loopCheck();

	string getRecId() { return rec_id; }
	string getPriId() { return pri_id; }
	string getSecId() { return sec_id; }
	string getPriText() { return pri_text; }
	string getSecText() { return sec_text; }
	string getTmpLine(int i) { return tmp_text[i]; }
	size_t getTmpSize() { return tmp_text.size(); }
	bool getStatus() { return status; }

	esmtools() : status(0) {}

private:
	void printStatus();

	int status;
	string file_path;
	string file_content;

	size_t rec_beg;
	size_t rec_end;
	size_t rec_size;
	string rec_id;
	string rec_content;

	size_t pri_pos;
	size_t pri_size;
	string pri_id;
	string pri_text;

	size_t sec_pos;
	size_t sec_size;
	string sec_id;
	string sec_text;

	vector<string> tmp_text;
};

#endif
