#include "esmconverter.hpp"
#include "scriptparser.hpp"
#include "esmtools.hpp"

//----------------------------------------------------------
EsmConverter::EsmConverter(
	const std::string & path,
	const DictMerger & merger,
	const bool add_hyperlinks,
	const bool safe,
	const std::string & file_suffix,
	const Tools::Encoding encoding
)
	: esm(path)
	, merger(&merger)
	, add_hyperlinks(add_hyperlinks)
	, file_suffix(file_suffix)
{
	if (encoding == Tools::Encoding::WINDOWS_1250)
	{
		esm_encoding = esm.detectEncoding();
		if (esm_encoding == Tools::Encoding::WINDOWS_1250)
		{
			this->add_hyperlinks = false;
		}
	}

	if (esm.isLoaded())
		convertEsm(safe);
}

//----------------------------------------------------------
void EsmConverter::convertEsm(const bool safe)
{
	Tools::addLog("------------------------------------------------\r\n"
		"      Converted / Identical / Unchanged /    All\r\n"
		"------------------------------------------------\r\n");

	convertMAST();
	convertCELL();
	convertPGRD();
	convertANAM();
	convertSCVR();
	convertDNAM();
	convertCNDT();
	convertDIAL();
	convertBNAM();
	convertSCPT();

	if (!safe)
	{
		convertGMST();
		convertFNAM();
		convertDESC();
		convertTEXT();
		convertRNAM();
		convertINDX();

		if (add_hyperlinks)
		{
			Tools::addLog("Adding hyperlinks...\r\n");
		}

		convertINFO();
	}

	Tools::addLog("------------------------------------------------\r\n");
}

//----------------------------------------------------------
void EsmConverter::convertMAST()
{
	std::string master_prefix;
	std::string master_suffix;
	resetCounters();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "TES3")
			continue;

		esm.setValue("MAST");
		while (esm.getValue().exist)
		{
			master_prefix = esm.getValue().text.substr(0, esm.getValue().text.find_last_of("."));
			master_suffix = esm.getValue().text.substr(esm.getValue().text.rfind("."));
			new_text = master_prefix + file_suffix + master_suffix + '\0';
			convertRecordContent();
			esm.setNextValue("MAST");
		}
	}
}

//----------------------------------------------------------
void EsmConverter::convertCELL()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "CELL")
			continue;

		esm.setValue("NAME");
		if (esm.getValue().exist &&
			esm.getValue().text != "")
		{
			key_text = esm.getValue().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			/* null terminated, can't be empty */
			new_text += '\0';
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void EsmConverter::convertPGRD()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "PGRD")
			continue;

		esm.setValue("NAME");
		if (esm.getValue().exist &&
			esm.getValue().text != "")
		{
			key_text = esm.getValue().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			new_text += '\0';
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::PGRD);
}

//----------------------------------------------------------
void EsmConverter::convertANAM()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "INFO")
			continue;

		esm.setValue("ANAM");
		if (esm.getValue().exist &&
			esm.getValue().text != "")
		{
			key_text = esm.getValue().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			new_text += '\0';
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::ANAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCVR()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "INFO")
			continue;

		esm.setValue("SCVR");
		while (esm.getValue().exist)
		{
			/* possible exceptions */
			if (esm.getValue().text.substr(1, 1) == "B")
			{
				key_text = esm.getValue().text.substr(5);
				val_text = esm.getValue().text.substr(5);
				setNewText();

				if (ready)
				{
					/* not null terminated */
					new_text = esm.getValue().text.substr(0, 5) + new_text;
					convertRecordContent();
				}
			}
			esm.setNextValue("SCVR");
		}
	}
	printLogLine(Tools::RecType::SCVR);
}

//----------------------------------------------------------
void EsmConverter::convertDNAM()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() == "CELL" ||
			esm.getRecordId() == "NPC_")
		{
			esm.setValue("DNAM");
			while (esm.getValue().exist)
			{
				key_text = esm.getValue().text;
				val_text = esm.getValue().text;
				setNewText();

				if (ready)
				{
					new_text += '\0';
					convertRecordContent();
				}

				esm.setNextValue("DNAM");
			}
		}
	}
	printLogLine(Tools::RecType::DNAM);
}

//----------------------------------------------------------
void EsmConverter::convertCNDT()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "NPC_")
			continue;

		esm.setValue("CNDT");
		while (esm.getValue().exist)
		{
			key_text = esm.getValue().text;
			val_text = esm.getValue().text;
			setNewText();

			if (ready)
			{
				new_text += '\0';
				convertRecordContent();
			}

			esm.setNextValue("CNDT");
		}
	}
	printLogLine(Tools::RecType::CNDT);
}

