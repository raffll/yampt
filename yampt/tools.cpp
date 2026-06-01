#include "tools.hpp"

std::string Tools::log1;
std::string Tools::log2;
bool Tools::error_flag = false;

const std::vector<std::string> Tools::keywords { "messagebox", "choice", "say" };

//----------------------------------------------------------
bool Tools::Chapter::insert(const RecordEntry & entry)
{
    auto it = index.find(entry.key_text);
    if (it != index.end())
        return false;

    index[entry.key_text] = records.size();
    records.push_back(entry);
    return true;
}

//----------------------------------------------------------
Tools::RecordEntry * Tools::Chapter::find(const std::string & id)
{
    auto it = index.find(id);
    if (it == index.end())
        return nullptr;
    return &records[it->second];
}

//----------------------------------------------------------
const Tools::RecordEntry * Tools::Chapter::find(const std::string & id) const
{
    auto it = index.find(id);
    if (it == index.end())
        return nullptr;
    return &records[it->second];
}

//----------------------------------------------------------
std::string Tools::readFile(const std::string & path)
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

//----------------------------------------------------------
void Tools::writeText(const std::string & text, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    addLog("--> Writing \"" + name + "\"...\r\n");
}

//----------------------------------------------------------
void Tools::writeFile(
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

//----------------------------------------------------------
void Tools::createFile(
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

//----------------------------------------------------------
size_t Tools::getNumberOfElementsInDict(const Dict & dict)
{
    size_t size = 0;
    for (const auto & chapter : dict)
    {
        size += chapter.second.size();
    }
    return size;
}

//----------------------------------------------------------
size_t Tools::convertStringByteArrayToUInt(const std::string & str)
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

//----------------------------------------------------------
std::string Tools::convertUIntToStringByteArray(const size_t size)
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

//----------------------------------------------------------
bool Tools::caseInsensitiveStringCmp(std::string lhs, std::string rhs)
{
    transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
    transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
    return lhs == rhs;
}

//----------------------------------------------------------
std::string Tools::eraseNullChars(std::string str)
{
    size_t is_null = str.find('\0');
    if (is_null != std::string::npos)
    {
        str.erase(is_null);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::trimCR(std::string str)
{
    if (!str.empty() && str.back() == '\r')
    {
        str.pop_back();
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::replaceNonReadableCharsWithDot(const std::string & str)
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

//----------------------------------------------------------
void Tools::addLog(
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

//----------------------------------------------------------
void Tools::resetLog()
{
    log1.clear();
    log2.clear();
    error_flag = false;
}

//----------------------------------------------------------
Tools::Dict Tools::initializeDict()
{
    return
    {
        { Tools::RecType::CELL, {} },
        { Tools::RecType::DIAL, {} },
        { Tools::RecType::INDX, {} },
        { Tools::RecType::RNAM, {} },
        { Tools::RecType::DESC, {} },
        { Tools::RecType::GMST, {} },
        { Tools::RecType::FNAM, {} },
        { Tools::RecType::INFO, {} },
        { Tools::RecType::TEXT, {} },
        { Tools::RecType::BNAM, {} },
        { Tools::RecType::SCTX, {} },
        { Tools::RecType::Glossary, {} },
        { Tools::RecType::NPC_FLAG, {} },
    };
}

//----------------------------------------------------------
std::string Tools::type2Str(Tools::RecType type)
{
    switch (type)
    {
        case Tools::RecType::CELL: return "CELL";
        case Tools::RecType::DIAL: return "DIAL";
        case Tools::RecType::INDX: return "INDX";
        case Tools::RecType::RNAM: return "RNAM";
        case Tools::RecType::DESC: return "DESC";
        case Tools::RecType::GMST: return "GMST";
        case Tools::RecType::FNAM: return "FNAM";
        case Tools::RecType::INFO: return "INFO";
        case Tools::RecType::TEXT: return "TEXT";
        case Tools::RecType::BNAM: return "BNAM";
        case Tools::RecType::SCTX: return "SCTX";

        case Tools::RecType::PGRD: return "PGRD";
        case Tools::RecType::ANAM: return "ANAM";
        case Tools::RecType::SCVR: return "SCVR";
        case Tools::RecType::DNAM: return "DNAM";
        case Tools::RecType::CNDT: return "CNDT";
        case Tools::RecType::GMDT: return "GMDT";

        case Tools::RecType::Default: return "+ Default";
        case Tools::RecType::REGN: return "+ REGN";

        case Tools::RecType::Glossary: return "Glossary";
        case Tools::RecType::NPC_FLAG: return "NPC_FLAG";

        default: return "N/A";
    }
}

//----------------------------------------------------------
Tools::RecType Tools::str2Type(const std::string & str)
{
    std::map<std::string, Tools::RecType> str2type
    {
        { "CELL", Tools::RecType::CELL },
        { "DIAL", Tools::RecType::DIAL },
        { "INDX", Tools::RecType::INDX },
        { "RNAM", Tools::RecType::RNAM },
        { "DESC", Tools::RecType::DESC },
        { "GMST", Tools::RecType::GMST },
        { "FNAM", Tools::RecType::FNAM },
        { "INFO", Tools::RecType::INFO },
        { "TEXT", Tools::RecType::TEXT },
        { "BNAM", Tools::RecType::BNAM },
        { "SCTX", Tools::RecType::SCTX },
        { "Glossary", Tools::RecType::Glossary },
        { "NPC_FLAG", Tools::RecType::NPC_FLAG },
    };

    auto search = str2type.find(str);
    if (search != str2type.end())
        return search->second;
    else
        return Tools::RecType::Unknown;
}

//----------------------------------------------------------
std::string Tools::getDialogType(const std::string & content)
{
    static const std::vector<std::string> dialog_type { "T", "V", "G", "P", "J" };
    size_t type = Tools::convertStringByteArrayToUInt(content.substr(0, 1));
    return dialog_type.at(type);
}

//----------------------------------------------------------
std::string Tools::getINDX(const std::string & content)
{
    size_t indx = Tools::convertStringByteArrayToUInt(content);
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(3) << indx;
    return ss.str();
}

//----------------------------------------------------------
bool Tools::isFNAM(const std::string & rec_id)
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
