#include "DictReader.hpp"

using namespace std;
using namespace yampt;

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
					         counter_all(that.counter_all),
					         log(that.log),
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
	counter_all = that.counter_all;
	log = that.log;
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

	makeLogHeader();

	while(true)
	{
		pos_beg = content.find(sep[1], pos_beg);
		pos_mid = content.find(sep[2], pos_mid);
		pos_end = content.find(sep[3], pos_end);
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
			unique_key = content.substr(pos_beg + sep[1].size(),
						    pos_mid - pos_beg - sep[1].size());

			friendly = content.substr(pos_mid + sep[2].size(),
						  pos_end - pos_mid - sep[2].size());

			counter_all++;

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
                        if(friendly.size() > 63)
			{
				merger_log_ptr = &merger_log[3];
				makeLog();
				counter_toolong++;
			}
			else
			{
                                insertRecord(rec_type::CELL);
			}
		}
		else if(id == "GMST")
		{
			insertRecord(rec_type::GMST);
		}
		else if(id == "DESC")
		{
			insertRecord(rec_type::DESC);
		}
		else if(id == "TEXT")
		{
			insertRecord(rec_type::TEXT);
		}
		else if(id == "INDX")
		{
			insertRecord(rec_type::INDX);
		}
		else if(id == "DIAL")
		{
			insertRecord(rec_type::DIAL);
		}
		else if(id == "BNAM")
		{
			friendly = friendly.substr(friendly.find(sep[0]) + 1);
			insertRecord(rec_type::BNAM);
		}
		else if(id == "SCTX")
		{
			friendly = friendly.substr(friendly.find(sep[0]) + 1);
			insertRecord(rec_type::SCTX);
		}
		else if(id == "RNAM")
		{
			if(friendly.size() > 32)
			{
				merger_log_ptr = &merger_log[3];
				makeLog();
				counter_toolong++;
			}
			else
			{
				insertRecord(rec_type::RNAM);
			}
		}
		else if(id == "FNAM")
		{
			if(friendly.size() > 31)
			{
				merger_log_ptr = &merger_log[3];
				makeLog();
				counter_toolong++;
			}
			else
			{
				insertRecord(rec_type::FNAM);
			}
		}
		else if(id == "INFO")
		{
			if(friendly.size() > 512)
			{
				merger_log_ptr = &merger_log[4];
				makeLog();
				insertRecord(rec_type::INFO);
			}
			else
			{
				insertRecord(rec_type::INFO);
			}
		}
		else
		{
			merger_log_ptr = &merger_log[2];
			makeLog();
			counter_invalid++;
		}
	}
	else
	{
		merger_log_ptr = &merger_log[2];
		makeLog();
		counter_invalid++;
	}
}

//----------------------------------------------------------
void DictReader::insertRecord(rec_type type)
{
	if(dict[type].insert({unique_key, friendly}).second == true)
	{
		counter_loaded++;
	}
	else
	{
		merger_log_ptr = &merger_log[1];
		makeLog();
		counter_doubled++;
	}
}

//----------------------------------------------------------
void DictReader::makeLogHeader()
{
	log += "<!-- Loading " + name + "... -->\r\n";
	log += sep_line + "\r\n";
}

//----------------------------------------------------------
void DictReader::makeLog()
{
	log += "<!-- " + *merger_log_ptr;
	if(merger_log_ptr == &merger_log[3] || merger_log_ptr == &merger_log[4])
        {
                log += " (" + to_string(friendly.size()) + " bytes)";
        }
        log += " -->\r\n";
	log += sep[1] + unique_key + sep[2] + friendly + sep[3] + "\r\n";
	log += sep_line + "\r\n";
}

//----------------------------------------------------------
void DictReader::printLog()
{
	cout << "--------------------------------------------------" << endl
	     << "    Loaded / Doubled / Too long / Invalid /    All" << endl
	     << "--------------------------------------------------" << endl
	     << setw(10) << to_string(counter_loaded) << " / "
	     << setw(7) << to_string(counter_doubled) << " / "
	     << setw(8) << to_string(counter_toolong) << " / "
	     << setw(7) << to_string(counter_invalid) << " / "
	     << setw(6) << to_string(counter_all) << endl
	     << "--------------------------------------------------" << endl;
}
