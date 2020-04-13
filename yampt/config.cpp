#include "config.hpp"

//----------------------------------------------------------
std::string Tools::readFile(const std::string & path)
{
    std::string content;
    std::ifstream file(path, std::ios::binary);
    if (file)
    {
        std::cout << "--> Loading \"" + path + "\"..." << std::endl;
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
        std::cout << "--> Error loading \"" + path + "\" (wrong path)!" << std::endl;
    }
    return content;
}

//----------------------------------------------------------
void Tools::writeDict(const yampt::dict_t & dict, const std::string & name)
{
    if (getNumberOfElementsInDict(dict) > 0)
    {
        std::ofstream file(name, std::ios::binary);
        for (size_t i = 0; i < dict.size(); ++i)
        {
            for (const auto & elem : dict[i])
            {
                file << "<record>\r\n"
                    << "\t" << yampt::sep[1] << yampt::type_name[i] << yampt::sep[2] << "\r\n"
                    << "\t" << yampt::sep[3] << elem.first << yampt::sep[4] << "\r\n"
                    << "\t" << yampt::sep[5] << elem.second << yampt::sep[6] << "\r\n"
                    << "</record>\r\n";
            }
        }
        std::cout << "--> Writing " << std::to_string(getNumberOfElementsInDict(dict)) <<
            " records to \"" << name << "\"..." << std::endl;
    }
    else
    {
        std::cout << "--> No records to make dictionary!" << std::endl;
    }
}

//----------------------------------------------------------
void Tools::writeText(const std::string & text, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    std::cout << "--> Writing \"" << name << "\"..." << std::endl;
}

//----------------------------------------------------------
void Tools::writeFile(const std::vector<std::string> & rec_coll, const std::string & name)
{
    std::ofstream file(name, std::ios::binary);
    for (auto & elem : rec_coll)
    {
        file << elem;
    }
    std::cout << "--> Writing \"" << name << "\"..." << std::endl;
}

//----------------------------------------------------------
int Tools::getNumberOfElementsInDict(const yampt::dict_t & dict)
{
    int size = 0;
    for (auto const & elem : dict)
    {
        size += elem.size();
    }
    return size;
}

//----------------------------------------------------------
unsigned int Tools::convertStringByteArrayToUInt(const std::string & str)
{
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
    else if (str.size() == 1)
    {
        return x = ubuffer[0];
    }
    else
    {
        return x = 0;
    }
}

//----------------------------------------------------------
std::string Tools::convertUIntToStringByteArray(const unsigned int x)
{
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
    if (lhs == rhs)
    {
        return true;
    }
    else
    {
        return false;
    }
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
std::string Tools::eraseCarriageReturnChar(std::string str)
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
std::string Tools::addDialogTopicsToINFOStrings(
    yampt::single_dict_t dict,
    const std::string & friendly_text,
    bool extended)
{
    std::string unique_text_lc;
    std::string new_friendly;
    std::string new_friendly_lc;
    size_t pos;

    new_friendly = friendly_text;
    new_friendly_lc = friendly_text;
    transform(new_friendly_lc.begin(), new_friendly_lc.end(),
              new_friendly_lc.begin(), ::tolower);

    for (const auto & elem : dict)
    {
        unique_text_lc = elem.first;
        transform(unique_text_lc.begin(), unique_text_lc.end(),
                  unique_text_lc.begin(), ::tolower);

        if (unique_text_lc != elem.second)
        {
            pos = new_friendly_lc.find(unique_text_lc);
            if (pos != std::string::npos)
            {
                if (extended == false)
                {
                    new_friendly.insert(new_friendly.size(), " [" + elem.second + "]");
                }
                else
                {
                    new_friendly.insert(new_friendly.size(), " [" + elem.first + " -> " + elem.second + "]");
                }
            }
        }
    }
    return new_friendly;
}

//----------------------------------------------------------
void Tools::addLog(const std::string log)
{
    this->log += log + "\r\n";
}

//----------------------------------------------------------
void Tools::clearLog()
{
    log.clear();
}
