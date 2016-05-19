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

	merger();
	merger(const char* path1, const char* path2);
	merger(const char* path1, const char* path2, const char* path3);

private:
	dicttools dict_first;
	dicttools dict_second;
	dicttools dict_third;

	array<multimap<string, string>, 10> dict;
};

#endif
