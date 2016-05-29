#include "converter.hpp"

//----------------------------------------------------------
converter::converter(const char* base_path, const char* dict_path)
{
	base.readFile(base_path);
	dict.readDict(dict_path);

	file_name = base_path;
	file_name = file_name.substr(file_name.find_last_of("\\/") + 1);
	file_suffix = file_name.substr(file_name.rfind("."));
	file_name = file_name.substr(0, file_name.find_last_of("."));
}

//----------------------------------------------------------
void converter::writeFile()
{
	ofstream file;
	string new_name = file_name + "_conv" + file_suffix;
	file.open(new_name);
	file << file_content;
}

//----------------------------------------------------------
string converter::intToByte(unsigned int x)
{
	char bytes[4];
	string str;
	copy(static_cast<const char*>(static_cast<const void*>(&x)),
		static_cast<const char*>(static_cast<const void*>(&x)) + sizeof x,
		bytes);
	for(int i = 0; i < 4; i++)
	{
		str.push_back(bytes[i]);
	}
	return str;
}

//----------------------------------------------------------
void converter::printBinary(string str)
{
	for(size_t i = 0; i < str.size(); i++)
	{
		if(isprint(str.at(i)))
		{
			cout << str.at(i);
		}
		else
		{
			cout << ".";
		}
	}
}

//----------------------------------------------------------
void converter::convertCell()
{
	int counter_conv = 0;
	int counter_not = 0;
	base.resetRec();
	while(base.loopCheck())
	{
		base.setNextRec();
		base.setRecContent();
		current_rec = base.getRecContent();
		if(base.getRecId() == "CELL")
		{
			base.setPriSubRec("NAME");
			if(!base.getPriText().empty())
			{
				auto search = dict.getDict(0).find(base.getPriText());
				if(search != dict.getDict(0).end())
				{
					current_rec.erase(base.getPriPos() + 4, 4);
					current_rec.insert(base.getPriPos() + 4, intToByte(search->second.size() + 1));

					current_rec.erase(base.getPriPos() + 8, base.getPriSize());
					current_rec.insert(base.getPriPos() + 8, search->second + '\0');

					current_rec.erase(4, 4);
					current_rec.insert(4, intToByte(current_rec.size() - 16));

					file_content.append(current_rec);
					counter_conv++;
				}
			}
		}
		else
		{
			file_content.append(current_rec);
			counter_not++;
		}
	}
	cerr << "--> Converted: " << counter_conv << " records" << endl;
	cerr << "--> Ignored: " << counter_not << " records" << endl;
}

