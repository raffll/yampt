#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#define private public

#include "../src/config.hpp"
#include "../src/esmreader.hpp"

TEST_CASE("EsmReader::setName()")
{
    EsmReader esmreader;
    esmreader.is_loaded = true;
    esmreader.setName("/path/to/Morrowind.esm");
    REQUIRE(esmreader.name_full == "Morrowind.esm");
    REQUIRE(esmreader.name_prefix == "Morrowind");
    REQUIRE(esmreader.name_suffix == ".esm");
}

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
    std::string dead = "DEAD\0\0\0\0";
    REQUIRE(tools.eraseNullChars(dead) == "DEAD");
    dead = "DEAD\0\0BEEF\0\0";
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
    std::string text = "Sone text clanfear some text";
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Sone text clanfear some text [postrach klanów]");
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Sone text CLANFEAR some text [postrach klanów]");
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, true) ==
            "Sone text clanfear some text [clanfear -> postrach klanów]");
}
