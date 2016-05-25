#ifndef MERGER_HPP
#define MERGER_HPP

#include <array>
#include <map>

#include "tools.hpp"
#include "dicttools.hpp"

using namespace std;

class merger: public tools
{
public:
	void mergeDict(int i);
	void writeDict(int i);
	void writeDuplicatesAll();
	void writeDuplicates(int i);

	merger() {}
	merger(const char* path_pri);
	merger(const char* path_pri, const char* path_sec);

private:
	dicttools dict_pri;
	dicttools dict_sec;

	array<dict_t, 10> dict_merged;
};

#endif
