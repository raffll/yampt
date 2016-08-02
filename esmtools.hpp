#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "config.hpp"

class Esmtools
{
public:
	void readEsm(std::string path);
	void resetRec();
	bool setNextRec();
	void setRecContent();
	void setRecContent(std::string c) { rec_content = c; }
	void setEsmContent(std::string c) { esm_content = c; }
	bool setPriSubRec(std::string id, size_t next = 0);
	bool setSecSubRec(std::string id, size_t next = 0);
	void setPriSubRecINDX();
	void setCollScript();
	void setCollMessageOnly();

	std::string dialType();

	bool getEsmStatus() { return esm_status; }
	std::string getEsmName() { return esm_name; }
	std::string getEsmPrefix() { return esm_prefix; }
	std::string getEsmSuffix() { return esm_suffix; }
	std::string getEsmContent() { return esm_content; }

	size_t getRecSize() { return rec_size; }
	std::string getRecId() { return rec_id; }
	std::string getRecContent() { return rec_content; }

	size_t getPriPos() { return pri_pos; }
	size_t getPriSize() { return pri_size; }
	std::string getPriId() { return pri_id; }
	std::string getPriText() { return pri_text; }

	size_t getSecPos() { return sec_pos; }
	size_t getSecSize() { return sec_size; }
	std::string getSecId() { return sec_id; }
	std::string getSecText() { return sec_text; }

	std::string getCollType(int i) { return std::get<0>(text_coll[i]); }
	std::string getCollLine(int i) { return std::get<1>(text_coll[i]); }
	std::string getCollText(int i) { return std::get<2>(text_coll[i]); }
	size_t getCollPos(int i) { return std::get<3>(text_coll[i]); }
	size_t getCollSize() { return text_coll.size(); }

	Esmtools() {}
	Esmtools(const Esmtools& that);
	Esmtools& operator=(const Esmtools& that);
	~Esmtools();

private:
	void setEsmStatus(bool st, std::string path);
	void setEsmName(std::string path);

	unsigned int byteToInt(const std::string &str);
	void eraseNullChars(std::string &str);
	std::string eraseCarriageReturnChar(std::string &str);
	void addEndLineToLastCollItem();
	void extractText(const std::string &line, std::string &text, size_t &pos);

	bool esm_status = {};
	std::string esm_name;
	std::string esm_prefix;
	std::string esm_suffix;
	std::string esm_content;

	size_t rec_beg;
	size_t rec_end;
	size_t rec_size;
	std::string rec_id;
	std::string rec_content;

	size_t pri_pos;
	size_t pri_size;
	std::string pri_id;
	std::string pri_text;

	size_t sec_pos;
	size_t sec_size;
	std::string sec_id;
	std::string sec_text;

	std::vector<std::tuple<std::string, std::string, std::string, size_t>> text_coll;
};

#endif
