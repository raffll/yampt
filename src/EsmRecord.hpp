#ifndef RECTOOLS_HPP
#define RECTOOLS_HPP

#include "EsmReader.hpp"

class EsmRecord : public EsmReader
{
public:
	void setRec(size_t i);
	void setRecContent(std::string content) { *rec = content; }
	std::string getRecContent() { return *rec; }

	void setPri(std::string id);
	void setPriColl(std::string id);
	void setPriINDX();
	void setSec(std::string id);
	void setSecColl(std::string id);
	void setSecScptColl(std::string id);
	void setSecMessageColl(std::string id);
	void setSecDialType(std::string id);

	std::string getRecId() { return rec_id; }
	size_t getRecSize() { return rec_size; }

	std::vector<std::tuple<size_t, size_t, std::string>> getPriColl() { return pri_coll; }
	std::string getPriId() { return pri_id; }
	size_t getPriPos(size_t i = 0) { return std::get<0>(pri_coll[i]); }
	size_t getPriSize(size_t i = 0) { return std::get<1>(pri_coll[i]); }
	std::string getPriText(size_t i = 0) { return std::get<2>(pri_coll[i]); }

	std::vector<std::tuple<size_t, size_t, std::string>> getSecColl() { return sec_coll; }
	std::string getSecId() { return sec_id; }
	size_t getSecPos(size_t i = 0) { return std::get<0>(sec_coll[i]); }
	size_t getSecSize(size_t i = 0) { return std::get<1>(sec_coll[i]); }
	std::string getSecText(size_t i = 0) { return std::get<2>(sec_coll[i]); }

	std::vector<std::tuple<std::string, std::string, std::string, size_t>> getScptColl() { return scpt_coll; }
	std::string getScptLineType(size_t i) { return std::get<0>(scpt_coll[i]); }
	std::string getScptLine(size_t i) { return std::get<1>(scpt_coll[i]); }
	std::string getScptText(size_t i) { return std::get<2>(scpt_coll[i]); }
	size_t getScptTextPos(size_t i) { return std::get<3>(scpt_coll[i]); }

	std::string getDialType() { return dial_type; }

	EsmRecord();

private:
	void eraseNullChars(std::string &str);
	void replaceBrokenChars(std::string &str);
	std::string eraseCarriageReturnChar(std::string &str);
	void extractText(const std::string &line, std::string &text, size_t &pos);

	std::string *rec;

	size_t rec_size;
	std::string rec_id;

	std::string pri_id;
	std::vector<std::tuple<size_t, size_t, std::string>> pri_coll;

	std::string sec_id;
	std::vector<std::tuple<size_t, size_t, std::string>> sec_coll;

	std::vector<std::tuple<std::string, std::string, std::string, size_t>> scpt_coll;

	std::string dial_type;
	bool replace_broken_chars;
};

#endif