//----------------------------------------------------------
void EsmConverter::convertGMST()
{
	resetCounters();
	type = Tools::RecType::GMST;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");

		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm.getKey().text.substr(0, 1) == "s") /* possible exception */
		{
			key_text = esm.getKey().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			/* null terminated only if empty */
			addNullTerminatorIfEmpty();
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void EsmConverter::convertFNAM()
{
	resetCounters();
	type = Tools::RecType::FNAM;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() == "ACTI" ||
			esm.getRecordId() == "ALCH" ||
			esm.getRecordId() == "APPA" ||
			esm.getRecordId() == "ARMO" ||
			esm.getRecordId() == "BOOK" ||
			esm.getRecordId() == "BSGN" ||
			esm.getRecordId() == "CLAS" ||
			esm.getRecordId() == "CLOT" ||
			esm.getRecordId() == "CONT" ||
			esm.getRecordId() == "CREA" ||
			esm.getRecordId() == "DOOR" ||
			esm.getRecordId() == "FACT" ||
			esm.getRecordId() == "INGR" ||
			esm.getRecordId() == "LIGH" ||
			esm.getRecordId() == "LOCK" ||
			esm.getRecordId() == "MISC" ||
			esm.getRecordId() == "NPC_" ||
			esm.getRecordId() == "PROB" ||
			esm.getRecordId() == "RACE" ||
			esm.getRecordId() == "REGN" ||
			esm.getRecordId() == "REPA" ||
			esm.getRecordId() == "SKIL" ||
			esm.getRecordId() == "SPEL" ||
			esm.getRecordId() == "WEAP")
		{
			esm.setKey("NAME");
			esm.setValue("FNAM");

			if (esm.getKey().exist &&
				esm.getValue().exist &&
				esm.getKey().text != "player")
			{
				key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
				val_text = esm.getValue().text;
				setNewText();

				if (!ready)
					continue;

				/* null terminated, don't exist if empty */
				new_text += '\0';
				convertRecordContent();
			}
		}
	}
	printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void EsmConverter::convertDESC()
{
	resetCounters();
	type = Tools::RecType::DESC;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() == "BSGN" ||
			esm.getRecordId() == "CLAS" ||
			esm.getRecordId() == "RACE")
		{
			esm.setKey("NAME");
			esm.setValue("DESC");

			if (esm.getKey().exist &&
				esm.getValue().exist)
			{
				key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
				val_text = esm.getValue().text;
				setNewText();

				if (!ready)
					continue;

				if (esm.getRecordId() == "BSGN")
				{
					/* null terminated, don't exist if empty */
					new_text += '\0';
					convertRecordContent();
				}

				if (esm.getRecordId() == "CLAS" ||
					esm.getRecordId() == "RACE")
				{
					/* not null terminated, don't exist if empty */
					addNullTerminatorIfEmpty();
					convertRecordContent();
				}
			}
		}
	}
	printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void EsmConverter::convertTEXT()
{
	resetCounters();
	type = Tools::RecType::TEXT;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "BOOK")
			continue;

		esm.setKey("NAME");
		esm.setValue("TEXT");

		if (esm.getKey().exist &&
			esm.getValue().exist)
		{
			key_text = esm.getKey().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			/* not null terminated, don't exist if empty */
			addNullTerminatorIfEmpty();
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void EsmConverter::convertRNAM()
{
	resetCounters();
	type = Tools::RecType::RNAM;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "FACT")
			continue;

		esm.setKey("NAME");
		esm.setValue("RNAM");

		if (!esm.getKey().exist)
			continue;

		while (esm.getValue().exist)
		{
			key_text = esm.getKey().text + Tools::sep[0] + std::to_string(esm.getValue().counter);
			val_text = esm.getValue().text;
			setNewText();

			if (ready)
			{
				/* null terminated up to 32 */
				new_text.resize(32);
				convertRecordContent();
			}
			esm.setNextValue("RNAM");
		}
	}
	printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void EsmConverter::convertINDX()
{
	resetCounters();
	type = Tools::RecType::INDX;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() == "SKIL" ||
			esm.getRecordId() == "MGEF")
		{
			esm.setKey("INDX");
			esm.setValue("DESC");

			if (esm.getKey().exist &&
				esm.getValue().exist)
			{
				key_text = esm.getRecordId() + Tools::sep[0] + esm.getKey().text;
				val_text = esm.getValue().text;
				setNewText();

				if (!ready)
					continue;

				/* not null terminated, don't exist if empty */
				addNullTerminatorIfEmpty();
				convertRecordContent();
			}
		}
	}
	printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void EsmConverter::convertDIAL()
{
	resetCounters();
	type = Tools::RecType::DIAL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "DIAL")
			continue;

		esm.setKey("DATA");
		esm.setValue("NAME");

		if (Tools::getDialogType(esm.getKey().content) == "T" &&
			esm.getValue().exist)
		{
			key_text = esm.getValue().text;
			val_text = esm.getValue().text;
			setNewText();

			if (!ready)
				continue;

			/* null terminated */
			new_text += '\0';
			convertRecordContent();
		}
	}
	printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void EsmConverter::convertINFO()
{
	std::string key_prefix;
	resetCounters();
	type = Tools::RecType::INFO;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() == "DIAL")
		{
			esm.setKey("DATA");
			esm.setValue("NAME");

			if (esm.getKey().exist &&
				esm.getValue().exist)
			{
				key_prefix = Tools::getDialogType(esm.getKey().content) + Tools::sep[0] + esm.getValue().text;
			}
		}

		if (esm.getRecordId() == "INFO")
		{
			esm.setKey("INAM");
			esm.setValue("NAME");

			if (esm.getKey().exist &&
				esm.getValue().exist)
			{
				key_text = key_prefix + Tools::sep[0] + esm.getKey().text;
				val_text = esm.getValue().text;
				setNewText(key_prefix);

				if (ready)
				{
					/* not null terminated, don't exist if empty */
					addNullTerminatorIfEmpty();
					convertRecordContent();
				}
			}
		}
	}
	printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void EsmConverter::convertBNAM()
{
	resetCounters();
	type = Tools::RecType::BNAM;

	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "INFO")
			continue;

		esm.setKey("INAM");
		esm.setValue("BNAM");

		if (esm.getKey().exist &&
			esm.getValue().exist)
		{
			key_text = esm.getKey().text;
			val_text = esm.getValue().text;

			const auto & script_name = key_text;
			const auto & file_name = getNameFull();
			const auto & old_script = val_text;

			counter_all++;
			ScriptParser parser(
				type,
				*merger,
				script_name,
				file_name,
				old_script);

			new_text = parser.getNewScript();
			checkIfIdentical();

			if (!ready)
				continue;

			convertRecordContent();
		}
	}

	Tools::addLog("---\r\n", true);
	printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void EsmConverter::convertSCPT()
{
	std::string old_schd;
	resetCounters();
	type = Tools::RecType::SCTX;

	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "SCPT")
			continue;

		esm.setKey("SCHD");
		esm.setValue("SCDT");
		if (esm.getValue().exist)
		{
			old_schd = esm.getValue().content;
		}
		else
		{
			old_schd.clear();
		}

		esm.setValue("SCTX");
		if (esm.getKey().exist &&
			esm.getValue().exist)
		{
			key_text = esm.getKey().text;
			val_text = esm.getValue().text;

			const auto & script_name = key_text;
			const auto & file_name = getNameFull();
			const auto & old_script = val_text;

			counter_all++;
			ScriptParser parser(
				type,
				*merger,
				script_name,
				file_name,
				old_script,
				old_schd);

			new_text = parser.getNewScript();
			checkIfIdentical();

			if (!ready)
				continue;

			convertRecordContent();

			{
				/* compiled script data */
				esm.setValue("SCDT");
				new_text = parser.getNewSCHD();
				convertRecordContent();
			}

			{
				/* compiled script data size in script name */
				esm.setValue("SCHD");
				new_text = esm.getValue().content;
				new_text.erase(44, 4);
				new_text.insert(44, Tools::convertUIntToStringByteArray(parser.getNewSCHD().size()));
				convertRecordContent();
			}
		}
	}

	Tools::addLog("---\r\n", true);
	printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void EsmConverter::resetCounters()
{
	counter_converted = 0;
	counter_identical = 0;
	counter_unchanged = 0;
	counter_all = 0;
	counter_added = 0;
}

