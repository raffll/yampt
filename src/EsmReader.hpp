#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "Config.hpp"

class EsmReader : public Tools
{
public:
	void readFile(std::string path);

	bool getStatus() { return status; }
	std::string getName() { return name; }
	std::string getNamePrefix() { return name_prefix; }
	std::string getNameSuffix() { return name_suffix; }

	std::vector<std::string> const& getRecColl() const { return rec_coll; }

	EsmReader();

protected:
	bool status = 0;
	std::vector<std::string> rec_coll;

private:
	void printStatus(std::string path);
	void setName(std::string path);
	void setRecColl(std::string &content);

	std::string name;
	std::string name_prefix;
	std::string name_suffix;
};

#endif
