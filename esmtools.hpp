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
	void readEsm(string path);
	void resetRec();
	void setNextRec();
	void setRecContent();
	void setPriSubRec(string id);
	void setSecSubRec(string id);
	void setRnamColl();
	void setScptColl(bool key_chooser = 0);
	void setEsmContent(string c) { esm_content = c; }

	string dialType();
	bool loopCheck();

	bool getEsmStatus() { return esm_status; }
	string getEsmName() { return esm_name; }
	string getEsmPrefix() { return esm_prefix; }
	string getEsmSuffix() { return esm_suffix; }
	string getEsmContent() { return esm_content; }

	size_t getRecSize() { return rec_size; }
	string getRecId() { return rec_id; }
	string getRecContent() { return rec_content; }

	size_t getPriPos() { return pri_pos; }
	size_t getPriSize() { return pri_size; }
	string getPriId() { return pri_id; }
	string getPriText() { return pri_text; }

	size_t getSecPos() { return sec_pos; }
	size_t getSecSize() { return sec_size; }
	string getSecId() { return sec_id; }
	string getSecText() { return sec_text; }

	string getRnamText(int i) { return text_coll[i].second; }
	size_t getRnamPos(int i) { return text_coll[i].first; }
	string getScptText(int i) { return text_coll[i].second; }
	string getScptPos(int i) { return to_string(text_coll[i].first); }
	size_t getCollSize() { return text_coll.size(); }

	esmtools() {}

private:
	void setEsmStatus(bool st);
	void setEsmName(string path);

	unsigned int byteToInt(const string &str);
	void cutNullChar(string &str);

	bool esm_status = {};
	string esm_name;
	string esm_prefix;
	string esm_suffix;
	string esm_content;

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

	vector<pair<size_t, string>> text_coll;
};

#endif
