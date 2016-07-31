#ifndef DICTTOOLS_HPP
#define DICTTOOLS_HPP

#include "config.hpp"

class Dicttools
{
public:
	void readDict(std::string path);
	bool getDictStatus() { return status; }
	std::string getDictName() { return name; }
	std::string getDictPrefix() { return prefix; }
	std::string getDictLog() { return log; }
	std::map<std::string, std::string> const& getDict() const { return dict; }

	Dicttools() {}
	Dicttools(const Dicttools& that);
	Dicttools& operator=(const Dicttools& that);
	~Dicttools();

private:
	void setDictStatus(bool st, std::string path);
	void setDictName(std::string path);
	void parseDict(std::string path);
	bool validateRecLength(const std::string &pri, const std::string &sec);

	bool status = {};
	std::string name;
	std::string prefix;
	std::string content;
	std::string log;
	int invalid;
	std::map<std::string, std::string> dict;
};

#endif
