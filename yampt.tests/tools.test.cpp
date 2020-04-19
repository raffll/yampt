#include "catch.hpp"
#include "../yampt/config.hpp"

TEST_CASE("Convert string byte array to uint")
{
    std::string text = "DEAD";
    REQUIRE(Tools::convertStringByteArrayToUInt(text) == 1145128260);
    text = "D";
    REQUIRE(Tools::convertStringByteArrayToUInt(text) == 68);
    for (int i = 0; i < 3; ++i)
    {
        text += '\0';
    }
    REQUIRE(text.size() == 4);
    REQUIRE(Tools::convertStringByteArrayToUInt(text) == 68);
}

TEST_CASE("Convert uint to string byte array")
{
    REQUIRE(Tools::convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("Case insensitive string comparison")
{
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("Erase null chars from first found")
{
    std::string text = "DEAD";
    text.resize(8);
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
    text = "DEAD";
    text.resize(8);
    text += "BEEF";
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
}

TEST_CASE("Erase only last \r char")
{
    std::string text = "DEAD\r";
    REQUIRE(Tools::eraseCarriageReturnChar(text) == "DEAD");
    text = "DE\rAD\r";
    REQUIRE(Tools::eraseCarriageReturnChar(text) == "DE\rAD");
}

TEST_CASE("Add dialog topics to INFO strings")
{
    std::string text;
    Tools::single_dict_t dict;
    dict.insert({ "clanfear", "postrach klanów" });

    text = "Some text clanfear some text";
    REQUIRE(Tools::addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text clanfear some text [postrach klanów]");
    REQUIRE(Tools::addDialogTopicsToINFOStrings(dict, text, true) ==
            "Some text clanfear some text [clanfear -> postrach klanów]");

    text = "Some text CLANFEAR some text";
    REQUIRE(Tools::addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text CLANFEAR some text [postrach klanów]");
}
