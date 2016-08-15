#ifndef RECTOOLS_HPP
#define RECTOOLS_HPP

#include "EsmTools.hpp"

using namespace std;

class RecTools : public EsmTools
{
public:
	void setRec(size_t num);

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

	size_t getPriCollSize() { return pri_coll.size(); }
	string getPriId() { return pri_id; }
	size_t getPriPos(size_t num = 0) { return get<0>(pri_coll[num]); }
	size_t getPriSize(size_t num = 0) { return get<1>(pri_coll[num]); }
	string getPriText(size_t num = 0) { return get<2>(pri_coll[num]); }

	size_t getSecCollSize() { return sec_coll.size(); }
	string getSecId() { return sec_id; }
	size_t getSecPos(size_t num = 0) { return get<0>(sec_coll[num]); }
	size_t getSecSize(size_t num = 0) { return get<1>(sec_coll[num]); }
	string getSecText(size_t num = 0) { return get<2>(sec_coll[num]); }

	size_t getScptCollSize() { return scpt_coll.size(); }
	string getScptLineType(size_t num) { return get<0>(scpt_coll[num]); }
	string getScptLine(size_t num) { return get<1>(scpt_coll[num]); }
	string getScptText(size_t num) { return get<2>(scpt_coll[num]); }
	size_t getScptTextPos(size_t num) { return get<3>(scpt_coll[num]); }

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
