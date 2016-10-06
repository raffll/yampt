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
		status = compareEsm();
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
	}
}

//----------------------------------------------------------
void DictCreator::makeDict()
{
	if(status == true)
	{
		printLog("", true);
		makeDictCELL();
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
		cout << endl;
	}
}

//----------------------------------------------------------
void DictCreator::printLog(string id, bool header)
{
	if(header == true)
	{
		cout << endl;
		cout << "          CREATED / DOUBLED /    ALL" << endl;
		cout << "    --------------------------------" << endl;
	}
	else
	{
		cout << "    " << id << " "
		     << setw(8) << to_string(counter[0]) << " / "
		     << setw(7) << to_string(counter[1]) << " / "
		     << setw(6) << to_string(counter[2]) << endl;

		if(id == "GMST" || id == "FNAM")
		{
			cout << "     + CELL"
			     << setw(6) << to_string(counter_extra[0]) << " / "
			     << setw(7) << to_string(counter_extra[1]) << " / "
			     << setw(6) << to_string(counter_extra[2]) << endl;
		}
	}
}

//----------------------------------------------------------
bool DictCreator::compareEsm()
{
	if(esm_n.getRecColl().size() != esm_f.getRecColl().size())
	{
		cout << "--> They are not the same master files!\r\n";
		return false;
	}
	else
	{
		string esm_n_compare;
		string esm_f_compare;

		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n_compare += esm_n.getRecId();
		}

		for(size_t i = 0; i < esm_f.getRecColl().size(); ++i)
		{
			esm_f_compare += esm_f.getRecId();
		}

		if(esm_n_compare != esm_f_compare)
		{
			cout << "--> They are not the same master files!\r\n";
			return false;
		}
		else
		{
			return true;
		}
	}
}

//----------------------------------------------------------
void DictCreator::resetCounters()
{
	counter = {};
	counter_extra = {};
}

//----------------------------------------------------------
void DictCreator::validateRecord(const string &unique_key, const string &friendly, yampt::r_type type, bool extra)
{
	if(extra == true)
	{
		counter_extra[2]++;
	}
	else
	{
		counter[2]++;
	}

	if(mode == yampt::ins_mode::RAW || mode == yampt::ins_mode::BASE)
	{
		insertRecord(unique_key, friendly, type, extra);
	}

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
				insertRecord(unique_key, search->second, type, extra);
			}
			else
			{
				insertRecord(unique_key, friendly, type, extra);
			}
		}
		else
		{
			insertRecord(unique_key, friendly, type, extra);
		}
	}

	if(mode == yampt::ins_mode::NOTFOUND)
	{
		auto search = merger->getDict()[type].find(unique_key);
		if(search == merger->getDict()[type].end())
		{
			insertRecord(unique_key, friendly, type, extra);
		}
	}

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
					insertRecord(unique_key, friendly, type, extra);
				}
			}
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const string &unique_key, const string &friendly, yampt::r_type type, bool extra)
{
	if(dict[type].insert({unique_key, friendly}).second == true)
	{
		if(extra == true)
		{
			counter_extra[0]++;
		}
		else
		{
			counter[0]++;
		}
	}
	else
	{
		if(extra == true)
		{
			counter_extra[1]++;
		}
		else
		{
			counter[1]++;
		}
	}
}

//----------------------------------------------------------
string DictCreator::dialTranslator(string to_translate)
{
	if(mode == yampt::ins_mode::ALL || mode == yampt::ins_mode::NOTFOUND || mode == yampt::ins_mode::CHANGED)
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
	bool s_found;
	string s_line;
	string s_line_lc;
	size_t s_pos;

	istringstream ss(script_text);

	while(getline(ss, s_line))
	{
		s_found = false;
		eraseCarriageReturnChar(s_line);
		s_line_lc = s_line;
		transform(s_line_lc.begin(), s_line_lc.end(),
			  s_line_lc.begin(), ::tolower);

		for(auto const &elem : yampt::key_message)
		{
			if(s_found == false)
			{
				s_pos = s_line_lc.find(elem);
				if(s_pos != string::npos && s_line.rfind(";", s_pos) == string::npos)
				{
					message.push_back(s_line);
					s_found = true;
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
		esm_f.setRec(i);
		if(esm_n.getRecId() == "CELL")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("NAME");
			esm_f.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true)
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
void DictCreator::makeDictGMST()
{
	resetCounters();

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");
			esm_f.setFriendly("STRV");

			if(esm_n.getUniqueStatus() == true &&
			   esm_n.getFriendlyStatus() == true &&
			   esm_n.getUnique().substr(0, 1) == "s")
			{
				validateRecord("GMST" + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::GMST);

                                if(esm_n.getUnique() == "sDefaultCellname")
                                {
                                        validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
                                                       esm_n.getFriendly(),
					               yampt::r_type::CELL,
					               true);
                                }
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
		esm_f.setRec(i);
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
			esm_f.setFriendly("FNAM");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true && esm_n.getUnique() != "player")
			{
				validateRecord("FNAM" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					       esm_n.getFriendly(),
					       yampt::r_type::FNAM);

                                if(esm_n.getRecId() == "REGN")
                                {
                                        validateRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
                                                       esm_n.getFriendly(),
                                                       yampt::r_type::CELL,
                                                       true);
                                }
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
		esm_f.setRec(i);
		if(esm_n.getRecId() == "BSGN" ||
		   esm_n.getRecId() == "CLAS" ||
		   esm_n.getRecId() == "RACE")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("DESC");
			esm_f.setFriendly("DESC");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
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
		esm_f.setRec(i);
		if(esm_n.getRecId() == "BOOK")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("TEXT");
			esm_f.setFriendly("TEXT");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
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
		esm_f.setRec(i);
		if(esm_n.getRecId() == "FACT")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("RNAM");
			esm_f.setFriendly("RNAM");

			if(esm_n.getUniqueStatus() == true)
			{
				while(esm_n.getFriendlyStatus() == true)
				{
					validateRecord("RNAM" + yampt::sep[0] + esm_n.getUnique() + yampt::sep[0] + to_string(esm_n.getFriendlyCounter()),
						       esm_n.getFriendly(),
						       yampt::r_type::RNAM);

					esm_n.setFriendly("RNAM", true);
					esm_f.setFriendly("RNAM", true);
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
		esm_f.setRec(i);
		if(esm_n.getRecId() == "SKIL" ||
		   esm_n.getRecId() == "MGEF")
		{
			esm_n.setUnique("INDX");
			esm_n.setFriendly("DESC");
			esm_f.setFriendly("DESC");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
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

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true && esm_n.getUnique() == "T")
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
void DictCreator::makeDictINFO()
{
	resetCounters();
	string current_dialog;

	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "DIAL")
		{
			esm_n.setUnique("DATA");
			esm_n.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
			{
				current_dialog = esm_n.getUnique() + yampt::sep[0] + dialTranslator(esm_n.getFriendly());
			}
			else
			{
				current_dialog = "<NotFound>";
			}
		}
		if(esm_n.getRecId() == "INFO")
		{
			esm_n.setUnique("INAM");
			esm_n.setFriendly("NAME");
			esm_f.setFriendly("NAME");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
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

			if(esm_n.getFriendlyStatus() == true)
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

			if(esm_n.getFriendlyStatus() == true)
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
