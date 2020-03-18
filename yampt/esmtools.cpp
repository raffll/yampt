#include "esmtools.hpp"

//----------------------------------------------------------
EsmTools::EsmTools(const std::string &path)
    : esm(path)
{

}

//----------------------------------------------------------
std::string EsmTools::dumpFile()
{
    std::string dump;
    if(esm.isLoaded() == true)
    {
        for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
        {
            size_t cur_pos = 16;
            size_t cur_size = 0;
            std::string cur_id;
            std::string cur_text;
            std::string rec;

            esm.setRecordTo(i);
            rec = esm.getRecordContent();
            dump += esm.getRecordId() + " [No. " + std::to_string(i) + "]\r\n";
            while(cur_pos != rec.size())
            {
                cur_id = rec.substr(cur_pos, 4);
                cur_size = tools.convertStringByteArrayToUInt(rec.substr(cur_pos + 4, 4));
                cur_text = rec.substr(cur_pos + 8, cur_size);
                cur_text = tools.replaceNonReadableCharsWithDot(cur_text);
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
    if(esm.isLoaded() == true)
    {
        for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
        {
            esm.setRecordTo(i);
            if(esm.getRecordId() == "INFO")
            {
                esm.setUniqueTo("INAM");
                esm.setFriendlyTo("BNAM");
                scripts_coll.insert({esm.getUniqueText(), make_pair(esm.getFriendlyText(), "")});
            }
            if(esm.getRecordId() == "SCPT")
            {
                esm.setUniqueTo("SCHD");
                esm.setFriendlyTo("SCDT");
                compiled = tools.replaceNonReadableCharsWithDot(esm.getFriendlyWithNull());
                esm.setFriendlyTo("SCTX");
                scripts_coll.insert({esm.getUniqueText(), make_pair(esm.getFriendlyText(), compiled)});
            }
        }
    }
    for(auto const &elem : scripts_coll)
    {
        scripts += elem.first + "\r\n---\r\n";
        scripts += elem.second.first + "\r\n---\r\n";
        scripts += elem.second.second + "\r\n---\r\n";
    }
    return scripts;
}
