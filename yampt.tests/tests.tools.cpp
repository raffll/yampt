#include "catch.hpp"
#include "../yampt/tools.hpp"

TEST_CASE("convert string byte array to uint", "[u]")
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

TEST_CASE("convert uint to string byte array", "[u]")
{
    REQUIRE(Tools::convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("case insensitive string comparison", "[u]")
{
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(Tools::caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("erase null chars from first found", "[u]")
{
    std::string text = "DEAD";
    text.resize(8);
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
    text = "DEAD";
    text.resize(8);
    text += "BEEF";
    REQUIRE(Tools::eraseNullChars(text) == "DEAD");
}

TEST_CASE("erase only last \\r char", "[u]")
{
    std::string text = "DEAD\r";
    REQUIRE(Tools::trimCR(text) == "DEAD");
    text = "DE\rAD\r";
    REQUIRE(Tools::trimCR(text) == "DE\rAD");
}

TEST_CASE("add dialog topics to INFO strings", "[u]")
{
    std::string text;
    Tools::Chapter dict;
    dict.insert({ "clanfear", "postrach klanów" });

    text = "some text clanfear some text";
    REQUIRE(Tools::addHyperlinks(dict, text, false) == " [postrach klanów]");
    REQUIRE(Tools::addHyperlinks(dict, text, true) == " [clanfear -> postrach klanów]");

    text = "some text CLANFEAR some text";
    REQUIRE(Tools::addHyperlinks(dict, text, false) == " [postrach klanów]");
}
