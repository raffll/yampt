#include "DictReader.hpp"

using namespace std;

//----------------------------------------------------------
DictReader::DictReader()
{

}

//----------------------------------------------------------
DictReader::DictReader(const DictReader& that) : status(that.status),
					         name(that.name),
					         name_prefix(that.name_prefix),
					         counter_loaded(that.counter_loaded),
					         counter_invalid(that.counter_invalid),
					         counter_toolong(that.counter_toolong),
					         counter_doubled(that.counter_doubled),
					         dict(that.dict)
{

}

//----------------------------------------------------------
DictReader& DictReader::operator=(const DictReader& that)
{
	status = that.status;
	name = that.name;
	name_prefix = that.name_prefix;
	counter_loaded = that.counter_loaded;
	counter_invalid = that.counter_invalid;
	counter_toolong = that.counter_toolong;
	counter_doubled = that.counter_doubled;
	dict = that.dict;
	return *this;
}

//----------------------------------------------------------
DictReader::~DictReader()
{

}

//----------------------------------------------------------
void DictReader::readFile(string path)
{
	ifstream file(path, ios::binary);
	if(file)
	{
		string content;
		char buffer[16384];
		size_t size = file.tellg();
		content.reserve(size);
		streamsize chars_read;
		while(file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}
		setName(path);
		if(!content.empty() && parseDict(content) == true)
		{
			status = true;
		}
	}
	printStatus(path);
}

//----------------------------------------------------------
void DictReader::printStatus(string path)
{
	if(status == false)
	{
		cout << "--> Error while loading " << path << " (wrong path or missing separator)!\r\n";
	}
	else
	{
		cout << "--> Loading " << path << "..." << endl;
		printLog();
	}
}

//----------------------------------------------------------
void DictReader::setName(string path)
{
	name = path.substr(path.find_last_of("\\/") + 1);
	name_prefix = name.substr(0, name.find_last_of("."));
}

//----------------------------------------------------------
bool DictReader::parseDict(string &content)
{
	size_t pos_beg = 0;
	size_t pos_mid = 0;
	size_t pos_end = 0;

	while(true)
	{
		pos_beg = content.find(yampt::sep[1], pos_beg);
		pos_mid = content.find(yampt::sep[2], pos_mid);
		pos_end = content.find(yampt::sep[3], pos_end);
		if(pos_beg == string::npos &&
		   pos_mid == string::npos &&
		   pos_end == string::npos)
		{
			return true;
			break;
		}
		else if(pos_beg > pos_mid ||
			pos_beg > pos_end ||
			pos_mid > pos_end ||
			pos_end == string::npos)
		{
			return false;
			break;
		}
		else
		{
			unique_key = content.substr(pos_beg + yampt::sep[1].size(),
						    pos_mid - pos_beg - yampt::sep[1].size());

			friendly = content.substr(pos_mid + yampt::sep[2].size(),
						  pos_end - pos_mid - yampt::sep[2].size());

			validateRecord();

			pos_beg++;
			pos_mid++;
			pos_end++;
		}
	}
}

//----------------------------------------------------------
void DictReader::validateRecord()
{
	string id;
	if(unique_key.size() > 4)
	{
		id = unique_key.substr(0, 4);

		if(id == "CELL")
		{
			insertRecord(yampt::r_type::CELL);
		}
		else if(id == "GMST")
		{
			insertRecord(yampt::r_type::GMST);
		}
		else if(id == "DESC")
		{
			insertRecord(yampt::r_type::DESC);
		}
		else if(id == "TEXT")
		{
			insertRecord(yampt::r_type::TEXT);
		}
		else if(id == "INDX")
		{
			insertRecord(yampt::r_type::INDX);
		}
		else if(id == "DIAL")
		{
			insertRecord(yampt::r_type::DIAL);
		}
		else if(id == "BNAM")
		{
			friendly = friendly.substr(friendly.find(yampt::sep[0]) + 1);
			insertRecord(yampt::r_type::BNAM);
		}
		else if(id == "SCTX")
		{
			friendly = friendly.substr(friendly.find(yampt::sep[0]) + 1);
			insertRecord(yampt::r_type::SCTX);
		}
		else if(id == "RNAM")
		{
			if(friendly.size() > 32)
			{
				valid_ptr = &yampt::valid[3];
				makeLog();
				counter_toolong++;
			}
			else
			{
				insertRecord(yampt::r_type::RNAM);
			}
		}
		else if(id == "FNAM")
		{
			if(friendly.size() > 32)
			{
				valid_ptr = &yampt::valid[3];
				makeLog();
				counter_toolong++;
			}
			else
			{
				insertRecord(yampt::r_type::FNAM);
			}
		}
		else if(id == "INFO")
		{
			if(friendly.size() > 512)
			{
				valid_ptr = &yampt::valid[4];
				makeLog();
				counter_toolong++;
				insertRecord(yampt::r_type::INFO);
			}
			else
			{
				insertRecord(yampt::r_type::INFO);
			}
		}
		else
		{
			valid_ptr = &yampt::valid[2];
			makeLog();
			counter_invalid++;
		}
	}
	else
	{
		valid_ptr = &yampt::valid[2];
		makeLog();
		counter_invalid++;
	}
}

//----------------------------------------------------------
void DictReader::insertRecord(yampt::r_type type)
{
	if(dict[type].insert({unique_key, friendly}).second == true)
	{
		counter_loaded++;
	}
	else
	{
		valid_ptr = &yampt::valid[1];
		makeLog();
		counter_doubled++;
	}
}

//----------------------------------------------------------
void DictReader::makeLog()
{
	log += "Dictionary: " + name + "\r\n" +
	       "Record    : " + unique_key + "\r\n" +
	       "Result    : " + *valid_ptr + "\r\n" +
	       "--------------------------------------------------" + "\r\n" +
	       friendly + "\r\n" +
	       "--------------------------------------------------" + "\r\n\r\n\r\n";
}

//----------------------------------------------------------
void DictReader::printLog()
{
	cout << endl
	     << "    Loaded / Doubled / Too long / Invalid" << endl
	     << "    -------------------------------------" << endl
	     << setw(10) << to_string(counter_loaded) << " / "
	     << setw(7) << to_string(counter_doubled) << " / "
	     << setw(8) << to_string(counter_toolong) << " / "
	     << setw(7) << to_string(counter_invalid)
	     << endl << endl;
}
