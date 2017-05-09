#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <locale>
#include <regex>

namespace yampt
{

struct CaseAwareCompare
{
	bool operator()(const char * left, const char * right) const
	{
		bool tied = true;
		bool tiebreaker = false;
        int i;

		for(i = 0; left[i] != 0; ++i)
		{
			if(right[i] == 0)
			{
				return false;
			}
			if(tolower(left[i]) != tolower(right[i]))
			{
				return tolower(left[i]) < tolower(right[i]);
			}
			if(tied && left[i] != right[i])
			{
				tied = false;
				tiebreaker = left[i] < right[i];
			}
		}
		return(right[i] != 0) || (!tied && tiebreaker);
	}

	bool operator()(const std::string & left, const std::string & right) const
	{
		return operator()(left.c_str(), right.c_str());
	}
};

enum r_type { CELL, DIAL, INDX, RNAM, DESC, GMST, FNAM, INFO, BNAM, SCTX, TEXT };
enum ins_mode { RAW, BASE, ALL, NOTFOUND, CHANGED };

typedef std::array<std::map<std::string, std::string, CaseAwareCompare>, 11> dict_t;

const std::array<std::string, 5> dialog_type = {"T", "V", "G", "P", "J"};
const std::vector<std::string> sep = {"^", "<h3>", "</h3>", "<hr>"};
const std::string line = "<!------------------------------------------------------------>";
const std::vector<std::string> key_message = {"messagebox", "say ", "say,", "choice"};
const std::vector<std::string> converter_log = {"UNCHANGED", "CONVERTED", "SKIPPED", "LINK ADDED"};
const std::vector<std::string> merger_log = {"REPLACED", "DOUBLED", "INVALID", "INVALID", "LOADED"};

}

class Writer
{
public:
	void writeDict(const yampt::dict_t &dict, std::string name);
	void writeText(const std::string &text, std::string name);
	int getSize(const yampt::dict_t &dict);
};

class Tools
{
protected:
	unsigned int convertByteArrayToInt(const std::string &str);
	std::string convertIntToByteArray(unsigned int x);
	bool caseInsensitiveStringCmp(std::string lhs, std::string rhs);
	void eraseNullChars(std::string &str);
	std::string eraseCarriageReturnChar(std::string &str);
};

#endif
