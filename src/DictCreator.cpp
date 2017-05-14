#include "DictCreator.hpp"

using namespace std;

//----------------------------------------------------------
DictCreator::DictCreator(string path_n)
{
	esm_ptr = &esm_n;
	message_ptr = &message_n;

	mode = yampt::ins_mode::RAW;

	esm_n.readFile(path_n);

	if(esm_n.getStatus() == true)
	{
		status = true;
		basic_mode = true;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string path_n, string path_f)
{
	esm_ptr = &esm_f;
	message_ptr = &message_f;

	mode = yampt::ins_mode::BASE;

	esm_n.readFile(path_n);
	esm_f.readFile(path_f);

	if(esm_n.getStatus() == true && esm_f.getStatus() == true)
	{
		if(compareMasterFiles())
		{
			basic_mode = true;
		}
		else
		{
			basic_mode = false;
		}
		status = true;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string path_n, DictMerger &merger, yampt::ins_mode mode)
{
	this->merger = &merger;
	this->mode = mode;

	esm_ptr = &esm_n;
	message_ptr = &message_n;

	esm_n.readFile(path_n);

	if(esm_n.getStatus() == true && merger.getStatus() == true)
	{
		status = true;
		basic_mode = true;
	}
}

//----------------------------------------------------------
void DictCreator::makeDict()
{
	if(basic_mode == true)
	{
		makeDictBasic();
	}
	else
	{
		makeDictExtended();
	}
}

//----------------------------------------------------------
void DictCreator::makeDictBasic()
{
	if(status == true)
	{
		printLogHeader();

		makeDictCELL();
		makeDictDefaultCELL();
		makeDictRegionCELL();
		makeDictGMST();
		makeDictFNAM();
		makeDictDESC();
		makeDictTEXT();
		makeDictRNAM();
		makeDictINDX();
		makeDictDIAL();
		makeDictINFO();
		makeDictBNAM();
		makeDictSCPT();

		cout << "----------------------------------------------" << endl;
	}
}

//----------------------------------------------------------
void DictCreator::makeDictExtended()
{
	if(status == true)
	{
		printLogHeader();

		makeDictCELLExtended();
		makeDictDefaultCELLExtended();
		makeDictRegionCELLExtended();
		makeDictGMST();
		makeDictFNAM();
		makeDictDESC();
		makeDictTEXT();
		makeDictRNAM();
		makeDictINDX();
		makeDictDIALExtended();
		makeDictINFO();
		makeDictBNAMExtended();
		makeDictSCPTExtended();

		cout << "----------------------------------------------" << endl;
	}
}

//----------------------------------------------------------
void DictCreator::printLogHeader()
{
	cout << "----------------------------------------------" << endl;
	cout << "         Created / Doubled / Identical /   All" << endl;
	cout << "----------------------------------------------" << endl;
}

//----------------------------------------------------------
void DictCreator::printLog(string id)
{
	string id_text = id;
	id_text.resize(9, ' ');
	cout << id_text
	     << setw(7) << to_string(counter_created) << " / "
	     << setw(7) << to_string(counter_doubled) << " / "
	     << setw(8) << to_string(counter_identical) << " / "
	     << setw(6) << to_string(counter_all) << endl;
}

//----------------------------------------------------------
bool DictCreator::compareMasterFiles()
{
	string esm_n_compare;
	string esm_f_compare;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_n_compare += esm_n.getRecId();
	}

	for(size_t i = 0; i < esm_f.getRecColl().size(); ++i)
	{
		esm_f.setRec(i);
		esm_f_compare += esm_f.getRecId();
	}

	if(esm_n_compare != esm_f_compare)
	{
		cout << "--> Files have records in different order!" << endl;
		cout << "    If any doubled records will be created," << endl;
		cout <<	"    please check DIAL and CELL dictionary manually!" << endl;
		cout << "    Problematic records are marked as DOUBLED." << endl;
		cout << "    Please be patient..." << endl;
		return false;
	}
	else
	{
		return true;
	}
}

//----------------------------------------------------------
void DictCreator::resetCounters()
{
	counter_created = 0;
	counter_doubled = 0;
	counter_identical = 0;
	counter_all = 0;
}

//----------------------------------------------------------
void DictCreator::validateRecord(const string &unique_key, const string &friendly, yampt::r_type type)
{
	counter_all++;

	// Insert without special cases
	if(mode == yampt::ins_mode::RAW || mode == yampt::ins_mode::BASE)
	{
		insertRecord(unique_key, friendly, type);
	}

	// For CELL, DIAL, BNAM, SCTX find corresponding record in dictionary given by user
	if(mode == yampt::ins_mode::ALL)
	{
		if(type == yampt::r_type::CELL ||
		   type == yampt::r_type::DIAL ||
		   type == yampt::r_type::BNAM ||
		   type == yampt::r_type::SCTX)
		{
			auto search = merger->getDict()[type].find(unique_key);
			if(search != merger->getDict()[type].end())
			{
				insertRecord(unique_key, search->second, type);
			}
			else
			{
				insertRecord(unique_key, friendly, type);
			}
		}
		else
		{
			insertRecord(unique_key, friendly, type);
		}
	}

	// Insert only ones not found in dictionary given by user
	if(mode == yampt::ins_mode::NOTFOUND)
	{
		auto search = merger->getDict()[type].find(unique_key);
		if(search == merger->getDict()[type].end())
		{
			insertRecord(unique_key, friendly, type);
		}
	}

	// Insert only with changed friendly text compared to dictionary given by user
	if(mode == yampt::ins_mode::CHANGED)
	{
		if(type == yampt::r_type::GMST ||
		   type == yampt::r_type::FNAM ||
		   type == yampt::r_type::DESC ||
		   type == yampt::r_type::TEXT ||
		   type == yampt::r_type::RNAM ||
		   type == yampt::r_type::INDX ||
		   type == yampt::r_type::INFO)
		{
			auto search = merger->getDict()[type].find(unique_key);
			if(search != merger->getDict()[type].end())
			{
				if(search->second != friendly)
				{
					insertRecord(unique_key, friendly, type);
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const string &unique_key, const string &friendly, yampt::r_type type)
{
	if(dict[type].insert({unique_key, friendly}).second == true)
	{
		counter_created++;
	}
	else
	{
		auto search = dict[type].find(unique_key);
		if(friendly != search->second)
		{
			string temp = unique_key + "<DOUBLED_" + to_string(counter_doubled + 1) + ">";
			if(dict[type].insert({temp, friendly}).second == true)
			{
				counter_doubled++;
			}
			else
			{
				counter_identical++;
			}
		}
		else
		{
			counter_identical++;
		}
	}
}

//----------------------------------------------------------
string DictCreator::dialTranslator(string to_translate)
{
	if(mode == yampt::ins_mode::ALL ||
	   mode == yampt::ins_mode::NOTFOUND ||
	   mode == yampt::ins_mode::CHANGED)
	{
		auto search = merger->getDict()[yampt::r_type::DIAL].find("DIAL" + yampt::sep[0] + to_translate);
		if(search != merger->getDict()[yampt::r_type::DIAL].end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
vector<string> DictCreator::makeMessageColl(const string &script_text)
{
	vector<string> message;
	bool found;
	string line;
	string line_lc;
	size_t pos;

	istringstream ss(script_text);

	while(getline(ss, line))
	{
		found = false;
		eraseCarriageReturnChar(line);
		line_lc = line;
		transform(line_lc.begin(), line_lc.end(),
			  line_lc.begin(), ::tolower);

		for(auto const &elem : yampt::key_message)
		{
			if(found == false)
			{
				pos = line_lc.find(elem);
				if(pos != string::npos &&
				   line.rfind(";", pos) == string::npos)
				{
					message.push_back(line);
					found = true;
					break;
				}
			}
		}
	}
	return message;
}

//----------------------------------------------------------
void DictCreator::binaryDump()
{
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_n.setDump();
		dump += esm_n.getRecId() + "\r\n";
		dump += esm_n.getDump() + "\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("NAME");

			esm_f.setRec(i);
			esm_f.setFriendly("NAME");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
					       esm_n.getFriendly(),
					       yampt::r_type::CELL);
			}
		}
	}
	printLog("CELL");
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtended()
{
	resetCounters();
	string pattern;
	string match;
	bool found = false;

	cout << "CELL in progress..." << flush;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setUnique("NAME");

			if(esm_n.getUniqueStatus())
			{
				found = false;

				// Pattern is the DATA and combined id of all objects in cell
				pattern = "";

				esm_n.setFriendly("DATA", false, false);
				pattern += esm_n.getFriendly();

				esm_n.setFriendly("NAME", false, false);

				while(esm_n.getFriendlyStatus())
				{
					esm_n.setFriendly("NAME", true, false);
					pattern += esm_n.getFriendly();
				}

				// Search for match in every CELL record
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);

					if(esm_f.getRecId() == "CELL")
					{
						match = "";

						esm_f.setFriendly("DATA", false, false);
						match += esm_f.getFriendly();

						esm_f.setFriendly("NAME", false, false);

						while(esm_f.getFriendlyStatus())
						{
							esm_f.setFriendly("NAME", true, false);
							match += esm_f.getFriendly();
						}

						if(match == pattern)
						{
							found = true;
							break;
						}
					}
				}

				esm_n.setFriendly("NAME");

				if(found == true)
				{
					esm_f.setFriendly("NAME");
				}
				else
				{
					esm_f.setFriendly("<NOTFOUND>");
				}

				if(esm_n.getFriendlyStatus())
				{
					validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
						       esm_n.getFriendly(),
						       yampt::r_type::CELL);
				}

				if(counter_created % 25 == 0)
				{
					cout << "." << flush;
				}
			}
		}
	}

	cout << endl;

	printLog("CELL");
}

