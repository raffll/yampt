#include "catch.hpp"
#include "../yampt/tools.hpp"

TEST_CASE("Convert string byte array to uint", "[u]")
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

TEST_CASE("Convert uint to string byte array", "[u]")
{
    REQUIRE(Tools::convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("Case insensitive string comparison", "[u]")
{
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("Erase null chars from first found", "[u]")
{
    std::string text = "DEAD";
    text.resize(8);
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
    text = "DEAD";
    text.resize(8);
    text += "BEEF";
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
}

TEST_CASE("Erase only last \r char", "[u]")
{
    std::string text = "DEAD\r";
    REQUIRE(Tools::trimCR(text) == "DEAD");
    text = "DE\rAD\r";
    REQUIRE(Tools::trimCR(text) == "DE\rAD");
}

TEST_CASE("Add dialog topics to INFO strings", "[u]")
{
    std::string text;
    Tools::Chapter dict;
    dict.insert({ "clanfear", "postrach klanów" });

    text = "Some text clanfear some text";
    REQUIRE(Tools::addHyperlinks(dict, text, false) == " [postrach klanów]");
    REQUIRE(Tools::addHyperlinks(dict, text, true) == " [clanfear -> postrach klanów]");

    text = "Some text CLANFEAR some text";
    REQUIRE(Tools::addHyperlinks(dict, text, false) == " [postrach klanów]");
}