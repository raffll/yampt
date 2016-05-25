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
	void mergeDict();
	void writeMerged();
	void writeDiffLog();

	merger() {}
	merger(const char* path_pri);
	merger(const char* path_pri, const char* path_sec);

private:
	void printStatus(int i);

	array<dicttools, 3> dict_unique;
	array<dict_t, 10> dict_merged;
};

#endif
