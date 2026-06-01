#include "tools.hpp"

std::string tools_t::log1;
std::string tools_t::log2;
bool tools_t::error_flag = false;

const std::vector<std::string> tools_t::keywords { "messagebox", "choice", "say" };


bool tools_t::Chapter::insert(const RecordEntry & entry)
{
    auto it = index.find(entry.key_text);
    if (it != index.end())
        return false;

    index[entry.key_text] = records.size();
    records.push_back(entry);
    return true;
}


tools_t::RecordEntry * tools_t::Chapter::find(const std::string & id)
{
    auto it = index.find(id);
    if (it == index.end())
        return nullptr;
    return &records[it->second];
}


const tools_t::RecordEntry * tools_t::Chapter::find(const std::string & id) const
{
    auto it = index.find(id);
    if (it == index.end())
        return nullptr;
    return &records[it->second];
}


std::string tools_t::readFile(const std::string & path)
{
    std::string content;
    std::ifstream file(path, std::ios::binary);
    if (file)
    {
        addLog("--> Loading \"" + path + "\"...\r\n");
        char buffer[16384];
        file.seekg(0, std::ios::end);
        content.reserve(static_cast<size_t>(file.tellg()));
        file.seekg(0, std::ios::beg);
        std::streamsize chars_read;
        while (file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
        {
            content.append(buffer, chars_read);
        }
    }
    else
    {
        addLog("--> Error loading \"" + path + "\" (wrong path)!\r\n");
    }
    return content;
}


void tools_t::writeText(const std::string & text, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    addLog("--> Writing \"" + name + "\"...\r\n");
}


void tools_t::writeFile(
    const std::vector<Record> & records,
    const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    for (const auto & record : records)
    {
        file << record.content;
    }
    addLog("--> Writing \"" + name + "\"...\r\n");
}


void tools_t::createFile(
    const std::vector<Record> & records,
    const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    for (size_t i = 0; i < records.size(); ++i)
    {
        const auto & record = records.at(i);
        if (!record.modified)
            continue;
        else
            file << record.content;
    }
    addLog("--> Writing \"" + name + "\"...\r\n");
}


size_t tools_t::getNumberOfElementsInDict(const dict_t & dict)
{
    size_t size = 0;
    for (const auto & chapter : dict)
    {
        size += chapter.second.size();
    }
    return size;
}


size_t tools_t::convertStringByteArrayToUInt(const std::string & str)
{
    assert(str.size() == 4 || str.size() == 1);

    char buffer[4] = {};
    unsigned char ubuffer[4];
    unsigned int x;
    str.copy(buffer, str.size());
    for (int i = 0; i < 4; i++)
    {
        ubuffer[i] = buffer[i];
    }

    if (str.size() == 4)
    {
        return x = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
    }

    if (str.size() == 1)
    {
        return x = ubuffer[0];
    }

    return std::string::npos;
}


std::string tools_t::convertUIntToStringByteArray(const size_t size)
{
    auto x = static_cast<const unsigned>(size);

    char bytes[4];
    std::string str;
    std::copy(static_cast<const char *>(static_cast<const void *>(&x)),
              static_cast<const char *>(static_cast<const void *>(&x)) + sizeof x,
              bytes);
    for (int i = 0; i < 4; i++)
    {
        str.push_back(bytes[i]);
    }
    return str;
}


bool tools_t::caseInsensitiveStringCmp(std::string lhs, std::string rhs)
{
    transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
    transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
    return lhs == rhs;
}


std::string tools_t::eraseNullChars(std::string str)
{
    size_t is_null = str.find('\0');
    if (is_null != std::string::npos)
    {
        str.erase(is_null);
    }
    return str;
}


std::string tools_t::trimCR(std::string str)
{
    if (!str.empty() && str.back() == '\r')
    {
        str.pop_back();
    }
    return str;
}


std::string tools_t::replaceNonReadableCharsWithDot(const std::string & str)
{
    std::string text;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (std::isprint(static_cast<unsigned char>(str[i])))
        {
            text += str[i];
        }
        else
        {
            text += ".";
        }
    }
    return text;
}


void tools_t::addLog(
    const std::string & entry,
    const bool silent)
{
    if (entry.find("--> Error") == 0)
    {
        error_flag = true;
    }

    if (!silent)
    {
        std::cout << entry;
        log1 += entry;
    }
    else
        log2 += entry;
}


void tools_t::resetLog()
{
    log1.clear();
    log2.clear();
    error_flag = false;
}