//----------------------------------------------------------
void DictCreator::makeDictDefaultCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

			esm_f.setRec(i);
			esm_f.setFriendly("STRV");

                        if(esm_n.getUnique() == "sDefaultCellname" &&
			   esm_n.getFriendlyStatus())
                        {
				validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
						esm_n.getFriendly(),
						yampt::r_type::CELL);
                        }
		}
	}
	printLog("+ Default");
}

//----------------------------------------------------------
void DictCreator::makeDictDefaultCELLExtended()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

                        if(esm_n.getUnique() == "sDefaultCellname" &&
                           esm_n.getFriendlyStatus())
                        {
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);
					if(esm_f.getRecId() == "GMST")
					{
						esm_f.setUnique("NAME");
						esm_f.setFriendly("STRV");

						if(esm_f.getUnique() == "sDefaultCellname" &&
						   esm_f.getFriendlyStatus())
						{
							validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
									esm_n.getFriendly(),
									yampt::r_type::CELL);
							break;
						}
					}
				}
			}
		}
	}
	printLog("+ Default");
}

//----------------------------------------------------------
void DictCreator::makeDictRegionCELL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "REGN")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

			esm_f.setRec(i);
			esm_f.setFriendly("FNAM");

                        if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
                        {
				validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
						esm_n.getFriendly(),
						yampt::r_type::CELL);
                        }
		}
	}
	printLog("+ Region ");
}

