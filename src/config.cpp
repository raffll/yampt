#include "config.hpp"

//----------------------------------------------------------
void Tools::writeDict(const yampt::dict_t &dict, const std::string &name)
{
    if(getNumberOfElementsInDict(dict) > 0)
    {
        std::ofstream file(name, std::ios::binary);
        for(size_t i = 0; i < dict.size(); ++i)
        {
            for(const auto &elem : dict[i])
            {
                file << "<record>\r\n"
                     << "\t<id>" << yampt::type_name[i] << "</id>\r\n"
                     << "\t<key>" << elem.first << "</key>\r\n"
                     << "\t<val>" << elem.second << "</val>\r\n"
                     << "</record>\r\n";
            }
        }
        std::cout << "--> Writing " << std::to_string(getNumberOfElementsInDict(dict)) <<
                     " records to " << name << "..." << std::endl;
    }
    else
    {
        std::cout << "--> No records to make dictionary!" << std::endl;
    }
}

//----------------------------------------------------------
void Tools::writeText(const std::string &text, const std::string &name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    std::cout << "--> Writing " << name << "..." << std::endl;
}

//----------------------------------------------------------
void Tools::writeFile(const std::vector<std::string> &rec_coll, const std::string &name)
{
    std::ofstream file(name, std::ios::binary);
    for(auto &elem : rec_coll)
    {
        file << elem;
    }
    std::cout << "--> Writing " << name << "..." << std::endl;
}

//----------------------------------------------------------
int Tools::getNumberOfElementsInDict(const yampt::dict_t &dict)
{
    int size = 0;
    for(auto const &elem : dict)
    {
        size += elem.size();
    }
    return size;
}

//----------------------------------------------------------
unsigned int Tools::convertStringByteArrayToUInt(const std::string &str)
{
    char buffer[4];
    unsigned char ubuffer[4];
    unsigned int x;
    str.copy(buffer, 4);
    for(int i = 0; i < 4; i++)
    {
        ubuffer[i] = buffer[i];
    }
    if(str.size() == 4)
    {
        return x = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
    }
    else if(str.size() == 1)
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
    std::copy(static_cast<const char*>(static_cast<const void*>(&x)),
              static_cast<const char*>(static_cast<const void*>(&x)) + sizeof x,
              bytes);
    for(int i = 0; i < 4; i++)
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
    if(lhs == rhs)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------
std::string Tools::eraseNullChars(std::string &str)
{
    size_t is_null = str.find('\0');
    if(is_null != std::string::npos)
    {
        str.erase(is_null);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::eraseCarriageReturnChar(std::string &str)
{
    if(str.find('\r') != std::string::npos)
    {
        str.erase(str.size() - 1);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::replaceNonReadableCharsWithDot(const std::string &str)
{
    std::string text;
    for(size_t i = 0; i < str.size(); ++i)
    {
        if(isprint(str[i]))
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
