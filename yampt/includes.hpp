#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <ctime>

#include <boost/filesystem/operations.hpp>

#ifdef DEBUG
#define DEBUG_MSG(str) addLog(str)
#else
#define DEBUG_MSG(str)
#endif
