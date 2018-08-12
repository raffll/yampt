#include <catch2/catch.hpp>
#include "../src/config.hpp"

using namespace std;

TEST_CASE("Tools::convertStringByteArrayToUInt")
{
    Tools tools;
    REQUIRE(tools.convertStringByteArrayToUInt("DEAD") == 1145128260);
    REQUIRE(tools.convertStringByteArrayToUInt("D\0\0\0") == 68);
    REQUIRE(tools.convertStringByteArrayToUInt("D") == 68);
}

TEST_CASE("Tools::convertUIntToStringByteArray")
{
    Tools tools;
    REQUIRE(tools.convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("Tools::caseInsensitiveStringCmp")
{
    Tools tools;
    REQUIRE(tools.caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(tools.caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("Tools::eraseNullChars")
{
    Tools tools;
    std::string dead;

    dead = "DEAD";
    dead.resize(8);
    REQUIRE(tools.eraseNullChars(dead) == "DEAD");

    dead = "DEAD";
    dead.resize(8);
    dead += "BEEF";
    REQUIRE(tools.eraseNullChars(dead) == "DEAD");
}

TEST_CASE("Tools::eraseCarriageReturnChar")
{
    Tools tools;
    std::string dead = "DEAD\r";
    REQUIRE(tools.eraseCarriageReturnChar(dead) == "DEAD");
}

TEST_CASE("Tools::addDialogTopicsToINFOStrings")
{
    Tools tools;
    yampt::inner_dict_t dict;
    dict.insert({"clanfear", "postrach klanów"});
    std::string text;

    text = "Some text clanfear some text";
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text clanfear some text [postrach klanów]");
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, true) ==
            "Some text clanfear some text [clanfear -> postrach klanów]");

    text = "Some text CLANFEAR some text";
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text CLANFEAR some text [postrach klanów]");
}