//----------------------------------------------------------
void DictCreator::makeDictRegionCELLExtended()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "REGN")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

                        if(esm_n.getUniqueStatus() &&
                           esm_n.getFriendlyStatus())
                        {
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);
					if(esm_f.getRecId() == "REGN")
					{
						esm_f.setUnique("NAME");
						esm_f.setFriendly("FNAM");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus())
						{
							validateRecord("CELL" + yampt::sep[0] + esm_f.getFriendly(),
									esm_n.getFriendly(),
									yampt::r_type::CELL);
							break;
						}
					}
				}
			}
		}
	}
	printLog("+ Region ");
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus() &&
			   esm_n.getUnique().substr(0, 1) == "s")	// Make sure is string
			{
				validateRecord("GMST" + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::GMST);
			}
		}
	}
	printLog("GMST");
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "ACTI" ||
		   esm_n.getRecId() == "ALCH" ||
		   esm_n.getRecId() == "APPA" ||
		   esm_n.getRecId() == "ARMO" ||
		   esm_n.getRecId() == "BOOK" ||
		   esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "CLOT" ||
		   esm_n.getRecId() == "CONT" ||
		   esm_n.getRecId() == "CREA" ||
		   esm_n.getRecId() == "DOOR" ||
		   esm_n.getRecId() == "FACT" ||
		   esm_n.getRecId() == "INGR" ||
		   esm_n.getRecId() == "LIGH" ||
		   esm_n.getRecId() == "LOCK" ||
		   esm_n.getRecId() == "MISC" ||
		   esm_n.getRecId() == "NPC_" ||
		   esm_n.getRecId() == "PROB" ||
		   esm_n.getRecId() == "RACE" ||
		   esm_n.getRecId() == "REGN" ||
		   esm_n.getRecId() == "REPA" ||
		   esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "SPEL" ||
		   esm_n.getRecId() == "WEAP")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("FNAM");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus() &&
			   esm_n.getUnique() != "player")	// Skip player name
			{
				validateRecord("FNAM" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::FNAM);
			}
		}
	}
	printLog("FNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "RACE")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("DESC");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("DESC" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::DESC);
			}
		}
	}
	printLog("DESC");
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "BOOK")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("TEXT");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("TEXT" + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::TEXT);
			}
		}
	}
	printLog("TEXT");
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "FACT")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("RNAM");

			if(esm_n.getUniqueStatus())
			{
				while(esm_n.getFriendlyStatus())
				{
					validateRecord("RNAM" + yampt::sep[0] + esm_n.getUnique() + yampt::sep[0] + to_string(esm_n.getFriendlyCounter()),
						       esm_n.getFriendly(),
						       yampt::r_type::RNAM);

					esm_n.setFriendly("RNAM", true);
				}
			}
		}
	}
	printLog("RNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "MGEF")
		{
			esm_n.setUnique("INDX");
			esm_n.setFriendly("DESC");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("INDX" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::INDX);
			}
		}
	}
	printLog("INDX");
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");
			esm_n.setFriendly("NAME");
			esm_f.setFriendly("NAME");

			if(esm_n.getUnique() == "T" &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("DIAL" + yampt::sep[0] + esm_ptr->getFriendly(),
					       esm_n.getFriendly(),
					       yampt::r_type::DIAL);
			}
		}
	}
	printLog("DIAL");
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtended()
{
	resetCounters();
	string pattern;
	string match;

	cout << "DIAL in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");

			if(esm_n.getUnique() == "T")
			{
				// Pattern is the id of corresponding INFO string
				esm_n.setRec(i + 1);

				esm_n.setFriendly("INAM", false, false);
				pattern = esm_n.getFriendly();

				esm_n.setFriendly("SCVR", false, false);
				pattern += esm_n.getFriendly();

				esm_n.setRec(i);
				esm_n.setFriendly("NAME");

				//cout << "Pattern: " << pattern << endl;

				// Search for all match
				for(size_t k = 0; k < esm_f.getRecColl().size(); ++k)
				{
					esm_f.setRec(k);

					if(esm_f.getRecId() == "DIAL")
					{
						esm_f.setRec(k + 1);

						esm_f.setFriendly("INAM", false, false);
						match = esm_f.getFriendly();

						esm_f.setFriendly("SCVR", false, false);
						match += esm_f.getFriendly();

						if(match == pattern)
						{
							esm_f.setRec(k);
							esm_f.setFriendly("NAME");

							//cout << "Match: " << match << endl;

							break;
						}
					}
				}

				if(esm_n.getFriendlyStatus() &&
				   esm_f.getFriendlyStatus())
				{
					//cout << esm_f.getFriendly() << " <<< " << esm_n.getFriendly() << endl;

					validateRecord("DIAL" + yampt::sep[0] + esm_f.getFriendly(),
						       esm_n.getFriendly(),
						       yampt::r_type::CELL);
				}

				if(counter_created % 25 == 0)
				{
					cout << "." << flush;
				}
			}
		}
	}

	cout << endl;

	printLog("DIAL");
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
	resetCounters();
	string current_dialog;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");
			esm_n.setFriendly("NAME");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				current_dialog = esm_n.getUnique() + yampt::sep[0] + dialTranslator(esm_n.getFriendly());
			}
			else
			{
				current_dialog = "<NOTFOUND>";
			}
		}
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setUnique("INAM");
			esm_n.setFriendly("NAME");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				validateRecord("INFO" + yampt::sep[0] + current_dialog + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::INFO);
			}
		}
	}
	printLog("INFO");
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setFriendly("BNAM");
			esm_f.setFriendly("BNAM");

			if(esm_n.getFriendlyStatus())
			{
				message_n = makeMessageColl(esm_n.getFriendly());
				message_f = makeMessageColl(esm_f.getFriendly());

				for(size_t k = 0; k < message_n.size(); ++k)
				{
					validateRecord("BNAM" + yampt::sep[0] + message_ptr->at(k),
						       message_n.at(k),
						       yampt::r_type::BNAM);
				}
			}
		}
	}
	printLog("BNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtended()
{
	resetCounters();

	cout << "BNAM in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setUnique("INAM");
			esm_n.setFriendly("BNAM");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				for(size_t j = 0; j < esm_f.getRecColl().size(); ++j)
				{
					esm_f.setRec(j);

					if(esm_f.getRecId() == "INFO")
					{
						esm_f.setUnique("INAM");
						esm_f.setFriendly("BNAM");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus())
						{
							message_n = makeMessageColl(esm_n.getFriendly());
							message_f = makeMessageColl(esm_f.getFriendly());

							if(message_n.size() == message_f.size())
							{
								for(size_t k = 0; k < message_n.size(); ++k)
								{
									//cout << "---" << endl;
									//cout << message_f.at(k) << endl;
									//cout << message_n.at(k) << endl;

									validateRecord("BNAM" + yampt::sep[0] + message_f.at(k),
										       message_n.at(k),
										       yampt::r_type::BNAM);

									if(counter_created % 25 == 0)
									{
										cout << "." << flush;
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	cout << endl;

	printLog("BNAM");
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);

		if(esm_n.getRecId() == "SCPT")
		{
			esm_n.setFriendly("SCTX");
			esm_f.setFriendly("SCTX");

			if(esm_n.getFriendlyStatus())
			{
				message_n = makeMessageColl(esm_n.getFriendly());
				message_f = makeMessageColl(esm_f.getFriendly());

				for(size_t k = 0; k < message_n.size(); ++k)
				{
					validateRecord("SCTX" + yampt::sep[0] + message_ptr->at(k),
						       message_n.at(k),
						       yampt::r_type::SCTX);
				}
			}
		}
	}
	printLog("SCTX");
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtended()
{
	resetCounters();

	cout << "SCTX in progress...";

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);

		if(esm_n.getRecId() == "SCPT")
		{
			esm_n.setUnique("SCHD");
			esm_n.setFriendly("SCTX");

			if(esm_n.getUniqueStatus() &&
			   esm_n.getFriendlyStatus())
			{
				for(size_t j = 0; j < esm_f.getRecColl().size(); ++j)
				{
					esm_f.setRec(j);

					if(esm_f.getRecId() == "SCPT")
					{
						esm_f.setUnique("SCHD");
						esm_f.setFriendly("SCTX");

						if(esm_f.getUnique() == esm_n.getUnique() &&
						   esm_f.getFriendlyStatus())
						{
							message_n = makeMessageColl(esm_n.getFriendly());
							message_f = makeMessageColl(esm_f.getFriendly());

							if(message_n.size() == message_f.size())
							{
								for(size_t k = 0; k < message_n.size(); ++k)
								{
									//cout << "---" << endl;
									//cout << message_f.at(k) << endl;
									//cout << message_n.at(k) << endl;

									validateRecord("SCTX" + yampt::sep[0] + message_f.at(k),
										       message_n.at(k),
										       yampt::r_type::BNAM);

									if(counter_created % 25 == 0)
									{
										cout << "." << flush;
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	cout << endl;

	printLog("SCTX");
}
