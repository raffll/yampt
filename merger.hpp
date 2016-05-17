#ifndef MERGER_HPP
#define MERGER_HPP

#include <array>

#include "tools.hpp"
#include "dicttools.hpp"

using namespace std;

class merger: public tools
{
public:
	merger();

private:
	array<dicttools, 10> dict_loaded;
};

#endif
