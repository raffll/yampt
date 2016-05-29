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
	void writeDiff();

	merger() {}
	merger(const char* p1);
	merger(const char* p1, const char* p2);
	merger(const char* p1, const char* p2, const char* p3);

	array<dict_t, 10> dict_merged;

protected:
	void printStatus(const char* path);

	array<dicttools, 3> dict_unique;
};

#endif
