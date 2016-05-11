#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include <cstdlib>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

class esmtools
{
private:
	string file_content;
	bool is_loaded;
	size_t rec_beg, rec_end, pri_pos, sec_pos, rec_size, pri_size, sec_size;
	string rec_id, pri_id, sec_id, rec_content, pri_text, sec_text;
	map<int, string> type_coll = {{0, "T"},
								  {1, "V"},
								  {2, "G"},
								  {3, "P"},
								  {4, "J"}};

	void cutText(string *str);
	unsigned int byteToInt(size_t pos, string *str, bool q = 0);

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

	string dialType();
	bool loopCheck();

	esmtools();
	esmtools(const char* f);
};

#endif
