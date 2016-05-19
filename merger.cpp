#include "merger.hpp"

//----------------------------------------------------------
merger::merger()
{

}

//----------------------------------------------------------
merger::merger(const char* path1, const char* path2)
{
	dict_first.readDictAll(path1);
	dict_second.readDictAll(path2);
}

//----------------------------------------------------------
merger::merger(const char* path1, const char* path2, const char* path3)
{
	dict_first.readDictAll(path1);
	dict_second.readDictAll(path2);
	dict_third.readDictAll(path3);
}

//----------------------------------------------------------
void merger::mergeDict()
{

}

