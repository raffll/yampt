#ifndef RECTOOLS_HPP
#define RECTOOLS_HPP

#include "EsmReader.hpp"

class EsmRecord : public EsmReader
{
public:
	void setRec(size_t i);
	void setRecContent(std::string content) { *rec = content; }
	std::string getRecContent() { return *rec; }

	void setUnique(std::string id, bool erase_null = true);
	bool setFriendly(std::string id, bool next = false, bool erase_null = true);
	void setDump();

	std::string getRecId() { return rec_id; }

	std::string getUnique() { return unique_text; }
	std::string getUniqueId() { return unique_id; }
	bool getUniqueStatus() { return unique_status; }

	std::string getFriendly() { return friendly_text; }
	std::string getFriendlyId() { return friendly_id; }
	size_t getFriendlyPos() { return friendly_pos; }
	size_t getFriendlySize() { return friendly_size; }
	size_t getFriendlyCounter() { return friendly_counter; }
	bool getFriendlyStatus() { return friendly_status; }

	std::string getDump() { return dump; }

	EsmRecord();

private:
	std::string *rec;

	size_t rec_size;
	std::string rec_id;

	std::string unique_id;
	std::string unique_text;
	bool unique_status;

	std::string friendly_id;
	std::string friendly_text;
	size_t friendly_pos;
	size_t friendly_size;
	size_t friendly_counter;
	bool friendly_status;

	std::string dump;
};

#endif