//----------------------------------------------------------
void EsmConverter::setNewText(const std::string & prefix)
{
	counter_all++;
	new_text.clear();
	auto search = merger->getDict().at(type).find(key_text);
	if (search != merger->getDict().at(type).end())
	{
		new_text = search->second;
		checkIfIdentical();
	}
	else if (
		type == Tools::RecType::INFO &&
		add_hyperlinks &&
		prefix.substr(0, 1) != "V")
	{
		new_text = val_text + Tools::addHyperlinks(merger->getDict().at(Tools::RecType::DIAL),
			val_text,
			false);

		checkIfIdentical();

		if (new_text.size() > 1024)
		{
			new_text.resize(1024);
		}
	}
	else
	{
		ready = false;
		counter_unchanged++;
	}
}

//----------------------------------------------------------
void EsmConverter::checkIfIdentical()
{
	if (new_text != val_text)
	{
		ready = true;
		counter_converted++;
	}
	else
	{
		ready = false;
		counter_identical++;
	}
}

//----------------------------------------------------------
void EsmConverter::addNullTerminatorIfEmpty()
{
	if (new_text.empty())
		new_text = '\0';
}

//----------------------------------------------------------
void EsmConverter::convertRecordContent()
{
	size_t rec_size;
	std::string rec_content = esm.getRecordContent();
	rec_content.erase(esm.getValue().pos + 8, esm.getValue().size);
	rec_content.insert(esm.getValue().pos + 8, new_text);
	rec_content.erase(esm.getValue().pos + 4, 4);
	rec_content.insert(
		esm.getValue().pos + 4,
		Tools::convertUIntToStringByteArray(new_text.size()));
	rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, Tools::convertUIntToStringByteArray(rec_size));
	esm.replaceRecord(rec_content);
}

//----------------------------------------------------------
void EsmConverter::printLogLine(const Tools::RecType type)
{
	std::ostringstream ss;
	ss
		<< Tools::getTypeName(type) << " "
		<< std::setw(10) << std::to_string(counter_converted) << " / "
		<< std::setw(9) << std::to_string(counter_identical) << " / "
		<< std::setw(9) << std::to_string(counter_unchanged) << " / "
		<< std::setw(6) << std::to_string(counter_all) << std::endl;

	Tools::addLog(ss.str());
}
