#include "tools.hpp"

std::string Tools::log;

const std::vector<std::string> Tools::sep { "^", "<_id>", "</_id>", "<key>", "</key>", "<val>", "</val>", "<rec name=\"", "\"/>" };
const std::vector<std::string> Tools::err { "<err name=\"", "\"/>" };
const std::vector<std::string> Tools::keywords { "messagebox", "choice", "say" };

//----------------------------------------------------------
std::string Tools::readFile(const std::string & path)
{
    std::string content;
    std::ifstream file(path, std::ios::binary);
    if (file)
    {
        addLog("--> Loading \"" + path + "\"...\r\n");
        char buffer[16384];
        size_t size = file.tellg();
        content.reserve(size);
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
void Tools::writeDict(const Dict & dict, const std::string & name)
{
    if (getNumberOfElementsInDict(dict) == 0)
    {
        addLog("--> No records to make dictionary!\r\n");
        return;
    }

    std::ofstream file(name, std::ios::binary);
    for (const auto & chapter : dict)
    {
        const auto & type = chapter.first;
        if (type == Tools::RecType::Annotations)
            continue;

        for (const auto & elem : chapter.second)
        {
            file
                << "<record>\r\n"
                << "\t" << sep[1] << Tools::getTypeName(type) << sep[2] << "\r\n"
                << "\t" << sep[3] << elem.first << sep[4] << "\r\n"
                << "\t" << sep[5] << elem.second << sep[6] << "\r\n";

            if (type == Tools::RecType::INFO)
            {
                auto search = dict.at(Tools::RecType::Annotations).find(elem.first);
                if (search != dict.at(Tools::RecType::Annotations).end())
                {
                    file << "\t" << "<!--" << search->second << "\r\n\t-->" << "\r\n";;
                }
            }

            file << "</record>\r\n";
        }
    }
    addLog("--> Writing " + std::to_string(getNumberOfElementsInDict(dict))
           + " records to \"" + name + "\"...\r\n");
}

//----------------------------------------------------------
void Tools::writeText(const std::string & text, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    addLog("--> Writing \"" + name + "\"...\r\n");
}

//----------------------------------------------------------
void Tools::writeFile(const std::vector<std::string> & rec_coll, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    for (auto & elem : rec_coll)
    {
        file << elem;
    }
    addLog("--> Writing \"" + name + "\"...\r\n");
}

//----------------------------------------------------------
size_t Tools::getNumberOfElementsInDict(const Dict & dict)
{
    size_t size = 0;
    for (const auto & chapter : dict)
    {
        if (chapter.first == Tools::RecType::Annotations)
            continue;

        size += chapter.second.size();
    }
    return size;
}

//----------------------------------------------------------
size_t Tools::convertStringByteArrayToUInt(const std::string & str)
{
    assert(str.size() == 4 || str.size() == 1);

    char buffer[4];
    unsigned char ubuffer[4];
    unsigned int x;
    str.copy(buffer, 4);
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
    if (str.find('\r') != std::string::npos)
    {
        str.erase(str.size() - 1);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::replaceNonReadableCharsWithDot(const std::string & str)
{
    std::string text;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (isprint(str[i]))
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
std::string Tools::addHyperlinks(
    const Chapter & chapter,
    const std::string & val_text,
    bool extended)
{
    std::string result;
    std::string key_text_lc;
    std::string val_text_lc;
    size_t pos;

    val_text_lc = val_text;
    transform(val_text_lc.begin(), val_text_lc.end(),
              val_text_lc.begin(), ::tolower);

    for (const auto & elem : chapter)
    {
        key_text_lc = elem.first;
        transform(key_text_lc.begin(), key_text_lc.end(),
                  key_text_lc.begin(), ::tolower);

        if (key_text_lc == elem.second)
            continue;

        pos = val_text_lc.find(key_text_lc);
        if (pos == std::string::npos)
            continue;

        if (!extended)
        {
            result.insert(result.size(), " [" + elem.second + "]");
        }
        else
        {
            result.insert(result.size(), " [" + elem.first + " -> " + elem.second + "]");
        }
    }
    return result;
}

//----------------------------------------------------------
void Tools::addLog(
    const std::string & entry,
    const bool silent)
{
    if (!silent)
        std::cout << entry;

    log += entry;
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
        { Tools::RecType::SpeakerNAME, {} },
        { Tools::RecType::SpeakerINFO, {} },

        { Tools::RecType::Annotations, {} },
    };
}

//----------------------------------------------------------
std::string Tools::getTypeName(Tools::RecType type)
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

    case Tools::RecType::Wilderness: return "+ Wilderness";
    case Tools::RecType::Region: return "+ Region";

    case Tools::RecType::Glossary: return "Glossary";
    case Tools::RecType::SpeakerNAME: return "SpeakerNAME";
    case Tools::RecType::SpeakerINFO: return "SpeakerINFO";

    case Tools::RecType::Annotations: return "Annotations";
    }
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
