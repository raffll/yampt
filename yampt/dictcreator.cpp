#include "dictcreator.hpp"

//----------------------------------------------------------
DictCreator::DictCreator(
	const std::string & path_n
)
	: esm(path_n)
	, esm_ptr(&esm)
	, message_ptr(&message)
	, mode(Tools::CreatorMode::RAW)
	, add_hyperlinks(false)
{
	dict = Tools::initializeDict();

	if (esm.isLoaded())
		makeDict(true);
}

//----------------------------------------------------------
DictCreator::DictCreator(
	const std::string & path_n,
	const std::string & path_f
)
	: esm(path_n)
	, esm_ext(path_f)
	, esm_ptr(&esm_ext)
	, message_ptr(&message_ext)
	, mode(Tools::CreatorMode::BASE)
	, add_hyperlinks(false)
{
	dict = Tools::initializeDict();

	if (esm.isLoaded() &&
		esm_ext.isLoaded())
	{
		makeDict(isSameOrder());
	}
}

//----------------------------------------------------------
DictCreator::DictCreator(
	const std::string & path_n,
	const DictMerger & merger,
	const Tools::CreatorMode mode,
	const bool add_hyperlinks
)
	: esm(path_n)
	, esm_ptr(&esm)
	, merger(&merger)
	, message_ptr(&message)
	, mode(mode)
	, add_hyperlinks(add_hyperlinks)
{
	dict = Tools::initializeDict();

	if (esm.isLoaded())
		makeDict(true);
}

//----------------------------------------------------------
void DictCreator::makeDict(const bool same_order)
{
	Tools::addLog("-----------------------------------------------\r\n"
		"          Created / Missing / Identical /   All\r\n"
		"-----------------------------------------------\r\n");

	if (same_order)
	{
		makeDictCELL();
		makeDictCELLWilderness();
		makeDictCELLRegion();
		makeDictDIAL();
		makeDictBNAM();
		makeDictSCPT();
	}
	else
	{
		makeDictCELLExtended();
		makeDictCELLWildernessExtended();
		makeDictCELLRegionExtended();
		makeDictDIALExtended();
		makeDictBNAMExtended();
		makeDictSCPTExtended();
	}

	makeDictGMST();
	makeDictFNAM();
	makeDictDESC();
	makeDictTEXT();
	makeDictRNAM();
	makeDictINDX();

	if (add_hyperlinks)
	{
		Tools::addLog("Adding annotations...\r\n");
	}

	makeDictINFO();

	if (!same_order)
		Tools::addLog("--> Check dictionary for \"MISSING\" keyword!\r\n"
			"    Missing CELL and DIAL records needs to be added manually!\r\n");

	Tools::addLog("-----------------------------------------------\r\n");
}

//----------------------------------------------------------
bool DictCreator::isSameOrder()
{
	std::string ids;
	std::string ids_ext;

	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		ids += esm.getRecordId();
	}

	for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
	{
		esm_ext.selectRecord(i);
		ids_ext += esm_ext.getRecordId();
	}

	return ids == ids_ext;
}

