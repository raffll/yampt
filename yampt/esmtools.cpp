#include "esmtools.hpp"

//----------------------------------------------------------
EsmTools::EsmTools(const std::string & path)
    : esm(path)
{

}

//----------------------------------------------------------
std::string EsmTools::dumpFile()
{
    std::string dump;
    if (esm.isLoaded() == true)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            size_t cur_pos = 16;
            size_t cur_size = 0;
            std::string cur_id;
            std::string cur_text;
            std::string rec;

            esm.selectRecord(i);
            rec = esm.getRecordContent();
            dump += esm.getRecordId() + " [No. " + std::to_string(i) + "]\r\n";
            while (cur_pos != rec.size())
            {
                cur_id = rec.substr(cur_pos, 4);
                cur_size = Tools::convertStringByteArrayToUInt(rec.substr(cur_pos + 4, 4));
                cur_text = rec.substr(cur_pos + 8, cur_size);
                cur_text = Tools::replaceNonReadableCharsWithDot(cur_text);
                dump += "    " + cur_id + " " + std::to_string(cur_size) + " " + cur_text + "\r\n";
                cur_pos += 8 + cur_size;
            }
        }
    }
    return dump;
}

//----------------------------------------------------------
std::string EsmTools::makeScriptList()
{
    std::string scripts;
    std::string compiled;
    std::map<std::string, std::pair<std::string, std::string>> scripts_coll;
    if (esm.isLoaded() == true)
    {
        for (size_t i = 0; i < esm.getRecords().size(); ++i)
        {
            esm.selectRecord(i);
            if (esm.getRecordId() == "INFO")
            {
                esm.setKey("INAM");
                esm.setValue("BNAM");
                scripts_coll.insert({ esm.getKey().text, make_pair(esm.getValue().text, "") });
            }
            if (esm.getRecordId() == "SCPT")
            {
                esm.setKey("SCHD");
                esm.setValue("SCDT");
                compiled = esm.getValue().content;
                esm.setValue("SCTX");
                scripts_coll.insert({ esm.getKey().text, make_pair(esm.getValue().text, compiled) });
            }
        }
    }
    for (auto const & elem : scripts_coll)
    {
        scripts += elem.first + "\r\n---\r\n";
        scripts += elem.second.first + "\r\n---\r\n";
        scripts += elem.second.second + "\r\n---\r\n";
    }
    return scripts;
}
