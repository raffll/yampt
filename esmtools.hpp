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
	vector<string> temp_text;

	void resetRec();
	void setNextRec();
	void setRecContent();
	void setPriSubRec(const char* id);
	void setSecSubRec(const char* id);

	string getRecId();
	string getPriId();
	string getSecId();
	string getPriText();
	string getSecText();

	bool getStatus();
	void printStatus();

	string dialType();
	bool loopCheck();

	esmtools();
	esmtools(const char* path);

private:
	string file_name;
	string file_content;
	bool is_loaded;
	size_t rec_beg, rec_end, pri_pos, sec_pos, rec_size, pri_size, sec_size;
	string rec_id, pri_id, sec_id, rec_content, pri_text, sec_text;

	void cutText(string *str);
	unsigned int byteToInt(size_t pos, string *str, bool q = 0);
};

#endif
