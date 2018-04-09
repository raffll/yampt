#include "esmtools.hpp"

//----------------------------------------------------------
EsmTools::EsmTools(const std::string &path)
{
    esm.readFile(path);
}

//----------------------------------------------------------
std::string EsmTools::dumpFile()
{
    std::string dump;
    if(esm.getStatus() == true)
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
            dump += esm.getRecordId() + "\r\n";
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
    if(esm.getStatus() == true)
    {
        for(size_t i = 0; i < esm.getRecordColl().size(); ++i)
        {
            esm.setRecordTo(i);
            if(esm.getRecordId() == "SCPT")
            {
                esm.setUniqueTo("SCHD");
                scripts += esm.getUniqueText() + "\r\n";
                scripts += "---\r\n";
                esm.setFirstFriendlyTo("SCTX");
                scripts += esm.getFriendlyText() + "\r\n";
                scripts += "---\r\n";
                esm.setFirstFriendlyTo("SCDT", false);
                scripts += tools.replaceNonReadableCharsWithDot(esm.getFriendlyText()) + "\r\n";
                scripts += "---\r\n";
            }
        }
    }
    return scripts;
}