//----------------------------------------------------------
void DictCreator::makeDictCELL()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "CELL")
			continue;

		esm.setValue("NAME");
		esm_ptr->selectRecord(i);
		esm_ptr->setValue("NAME");

		if (esm.getValue().exist &&
			esm.getValue().text != "" &&
			esm_ptr->getValue().exist &&
			esm_ptr->getValue().text != "")
		{
			key_text = esm_ptr->getValue().text;
			val_text = esm.getValue().text;
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLWilderness()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");
		esm_ptr->selectRecord(i);
		esm_ptr->setKey("NAME");
		esm_ptr->setValue("STRV");

		if (esm.getKey().text == "sDefaultCellname" &&
			esm.getValue().exist &&
			esm_ptr->getKey().text == "sDefaultCellname" &&
			esm_ptr->getValue().exist)
		{
			key_text = esm_ptr->getValue().text;
			val_text = esm.getValue().text;
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLWildernessExtended()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "GMST")
			continue;

		esm.setKey("NAME");
		esm.setValue("STRV");

		if (esm.getKey().text == "sDefaultCellname" &&
			esm.getValue().exist)
		{
			for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
			{
				esm_ext.selectRecord(k);
				if (esm_ext.getRecordId() != "GMST")
					continue;

				esm_ext.setKey("NAME");
				esm_ext.setValue("STRV");

				if (esm_ext.getKey().text == "sDefaultCellname" &&
					esm_ext.getValue().exist)
				{
					key_text = esm_ext.getValue().text;
					val_text = esm.getValue().text;
					validateRecord();
					break;
				}
			}
			break;
		}
	}
	printLogLine(Tools::RecType::Wilderness);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLRegion()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "REGN")
			continue;

		esm.setValue("FNAM");
		esm_ptr->selectRecord(i);
		esm_ptr->setValue("FNAM");

		if (esm.getValue().exist &&
			esm_ptr->getValue().exist)
		{
			key_text = esm_ptr->getValue().text;
			val_text = esm.getValue().text;
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLRegionExtended()
{
	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "REGN")
			continue;

		esm.setKey("NAME");
		esm.setValue("FNAM");

		if (esm.getKey().exist &&
			esm.getValue().exist)
		{
			for (size_t k = 0; k < esm_ext.getRecords().size(); ++k)
			{
				esm_ext.selectRecord(k);
				if (esm_ext.getRecordId() != "REGN")
					continue;

				esm_ext.setKey("NAME");
				esm_ext.setValue("FNAM");

				if (esm_ext.getKey().text == esm.getKey().text &&
					esm_ext.getValue().exist)
				{
					key_text = esm_ext.getValue().text;
					val_text = esm.getValue().text;
					validateRecord();
					break;
				}
			}
		}
	}
	printLogLine(Tools::RecType::Region);
}

//----------------------------------------------------------
void DictCreator::makeDictGMST()
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
			esm.getKey().text.substr(0, 1) == "s")
		{
			key_text = esm.getKey().text;
			val_text = esm.getValue().text;
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::GMST);
}

//----------------------------------------------------------
void DictCreator::makeDictFNAM()
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
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::FNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictDESC()
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
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::DESC);
}

//----------------------------------------------------------
void DictCreator::makeDictTEXT()
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
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::TEXT);
}

//----------------------------------------------------------
void DictCreator::makeDictRNAM()
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
			validateRecord();
			esm.setNextValue("RNAM");
		}
	}
	printLogLine(Tools::RecType::RNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictINDX()
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
				key_text = esm.getRecordId() + Tools::sep[0] + Tools::getINDX(esm.getKey().content);
				val_text = esm.getValue().text;
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::INDX);
}