tools_t::dict_t tools_t::initializeDict()
{
    return
    {
        { tools_t::rec_type_t::CELL, {} },
        { tools_t::rec_type_t::DIAL, {} },
        { tools_t::rec_type_t::INDX, {} },
        { tools_t::rec_type_t::RNAM, {} },
        { tools_t::rec_type_t::DESC, {} },
        { tools_t::rec_type_t::GMST, {} },
        { tools_t::rec_type_t::FNAM, {} },
        { tools_t::rec_type_t::INFO, {} },
        { tools_t::rec_type_t::TEXT, {} },
        { tools_t::rec_type_t::BNAM, {} },
        { tools_t::rec_type_t::SCTX, {} },
        { tools_t::rec_type_t::Glossary, {} },
        { tools_t::rec_type_t::NPC_FLAG, {} },
    };
}


std::string tools_t::type2Str(tools_t::rec_type_t type)
{
    switch (type)
    {
        case tools_t::rec_type_t::CELL: return "CELL";
        case tools_t::rec_type_t::DIAL: return "DIAL";
        case tools_t::rec_type_t::INDX: return "INDX";
        case tools_t::rec_type_t::RNAM: return "RNAM";
        case tools_t::rec_type_t::DESC: return "DESC";
        case tools_t::rec_type_t::GMST: return "GMST";
        case tools_t::rec_type_t::FNAM: return "FNAM";
        case tools_t::rec_type_t::INFO: return "INFO";
        case tools_t::rec_type_t::TEXT: return "TEXT";
        case tools_t::rec_type_t::BNAM: return "BNAM";
        case tools_t::rec_type_t::SCTX: return "SCTX";

        case tools_t::rec_type_t::PGRD: return "PGRD";
        case tools_t::rec_type_t::ANAM: return "ANAM";
        case tools_t::rec_type_t::SCVR: return "SCVR";
        case tools_t::rec_type_t::DNAM: return "DNAM";
        case tools_t::rec_type_t::CNDT: return "CNDT";
        case tools_t::rec_type_t::GMDT: return "GMDT";

        case tools_t::rec_type_t::Default: return "+ Default";
        case tools_t::rec_type_t::REGN: return "+ REGN";

        case tools_t::rec_type_t::Glossary: return "Glossary";
        case tools_t::rec_type_t::NPC_FLAG: return "NPC_FLAG";

        default: return "N/A";
    }
}


tools_t::rec_type_t tools_t::str2Type(const std::string & str)
{
    std::map<std::string, tools_t::rec_type_t> str2type
    {
        { "CELL", tools_t::rec_type_t::CELL },
        { "DIAL", tools_t::rec_type_t::DIAL },
        { "INDX", tools_t::rec_type_t::INDX },
        { "RNAM", tools_t::rec_type_t::RNAM },
        { "DESC", tools_t::rec_type_t::DESC },
        { "GMST", tools_t::rec_type_t::GMST },
        { "FNAM", tools_t::rec_type_t::FNAM },
        { "INFO", tools_t::rec_type_t::INFO },
        { "TEXT", tools_t::rec_type_t::TEXT },
        { "BNAM", tools_t::rec_type_t::BNAM },
        { "SCTX", tools_t::rec_type_t::SCTX },
        { "Glossary", tools_t::rec_type_t::Glossary },
        { "NPC_FLAG", tools_t::rec_type_t::NPC_FLAG },
    };

    auto search = str2type.find(str);
    if (search != str2type.end())
        return search->second;
    else
        return tools_t::rec_type_t::Unknown;
}


std::string tools_t::getDialogType(const std::string & content)
{
    static const std::vector<std::string> dialog_type { "T", "V", "G", "P", "J" };
    size_t type = tools_t::convertStringByteArrayToUInt(content.substr(0, 1));
    return dialog_type.at(type);
}


std::string tools_t::getINDX(const std::string & content)
{
    size_t indx = tools_t::convertStringByteArrayToUInt(content);
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(3) << indx;
    return ss.str();
}


bool tools_t::isFNAM(const std::string & rec_id)
{
    return (rec_id == "ACTI" || rec_id == "ALCH" ||
            rec_id == "APPA" || rec_id == "ARMO" ||
            rec_id == "BOOK" || rec_id == "BSGN" ||
            rec_id == "CLAS" || rec_id == "CLOT" ||
            rec_id == "CONT" || rec_id == "CREA" ||
            rec_id == "DOOR" || rec_id == "FACT" ||
            rec_id == "INGR" || rec_id == "LIGH" ||
            rec_id == "LOCK" || rec_id == "MISC" ||
            rec_id == "NPC_" || rec_id == "PROB" ||
            rec_id == "RACE" || rec_id == "REGN" ||
            rec_id == "REPA" || rec_id == "SKIL" ||
            rec_id == "SPEL" || rec_id == "WEAP");
}
