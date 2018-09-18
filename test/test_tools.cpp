#include <catch2/catch.hpp>
#include "../src/config.hpp"

using namespace std;

static Tools tools;
static string text;

TEST_CASE("Convert string byte array to uint")
{
    REQUIRE(tools.convertStringByteArrayToUInt("DEAD") == 1145128260);
    text = "D";
    REQUIRE(tools.convertStringByteArrayToUInt(text) == 68);
    for(int i = 0; i < 3; ++i)
    {
        text += '\0';
    }
    REQUIRE(text.size() == 4);
    REQUIRE(tools.convertStringByteArrayToUInt(text) == 68);
}

TEST_CASE("Convert uint to string byte array")
{
    REQUIRE(tools.convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("Case insensitive string comparison")
{
    REQUIRE(tools.caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(tools.caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("Erase null chars from first found")
{
    text = "DEAD";
    text.resize(8);
    REQUIRE(tools.eraseNullChars(text) == "DEAD");
    text = "DEAD";
    text.resize(8);
    text += "BEEF";
    REQUIRE(tools.eraseNullChars(text) == "DEAD");
}

TEST_CASE("Erase only last \r char")
{
    text = "DEAD\r";
    REQUIRE(tools.eraseCarriageReturnChar(text) == "DEAD");
    text = "DE\rAD\r";
    REQUIRE(tools.eraseCarriageReturnChar(text) == "DE\rAD");
}

TEST_CASE("Add dialog topics to INFO strings")
{
    yampt::single_dict_t dict;
    dict.insert({"clanfear", "postrach klanów"});
    text = "Some text clanfear some text";
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text clanfear some text [postrach klanów]");
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, true) ==
            "Some text clanfear some text [clanfear -> postrach klanów]");

    text = "Some text CLANFEAR some text";
    REQUIRE(tools.addDialogTopicsToINFOStrings(dict, text, false) ==
            "Some text CLANFEAR some text [postrach klanów]");
}
