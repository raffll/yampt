#include "dicttools.hpp"

using namespace std;

//----------------------------------------------------------
dicttools::dicttools()
{
	is_loaded = 0;
}

//----------------------------------------------------------
void dicttools::readFile(const char* path, int i)
{
	dict_number = i;
	file_name += "dict_" + to_string(i) + "_" + dict_name[i] + ".txt";
	string file_content;
	ifstream file(file_name.c_str());
	cutFileName(file_name);

	if(file.good())
	{
		char buffer[16384];
		size_t file_size = file.tellg();
		file_content.reserve(file_size);
		streamsize chars_read;

		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			file_content.append(buffer, chars_read);
		}

		if(validateDict(file_content) == 1)
		{
			parseDict(file_content);
			is_loaded = 1;
			cout << file_name << " is loaded: " << is_loaded << endl;
		}
		else
		{
			is_loaded = 0;
		}
	}
	else
	{
		is_loaded = 0;
	}
}

//----------------------------------------------------------
void dicttools::printStatus()
{
	if(quiet == 0)
	{
		if(is_loaded == 1)
		{
			cout << file_name << " is loaded: " << is_loaded << endl;
		}
		else
		{
			cout << file_name << " is damaged - missing separator!" << endl;
		}
	}
}

//----------------------------------------------------------
void dicttools::printDict()
{
	for(const auto &elem : dict_in)
	{
		cout << elem.first << " " << elem.second.first << " " << elem.second.second << endl;
	}
}

//----------------------------------------------------------
bool dicttools::validateDict(string &file_content)
{
	while(getline(file

//----------------------------------------------------------
// obsolete
//----------------------------------------------------------
bool dicttools::validateDict(string &file_content)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;

	while(true)
	{
		pos_beg = file_content.find(line_sep[0], pos_beg);
		pos_mid = file_content.find(line_sep[1], pos_mid);
		pos_end = file_content.find(line_sep[2], pos_end);
		if(pos_beg == string::npos && pos_mid == string::npos && pos_end == string::npos)
		{
			return true;
			break;
		}
		else if(pos_beg > pos_mid || pos_beg > pos_end || pos_mid > pos_end || pos_end == string::npos)
		{
			return false;
			break;
		}

		else
		{
			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void dicttools::parseDict(string &file_content)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;
	size_t pri_size;
	size_t sec_size;
	string pri_text;
	string sec_text;

	if(is_loaded == 1)
	{
		while(true)
		{
			pos_beg = file_content.find(line_sep[0], pos_beg);
			pos_mid = file_content.find(line_sep[1], pos_mid);
			pos_end = file_content.find(line_sep[2], pos_end);

			if(pos_beg == string::npos)
			{
				break;
			}
			else
			{
				pri_size = pos_mid - pos_beg - line_sep[0].size();
				sec_size = pos_end - pos_mid - line_sep[1].size();
				pri_text = file_content.substr(pos_beg + line_sep[0].size(), pri_size);
				sec_text = file_content.substr(pos_mid + line_sep[1].size(), sec_size);

				dict_in.insert(make_pair(pri_text, make_pair(sec_size, sec_text)));

				pos_beg++;
				pos_mid++;
				pos_end++;
			}
		}
	}
}

//----------------------------------------------------------
/*void dicttools::parseDict()
{
	if(is_loaded == 1)
	{
		string test = "<h3>DUPADUPA</h3>Pierwsza linia\n"
					  "Druga linia<hr>\n"
					  "<h3>Kasztan</h3>Pierwsza linia test\n"
					  "Druga linia test<hr>\n";

		regex re("(<h3>)(.*)(</h3>)");
		sregex_iterator next(file_content.begin(), file_content.end(), re);
		sregex_iterator end;
		smatch m;

		while(next != end)
		{
			m = *next;
			cout << m.str(2) << endl;
			//dict.insert({pri_match.str(), sec_match.str()});
			next++;
		}

		regex re("<(/h3)>((.|\n)*)<(hr)>");
		sregex_iterator next(test.begin(), test.end(), re);
		sregex_iterator end;
		smatch m;

		while(next != end)
		{
			m = *next;
			for(size_t i = 0; i < m.size(); ++i)
			{
				cout << i << ": " << m.str(i) << endl;
			}
			//dict.insert({pri_match.str(), sec_match.str()});
			next++;
		}

		//for(const auto &elem : dict)
		//{
		//	cout << elem.first << elem.second << endl;
		//}
	}
}*/

