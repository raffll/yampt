#include "Config.hpp"

//----------------------------------------------------------
void Writer::writeDict(const yampt::dict_t &dict, std::string name)
{
    if(getSize(dict) > 0)
    {
        std::ofstream file(name, std::ios::binary);
        for(size_t i = 0; i < dict.size(); ++i)
        {
            for(const auto &elem : dict[i])
            {
                if(i == yampt::rec_type::BNAM || i == yampt::rec_type::SCTX)
                {
                    file << yampt::sep_line << "\r\n"
                         << yampt::sep[1] << elem.first
                         << yampt::sep[2] << "\r\n        " << yampt::sep[0] << elem.second
                         << yampt::sep[3] << "\r\n";
                }
                else
                {
                    file << yampt::sep_line << "\r\n"
                         << yampt::sep[1] << elem.first
                         << yampt::sep[2] << elem.second
                         << yampt::sep[3] << "\r\n";
                }
            }
        }
        std::cout << "--> Writing " << std::to_string(getSize(dict)) <<
                     " records to " << name << "..." << std::endl;
    }
    else
    {
        std::cout << "--> No records to make dictionary!" << std::endl;
    }
}

//----------------------------------------------------------
void Writer::writeText(const std::string &text, std::string name)
{
    std::ofstream file(name, std::ios::binary);
    file << text;
    std::cout << "--> Writing " << name << "..." << std::endl;
}

//----------------------------------------------------------
int Writer::getSize(const yampt::dict_t &dict)
{
    int size = 0;
    for(auto const &elem : dict)
    {
        size += elem.size();
    }
    return size;
}

//----------------------------------------------------------
unsigned int Tools::convertByteArrayToInt(const std::string &str)
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
std::string Tools::convertIntToByteArray(unsigned int x)
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
std::string Tools::eraseNullChars(std::string str)
{
    size_t is_null = str.find('\0');
    if(is_null != std::string::npos)
    {
        str.erase(is_null);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::eraseCarriageReturnChar(std::string str)
{
    if(str.find('\r') != std::string::npos)
    {
        str.erase(str.size() - 1);
    }
    return str;
}

//----------------------------------------------------------
std::string Tools::replaceNonReadableCharWithDot(const std::string &str)
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
