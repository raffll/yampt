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

class esmtools
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
	string getRecContent() { return rec_content; }

	size_t getPriPos() { return pri_pos; }
	size_t getPriSize() { return pri_size; }
	string getPriId() { return pri_id; }
	string getPriText() { return pri_text; }

	string getSecId() { return sec_id; }
	string getSecText() { return sec_text; }
	size_t getSecPos() { return sec_pos; }
	size_t getSecSize() { return sec_size; }

	string getTmpLine(int i) { return tmp_text[i]; }
	size_t getTmpSize() { return tmp_text.size(); }

	bool getStatus() { return status; }

	esmtools() : status(0) {}

private:
	enum st{not_loaded, loaded, error};
	void setStatus(st e);

	unsigned int byteToInt(const string &str);
	void cutNullCharFromText(string &str);

	int status;
	string name;
	string content;

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
