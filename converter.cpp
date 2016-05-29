#include "converter.hpp"

//----------------------------------------------------------
converter::converter(const char* file, const char* dict)
{
	base.readFile(file);
	dict_unique[0].readDict(dict);
	mergeDict();
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
	counter = 0;
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
				auto search = dict_merged[0].find(base.getPriText());
				if(search != dict_merged[0].end())
				{
					printBinary(current_rec);
					cout << endl << endl;

					current_rec.erase(base.getPriPos() + 4, 4);
					current_rec.insert(base.getPriPos() + 4, intToByte(search->second.size() + 1));

					current_rec.erase(base.getPriPos() + 8, base.getPriSize());
					current_rec.insert(base.getPriPos() + 8, search->second + '\0');


					printBinary(current_rec);
					cout << endl << endl;

					counter++;
					file_content.append(current_rec);
				}
			}
		}

		file_content.append(current_rec);
	}
}
