#include "DictCreator.hpp"

using namespace std;

//----------------------------------------------------------
DictCreator::DictCreator(string path_n)
{
	esm_ptr = &esm_n;
	message_ptr = &message_n;

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

	esm_n.readFile(path_n);
	esm_f.readFile(path_f);

	if(esm_n.getStatus() == true && esm_f.getStatus() == true)
	{
		status = true;
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(string path_n, DictMerger &merger, bool no_duplicates)
{
	this->merger = &merger;
	this->no_duplicates = no_duplicates;

	esm_ptr = &esm_n;
	message_ptr = &message_n;

	esm_n.readFile(path_n);

	if(esm_n.getStatus() == true && merger.getStatus() == true)
	{
		status = true;
		with_dict = true;
	}
}

//----------------------------------------------------------
void DictCreator::makeDict()
{
	if(status == true)
	{
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
	}
}

//----------------------------------------------------------
void DictCreator::compareEsm()
{
	if(status == true)
	{
		if(esm_n.getRecColl().size() != esm_f.getRecColl().size())
		{
			cout << "--> They are not the same master files!\r\n";
			status = false;
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
				status = false;
			}
			else
			{
				status = true;
			}
		}
	}
}

//----------------------------------------------------------
void DictCreator::insertRecord(const string &pri_text, const string &sec_text, yampt::r_type type, bool extra)
{
	if(no_duplicates == true)
	{
		auto search = merger->getDict()[type].find(pri_text);
		if(search == merger->getDict()[type].end())
		{
			if(dict[type].insert({pri_text, sec_text}).second == true)
			{
				if(extra == true)
				{
					counter_cell++;
				}
				else
				{
					counter++;
				}
			}
		}
	}
	else if(with_dict == true &&
		(type == yampt::r_type::CELL ||
		 type == yampt::r_type::DIAL ||
		 type == yampt::r_type::BNAM ||
		 type == yampt::r_type::SCTX))
	{
		auto search = merger->getDict()[type].find(pri_text);
		if(search != merger->getDict()[type].end())
		{
			if(dict[type].insert({pri_text, search->second}).second == true)
			{
				if(extra == true)
				{
					counter_cell++;
				}
				else
				{
					counter++;
				}
			}
		}
		else
		{
			if(dict[type].insert({pri_text, sec_text}).second == true)
			{
				if(extra == true)
				{
					counter_cell++;
				}
				else
				{
					counter++;
				}
			}
		}
	}
	else
	{
		if(dict[type].insert({pri_text, sec_text}).second == true)
		{
			if(extra == true)
			{
				counter_cell++;
			}
			else
			{
				counter++;
			}
		}
	}
}

//----------------------------------------------------------
string DictCreator::dialTranslator(string to_translate)
{
	if(with_dict == true)
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
void DictCreator::makeScriptText()
{
	if(status == true)
	{
		string dial;
		counter = 0;

		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "SCPT")
			{
				esm_n.setUnique("SCHD");
				esm_n.setFriendly("SCTX");

				if(esm_n.getFriendlyStatus() == true)
				{
					script_text += yampt::line + "\r\n" +
						       "SCTX " + esm_n.getUnique() + "\r\n" +
						       yampt::line + "\r\n" +
						       esm_n.getFriendly() + "\r\n";
					counter++;
				}
			}
		}
		cout << "    --> SCTX text count: " << to_string(counter) + "\r\n";

		counter = 0;

		for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
		{
			esm_n.setRec(i);
			if(esm_n.getRecId() == "DIAL")
			{
				esm_n.setUnique("NAME");
				dial = esm_n.getUnique();
			}
			if(esm_n.getRecId() == "INFO")
			{
				esm_n.setUnique("INAM");
				esm_n.setFriendly("BNAM");

				if(esm_n.getFriendlyStatus() == true)
				{
					script_text += yampt::line + "\r\n" +
						       "BNAM " + dial + " " + esm_n.getUnique() + "\r\n" +
						       yampt::line + "\r\n" +
						       esm_n.getFriendly() + "\r\n";
					counter++;
				}
			}
		}
		cout << "    --> BNAM text count: " << to_string(counter) + "\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
	counter = 0;
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
				insertRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
					     esm_n.getFriendly(),
					     yampt::r_type::CELL);
			}
		}
	}
	cout << "    --> CELL records created: " << to_string(counter) + "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
{
	counter = 0;
	counter_cell = 0;
	for(size_t i = 0; i < esm_n.getRecColl().size(); ++i)
	{
		esm_n.setRec(i);
		esm_f.setRec(i);
		if(esm_n.getRecId() == "GMST")
		{
			esm_n.setUnique("NAME");
			esm_n.setFriendly("STRV");
			esm_f.setFriendly("STRV");

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
			{
				insertRecord("GMST" + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::GMST);
			}

			if(esm_n.getUnique() == "sDefaultCellname")
			{
				insertRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
					     esm_n.getFriendly(),
					     yampt::r_type::CELL,
					     true);
			}
		}
	}
	cout << "    --> GMST records created: " << to_string(counter) + "\r\n";
	if(counter_cell > 0)
	{
		cout << "        + extra " + to_string(counter_cell) + " CELL records\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
{
	counter = 0;
	counter_cell = 0;
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

			if(esm_n.getUniqueStatus() == true && esm_n.getFriendlyStatus() == true)
			{
				insertRecord("FNAM" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::FNAM);
			}

			if(esm_n.getRecId() == "REGN")
			{
				insertRecord("CELL" + yampt::sep[0] + esm_ptr->getFriendly(),
					     esm_n.getFriendly(),
					     yampt::r_type::CELL,
					     true);
			}
		}
	}
	cout << "    --> FNAM records created: " << to_string(counter) << "\r\n";
	if(counter_cell > 0)
	{
		cout << "        + extra " + to_string(counter_cell) + " CELL records\r\n";
	}
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
{
	counter = 0;
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
				insertRecord("DESC" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::DESC);
			}
		}
	}
	cout << "    --> DESC records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
{
	counter = 0;
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
				insertRecord("TEXT" + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::TEXT);
			}
		}
	}
	cout << "    --> TEXT records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
{
	counter = 0;
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
					insertRecord("RNAM" + yampt::sep[0] + esm_n.getUnique() + yampt::sep[0] + to_string(esm_n.getFriendlyCounter()),
						     esm_n.getFriendly(),
						     yampt::r_type::RNAM);

					esm_n.setFriendly("RNAM", NEXT);
					esm_f.setFriendly("RNAM", NEXT);
				}
			}
		}
	}
	cout << "    --> RNAM records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
{
	counter = 0;
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
				insertRecord("INDX" + yampt::sep[0] + esm_n.getRecId() + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::INDX);
			}
		}
	}
	cout << "    --> INDX records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
{
	counter = 0;
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
				insertRecord("DIAL" + yampt::sep[0] + esm_ptr->getFriendly(),
					     esm_n.getFriendly(),
					     yampt::r_type::DIAL);
			}
		}
	}
	cout << "    --> DIAL records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
{
	counter = 0;
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
				insertRecord("INFO" + yampt::sep[0] + current_dialog + yampt::sep[0] + esm_n.getUnique(),
					     esm_n.getFriendly(),
					     yampt::r_type::INFO);
			}
		}
	}
	cout << "    --> INFO records created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
{
	counter = 0;
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
					insertRecord("BNAM" + yampt::sep[0] + message_ptr->at(k),
						     "\r\n        " + yampt::sep[0] + message_n.at(k),
						     yampt::r_type::BNAM);
				}
			}
		}
	}
	cout << "    --> BNAM script lines created: " << to_string(counter) << "\r\n";
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
	counter = 0;
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
					insertRecord("SCTX" + yampt::sep[0] + message_ptr->at(k),
						     "\r\n        " + yampt::sep[0] + message_n.at(k),
						     yampt::r_type::SCTX);
				}
			}
		}
	}
	cout << "    --> SCTX script lines created: " << to_string(counter) << "\r\n";
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

		for(auto const &elem : key_message)
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
