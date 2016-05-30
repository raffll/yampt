#ifndef MERGER_HPP
#define MERGER_HPP

#include <array>
#include <map>

#include "tools.hpp"
#include "dicttools.hpp"

using namespace std;

class merger
{
public:
	void mergeDict();
	void writeMerged();
	void writeDiff();

	dict_t const& getDict(int i) const { return dict[i]; }

	merger() {}
	merger(vector<string>& path);

private:
	vector<dicttools> dict_tool;
	array<dict_t, 10> dict;
};

#endif
