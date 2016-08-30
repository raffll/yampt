#ifndef ESMTOOLS_HPP
#define ESMTOOLS_HPP

#include "Config.hpp"

using namespace std;

class EsmReader
{
public:
	void readFile(string path);

	bool getStatus() { return status; }
	string getName() { return name; }
	string getNamePrefix() { return name_prefix; }
	string getNameSuffix() { return name_suffix; }

	vector<string> const& getRecColl() const { return rec_coll; }

	EsmReader();

protected:
	unsigned int convertByteArrayToInt(const string &str);

	bool status;
	vector<string> rec_coll;

private:
	void printStatus(string path);
	void setName(string path);
	void setRecColl(string &content);

	string name;
	string name_prefix;
	string name_suffix;

};

#endif
