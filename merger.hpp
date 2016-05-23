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
	void findDuplicates(int i);

	merger() {}
	merger(const char* path_1, const char* path_2);

private:
	dicttools dict_1;
	dicttools dict_2;

	array<multimap<string, pair<size_t, string>>, 10> dict;
};

#endif
