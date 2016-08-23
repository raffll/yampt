#ifndef RECTOOLS_HPP
#define RECTOOLS_HPP

#include "EsmTools.hpp"

using namespace std;

class RecTools : public EsmTools
{
public:
	void setRec(size_t i);
	void setRecContent(string content) { *rec = content; }
	string getRecContent() { return *rec; }

	void setPri(string id);
	void setPriColl(string id);
	void setPriINDX();
	void setSec(string id);
	void setSecColl(string id);
	void setSecScptColl(string id);
	void setSecMessageColl(string id);
	void setSecDialType(string id);

	string getRecId() { return rec_id; }
	size_t getRecSize() { return rec_size; }

	vector<tuple<size_t, size_t, string>> getPriColl() { return pri_coll; }
	string getPriId() { return pri_id; }
	size_t getPriPos(size_t i = 0) { return get<0>(pri_coll[i]); }
	size_t getPriSize(size_t i = 0) { return get<1>(pri_coll[i]); }
	string getPriText(size_t i = 0) { return get<2>(pri_coll[i]); }

	vector<tuple<size_t, size_t, string>> getSecColl() { return sec_coll; }
	string getSecId() { return sec_id; }
	size_t getSecPos(size_t i = 0) { return get<0>(sec_coll[i]); }
	size_t getSecSize(size_t i = 0) { return get<1>(sec_coll[i]); }
	string getSecText(size_t i = 0) { return get<2>(sec_coll[i]); }

	vector<tuple<string, string, string, size_t>> getScptColl() { return scpt_coll; }
	string getScptLineType(size_t i) { return get<0>(scpt_coll[i]); }
	string getScptLine(size_t i) { return get<1>(scpt_coll[i]); }
	string getScptText(size_t i) { return get<2>(scpt_coll[i]); }
	size_t getScptTextPos(size_t i) { return get<3>(scpt_coll[i]); }

	string getDialType() { return dial_type; }
	void printBinary(string content);

	RecTools();

private:
	void eraseNullChars(string &str);
	void replaceBrokenChars(string &str);
	string eraseCarriageReturnChar(string &str);
	void extractText(const string &line, string &text, size_t &pos);

	string *rec;

	size_t rec_size;
	string rec_id;

	string pri_id;
	vector<tuple<size_t, size_t, string>> pri_coll;

	string sec_id;
	vector<tuple<size_t, size_t, string>> sec_coll;

	vector<tuple<string, string, string, size_t>> scpt_coll;

	string dial_type;
};

#endif