//----------------------------------------------------------
void DictCreator::makeDictDIAL()
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
		esm_ptr->selectRecord(i);
		esm_ptr->setKey("DATA");
		esm_ptr->setValue("NAME");

		if (Tools::getDialogType(esm.getKey().content) == "T" &&
			esm.getValue().exist &&
			Tools::getDialogType(esm_ptr->getKey().content) == "T" &&
			esm_ptr->getValue().exist)
		{
			key_text = esm_ptr->getValue().text;
			val_text = esm.getValue().text;
			validateRecord();
		}
	}
	printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void DictCreator::makeDictINFO()
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
				key_prefix = Tools::getDialogType(esm.getKey().content) + Tools::sep[0] +
					translateDialogTopic(esm.getValue().text);
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
				addGenderAnnotations();
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::INFO);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAM()
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
		esm_ptr->selectRecord(i);
		esm_ptr->setKey("INAM");
		esm_ptr->setValue("BNAM");

		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm_ptr->getKey().exist &&
			esm_ptr->getValue().exist)
		{
			message = makeScriptMessages(esm.getValue().text);
			*message_ptr = makeScriptMessages(esm_ptr->getValue().text);

			if (message.size() != message_ptr->size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				key_text = esm_ptr->getKey().text + Tools::sep[0] + message_ptr->at(k);
				val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPT()
{
	resetCounters();
	type = Tools::RecType::SCTX;
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "SCPT")
			continue;

		esm.setKey("SCHD");
		esm.setValue("SCTX");
		esm_ptr->selectRecord(i);
		esm_ptr->setKey("SCHD");
		esm_ptr->setValue("SCTX");

		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm_ptr->getKey().exist &&
			esm_ptr->getValue().exist)
		{
			message = makeScriptMessages(esm.getValue().text);
			*message_ptr = makeScriptMessages(esm_ptr->getValue().text);

			if (message.size() != message_ptr->size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				key_text = esm_ptr->getKey().text + Tools::sep[0] + message_ptr->at(k);
				val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtended()
{
	makeDictCELLExtendedForeignColl();
	makeDictCELLExtendedNativeColl();

	resetCounters();
	type = Tools::RecType::CELL;
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.selectRecord(search->second);
			esm.setValue("NAME");
			esm_ext.selectRecord(patterns_ext[i].pos);
			esm_ext.setValue("NAME");

			if (esm.getValue().exist &&
				esm.getValue().text != "" &&
				esm_ext.getValue().exist &&
				esm_ext.getValue().text != "")
			{
				key_text = esm_ext.getValue().text;
				val_text = esm.getValue().text;
				validateRecord();
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	makeDictCELLExtendedAddMissing();
	printLogLine(Tools::RecType::CELL);
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedForeignColl()
{
	patterns_ext.clear();
	for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
	{
		esm_ext.selectRecord(i);
		if (esm_ext.getRecordId() != "CELL")
			continue;

		esm_ext.setValue("NAME");
		if (esm_ext.getValue().exist &&
			esm_ext.getValue().text != "")
		{
			patterns_ext.push_back({ makeDictCELLExtendedPattern(esm_ext), i, false });
		}
	}
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedNativeColl()
{
	patterns.clear();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "CELL")
			continue;

		esm.setValue("NAME");
		if (esm.getValue().exist &&
			esm.getValue().text != "")
		{
			patterns.insert({ makeDictCELLExtendedPattern(esm), i });
		}
	}
}

//----------------------------------------------------------
std::string DictCreator::makeDictCELLExtendedPattern(EsmReader & esm_cur)
{
	/* pattern is the DATA and combined ids of all objects in a cell */

	std::string pattern;
	esm_cur.setValue("DATA");
	pattern += esm_cur.getValue().content;
	esm_cur.setValue("NAME");
	while (esm_cur.getValue().exist)
	{
		esm_cur.setNextValue("NAME");
		pattern += esm_cur.getValue().content;
	}
	return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictCELLExtendedAddMissing()
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ext.selectRecord(patterns_ext[i].pos);
		esm_ext.setValue("NAME");

		if (esm_ext.getValue().exist &&
			esm_ext.getValue().text != "")
		{
			key_text = esm_ext.getValue().text;
			val_text = Tools::err[0] + "MISSING" + Tools::err[1];
			validateRecord();
			Tools::addLog("Missing CELL: " + esm_ext.getValue().text + "\r\n");
		}
	}
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtended()
{
	makeDictDIALExtendedForeignColl();
	makeDictDIALExtendedNativeColl();

	resetCounters();
	type = Tools::RecType::DIAL;
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search != patterns.end())
		{
			esm.selectRecord(search->second);
			esm.setValue("NAME");
			esm_ext.selectRecord(patterns_ext[i].pos);
			esm_ext.setValue("NAME");

			if (esm.getValue().exist &&
				esm_ext.getValue().exist)
			{
				key_text = esm_ext.getValue().text;
				val_text = esm.getValue().text;
				validateRecord();
			}
		}
		else
		{
			patterns_ext[i].missing = true;
			counter_missing++;
		}
	}
	makeDictDIALExtendedAddMissing();
	printLogLine(Tools::RecType::DIAL);
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedForeignColl()
{
	patterns_ext.clear();
	for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
	{
		esm_ext.selectRecord(i);
		if (esm_ext.getRecordId() != "DIAL")
			continue;

		esm_ext.setKey("DATA");
		if (Tools::getDialogType(esm_ext.getKey().content) == "T")
		{
			patterns_ext.push_back({ makeDictDIALExtendedPattern(esm_ext, i), i, false });
		}
	}
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedNativeColl()
{
	patterns.clear();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "DIAL")
			continue;

		esm.setKey("DATA");
		if (Tools::getDialogType(esm.getKey().content) == "T")
		{
			patterns.insert({ makeDictDIALExtendedPattern(esm, i), i });
		}
	}
}

//----------------------------------------------------------
std::string DictCreator::makeDictDIALExtendedPattern(EsmReader & esm_cur, size_t i)
{
	/* pattern is the INAM and SCVR from next INFO record */

	std::string pattern;
	esm_cur.selectRecord(i + 1);
	esm_cur.setValue("INAM");
	pattern += esm_cur.getValue().content;
	esm_cur.setValue("SCVR");
	pattern += esm_cur.getValue().content;
	return pattern;
}

//----------------------------------------------------------
void DictCreator::makeDictDIALExtendedAddMissing()
{
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		if (!patterns_ext[i].missing)
			continue;

		esm_ext.selectRecord(patterns_ext[i].pos);
		esm_ext.setValue("NAME");

		if (!esm_ext.getValue().exist)
			continue;

		key_text = esm_ext.getValue().text;
		val_text = Tools::err[0] + "MISSING" + Tools::err[1];
		validateRecord();
		Tools::addLog("Missing DIAL: " + esm_ext.getValue().text + "\r\n");
	}
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtended()
{
	makeDictBNAMExtendedForeignColl();
	makeDictBNAMExtendedNativeColl();

	resetCounters();
	type = Tools::RecType::BNAM;
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search == patterns.end())
			continue;

		esm.selectRecord(search->second);
		esm.setKey("INAM");
		esm.setValue("BNAM");
		esm_ext.selectRecord(patterns_ext[i].pos);
		esm_ext.setKey("INAM");
		esm_ext.setValue("BNAM");

		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm_ext.getKey().exist &&
			esm_ext.getValue().exist)
		{
			message = makeScriptMessages(esm.getValue().text);
			message_ext = makeScriptMessages(esm_ext.getValue().text);

			if (message.size() != message_ext.size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				key_text = esm_ext.getKey().text + Tools::sep[0] + message_ext.at(k);
				val_text = esm_ext.getKey().text + Tools::sep[0] + message.at(k);
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::BNAM);
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtendedForeignColl()
{
	patterns_ext.clear();
	for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
	{
		esm_ext.selectRecord(i);
		if (esm_ext.getRecordId() != "INFO")
			continue;

		esm_ext.setKey("INAM");
		if (!esm_ext.getKey().exist)
			continue;

		patterns_ext.push_back({ esm_ext.getKey().text, i, false });
	}
}

//----------------------------------------------------------
void DictCreator::makeDictBNAMExtendedNativeColl()
{
	patterns.clear();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "INFO")
			continue;

		esm.setKey("INAM");
		if (!esm.getKey().exist)
			continue;

		patterns.insert({ esm.getKey().text, i });
	}
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtended()
{
	makeDictSCPTExtendedForeignColl();
	makeDictSCPTExtendedNativeColl();

	resetCounters();
	type = Tools::RecType::SCTX;
	for (size_t i = 0; i < patterns_ext.size(); ++i)
	{
		auto search = patterns.find(patterns_ext[i].str);
		if (search == patterns.end())
			continue;

		esm.selectRecord(search->second);
		esm.setKey("SCHD");
		esm.setValue("SCTX");
		esm_ext.selectRecord(patterns_ext[i].pos);
		esm_ext.setKey("SCHD");
		esm_ext.setValue("SCTX");

		if (esm.getKey().exist &&
			esm.getValue().exist &&
			esm_ext.getKey().exist &&
			esm_ext.getValue().exist)
		{
			message = makeScriptMessages(esm.getValue().text);
			message_ext = makeScriptMessages(esm_ext.getValue().text);

			if (message.size() != message_ext.size())
				continue;

			for (size_t k = 0; k < message.size(); ++k)
			{
				key_text = esm_ext.getKey().text + Tools::sep[0] + message_ext.at(k);
				val_text = esm.getKey().text + Tools::sep[0] + message.at(k);
				validateRecord();
			}
		}
	}
	printLogLine(Tools::RecType::SCTX);
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtendedForeignColl()
{
	patterns_ext.clear();
	for (size_t i = 0; i < esm_ext.getRecords().size(); ++i)
	{
		esm_ext.selectRecord(i);
		if (esm_ext.getRecordId() != "SCPT")
			continue;

		esm_ext.setKey("SCHD");
		if (!esm_ext.getKey().exist)
			continue;

		patterns_ext.push_back({ esm_ext.getKey().text, i , false });
	}
}

//----------------------------------------------------------
void DictCreator::makeDictSCPTExtendedNativeColl()
{
	patterns.clear();
	for (size_t i = 0; i < esm.getRecords().size(); ++i)
	{
		esm.selectRecord(i);
		if (esm.getRecordId() != "SCPT")
			continue;

		esm.setKey("SCHD");
		if (!esm.getKey().exist)
			continue;

		patterns.insert({ esm.getKey().text, i });
	}
}

//----------------------------------------------------------
void DictCreator::resetCounters()
{
	counter_created = 0;
	counter_missing = 0;
	counter_doubled = 0;
	counter_identical = 0;
	counter_all = 0;
}

//----------------------------------------------------------
void DictCreator::validateRecord()
{
	counter_all++;

	if (mode == Tools::CreatorMode::RAW ||
		mode == Tools::CreatorMode::BASE)
	{
		insertRecordToDict();
	}

	if (mode == Tools::CreatorMode::ALL)
	{
		validateRecordForModeALL();
	}

	if (mode == Tools::CreatorMode::NOTFOUND)
	{
		validateRecordForModeNOT();
	}

	if (mode == Tools::CreatorMode::CHANGED)
	{
		validateRecordForModeCHANGED();
	}
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeALL()
{
	if (type == Tools::RecType::CELL ||
		type == Tools::RecType::DIAL ||
		type == Tools::RecType::BNAM ||
		type == Tools::RecType::SCTX)
	{
		auto search = merger->getDict().at(type).find(key_text);
		if (search != merger->getDict().at(type).end())
		{
			val_text = search->second;
		}
	}

	insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeNOT()
{
	auto search = merger->getDict().at(type).find(key_text);
	if (search != merger->getDict().at(type).end())
		return;

	addAnnotations();
	insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::validateRecordForModeCHANGED()
{
	if (type == Tools::RecType::CELL ||
		type == Tools::RecType::DIAL ||
		type == Tools::RecType::BNAM ||
		type == Tools::RecType::SCTX)
		return;

	auto search = merger->getDict().at(type).find(key_text);
	if (search == merger->getDict().at(type).end())
		return;

	if (search->second == val_text)
		return;

	addAnnotations();
	insertRecordToDict();
}

//----------------------------------------------------------
void DictCreator::addAnnotations()
{
	if (type == Tools::RecType::INFO && add_hyperlinks)
	{
		std::string annotations = "Hyperlinks:" + Tools::addHyperlinks(
			merger->getDict().at(Tools::RecType::DIAL),
			val_text,
			true);

		annotations += "\r\n\t     Glossary:" + Tools::addHyperlinks(
			merger->getDict().at(Tools::RecType::Glossary),
			val_text,
			true);

		auto search = dict.at(Tools::RecType::Gender).find(key_text);
		if (search != dict.at(Tools::RecType::Gender).end())
		{
			annotations += "\r\n\t     Speaker: " + search->second;
		}

		dict.at(Tools::RecType::Annotations).insert({ key_text, annotations });
	}
}

//----------------------------------------------------------
void DictCreator::insertRecordToDict()
{
	if (dict.at(type).insert({ key_text, val_text }).second)
	{
		counter_created++;
	}
	else
	{
		auto search = dict.at(type).find(key_text);
		if (val_text != search->second)
		{
			key_text =
				key_text + Tools::err[0] + "DOUBLED_" +
				std::to_string(counter_doubled) + Tools::err[1];

			dict.at(type).insert({ key_text, val_text });
			counter_doubled++;
			counter_created++;
			Tools::addLog("Doubled " + Tools::getTypeName(type) + ": " + key_text + "\r\n");
		}
		else
		{
			counter_identical++;
		}
	}
}

//----------------------------------------------------------
void DictCreator::printLogLine(const Tools::RecType type)
{
	std::string type_name = Tools::getTypeName(type);
	type_name.resize(12, ' ');

	std::ostringstream ss;
	ss << type_name << std::setw(5) << std::to_string(counter_created) << " / ";

	if (type == Tools::RecType::CELL ||
		type == Tools::RecType::DIAL)
	{
		ss << std::setw(7) << std::to_string(counter_missing) << " / ";
	}
	else
	{
		ss << std::setw(7) << "-" << " / ";
	}

	ss << std::setw(8) << std::to_string(counter_identical) << " / ";
	ss << std::setw(6) << std::to_string(counter_all) << "\r\n";

	Tools::addLog(ss.str());
}

//----------------------------------------------------------
std::string DictCreator::translateDialogTopic(std::string to_translate)
{
	if (mode == Tools::CreatorMode::ALL ||
		mode == Tools::CreatorMode::NOTFOUND ||
		mode == Tools::CreatorMode::CHANGED)
	{
		auto search = merger->getDict().at(Tools::RecType::DIAL).find(to_translate);
		if (search != merger->getDict().at(Tools::RecType::DIAL).end())
		{
			return search->second;
		}
	}
	return to_translate;
}

//----------------------------------------------------------
std::vector<std::string> DictCreator::makeScriptMessages(const std::string & script_text)
{
	std::vector<std::string> messages;
	std::string line;
	std::string line_lc;
	std::istringstream ss(script_text);

	while (std::getline(ss, line))
	{
		line = Tools::trimCR(line);
		line_lc = line;
		transform(line_lc.begin(), line_lc.end(),
			line_lc.begin(), ::tolower);

		size_t keyword_pos;
		std::set<size_t> keyword_pos_coll;

		for (const auto & keyword : Tools::keywords)
		{
			keyword_pos = line_lc.find(keyword);
			keyword_pos_coll.insert(keyword_pos);
		}

		keyword_pos = *keyword_pos_coll.begin();

		if (keyword_pos != std::string::npos &&
			line.rfind(";", keyword_pos) == std::string::npos &&
			line.find("\"", keyword_pos) != std::string::npos)
		{
			messages.push_back(line);
		}
	}
	return messages;
}

//----------------------------------------------------------
void DictCreator::addGenderAnnotations()
{
	if (mode == Tools::CreatorMode::NOTFOUND || mode == Tools::CreatorMode::CHANGED)
	{
		std::string gender = "N\\A";
		esm.setValue("ONAM");
		auto npc = esm.getValue().content;
		for (size_t k = 0; k < esm.getRecords().size(); ++k)
		{
			esm.selectRecord(k);
			if (esm.getRecordId() != "NPC_")
				continue;

			esm.setValue("NAME");
			if (esm.getValue().content != npc)
				continue;

			esm.setValue("FLAG");
			if ((Tools::convertStringByteArrayToUInt(esm.getValue().content) & 0x0001) != 0)
				gender = "Female";
			else
				gender = "Male";
		}
		dict.at(Tools::RecType::Gender).insert({ key_text, gender });
	}
}
