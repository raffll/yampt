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

TEST_CASE("Chapter::insert", "[u]")
{
    Tools::Chapter chapter;

    SECTION("inserting a new entry returns true")
    {
        bool result = chapter.insert({ "key1", "orig1", "trans1", "untranslated" });
        REQUIRE(result == true);
    }

    SECTION("inserting duplicate id returns false")
    {
        chapter.insert({ "key1", "orig1", "trans1", "untranslated" });
        bool result = chapter.insert({ "key1", "orig2", "trans2", "translated" });
        REQUIRE(result == false);
    }
}

TEST_CASE("Chapter::find", "[u]")
{
    Tools::Chapter chapter;
    chapter.insert({ "key1", "orig1", "trans1", "untranslated" });

    SECTION("finding existing id returns non-null pointer with correct data")
    {
        auto * entry = chapter.find("key1");
        REQUIRE(entry != nullptr);
        REQUIRE(entry->id == "key1");
        REQUIRE(entry->original == "orig1");
        REQUIRE(entry->translation == "trans1");
        REQUIRE(entry->status == "untranslated");
    }

    SECTION("finding non-existent id returns nullptr")
    {
        auto * entry = chapter.find("no_such_key");
        REQUIRE(entry == nullptr);
    }
}

TEST_CASE("Chapter::size", "[u]")
{
    Tools::Chapter chapter;
    REQUIRE(chapter.size() == 0);

    chapter.insert({ "key1", "orig1", "trans1", "untranslated" });
    REQUIRE(chapter.size() == 1);

    chapter.insert({ "key2", "orig2", "trans2", "translated" });
    REQUIRE(chapter.size() == 2);

    chapter.insert({ "key1", "dup", "dup", "dup" });
    REQUIRE(chapter.size() == 2);
}

TEST_CASE("Chapter::empty", "[u]")
{
    Tools::Chapter chapter;
    REQUIRE(chapter.empty() == true);

    chapter.insert({ "key1", "orig1", "trans1", "untranslated" });
    REQUIRE(chapter.empty() == false);
}

TEST_CASE("type2Str and str2Type round-trip", "[u]")
{
    const std::vector<Tools::RecType> defined_types
    {
        Tools::RecType::CELL,
        Tools::RecType::DIAL,
        Tools::RecType::INDX,
        Tools::RecType::RNAM,
        Tools::RecType::DESC,
        Tools::RecType::GMST,
        Tools::RecType::FNAM,
        Tools::RecType::INFO,
        Tools::RecType::TEXT,
        Tools::RecType::BNAM,
        Tools::RecType::SCTX,
        Tools::RecType::Glossary,
        Tools::RecType::NPC_FLAG,
    };

    for (const auto & type : defined_types)
    {
        REQUIRE(Tools::str2Type(Tools::type2Str(type)) == type);
    }

    SECTION("str2Type returns Unknown for unrecognized string")
    {
        REQUIRE(Tools::str2Type("XYZZY") == Tools::RecType::Unknown);
        REQUIRE(Tools::str2Type("") == Tools::RecType::Unknown);
        REQUIRE(Tools::str2Type("cell") == Tools::RecType::Unknown);
    }
}

TEST_CASE("getDialogType all values", "[u]")
{
    REQUIRE(Tools::getDialogType(std::string(1, '\x00')) == "T");
    REQUIRE(Tools::getDialogType(std::string(1, '\x01')) == "V");
    REQUIRE(Tools::getDialogType(std::string(1, '\x02')) == "G");
    REQUIRE(Tools::getDialogType(std::string(1, '\x03')) == "P");
    REQUIRE(Tools::getDialogType(std::string(1, '\x04')) == "J");
}

TEST_CASE("getINDX zero-padded output", "[u]")
{
    // 4-byte little-endian encoding of 1 → "001"
    std::string one = Tools::convertUIntToStringByteArray(1);
    REQUIRE(Tools::getINDX(one) == "001");

    // 4-byte little-endian encoding of 42 → "042"
    std::string fortytwo = Tools::convertUIntToStringByteArray(42);
    REQUIRE(Tools::getINDX(fortytwo) == "042");

    // 4-byte little-endian encoding of 255 → "255"
    std::string twofiftyfive = Tools::convertUIntToStringByteArray(255);
    REQUIRE(Tools::getINDX(twofiftyfive) == "255");
}

TEST_CASE("isFNAM true IDs", "[u]")
{
    const std::vector<std::string> true_ids
    {
        "ACTI", "ALCH", "APPA", "ARMO",
        "BOOK", "BSGN", "CLAS", "CLOT",
        "CONT", "CREA", "DOOR", "FACT",
        "INGR", "LIGH", "LOCK", "MISC",
        "NPC_", "PROB", "RACE", "REGN",
        "REPA", "SKIL", "SPEL", "WEAP",
    };

    for (const auto & id : true_ids)
    {
        REQUIRE(Tools::isFNAM(id) == true);
    }
}

TEST_CASE("isFNAM false IDs", "[u]")
{
    REQUIRE(Tools::isFNAM("CELL") == false);
    REQUIRE(Tools::isFNAM("INFO") == false);
    REQUIRE(Tools::isFNAM("DIAL") == false);
}

TEST_CASE("byte conversion round-trip", "[u]")
{
    // Validates: Requirements 1.5
    // Property 1: for any unsigned 32-bit integer x,
    // convertStringByteArrayToUInt(convertUIntToStringByteArray(x)) == x
    const unsigned int values[] = {
        0u,
        1u,
        127u,
        128u,
        255u,
        256u,
        65535u,
        65536u,
        0x7FFFFFFFu,
        0xFFFFFFFFu,
        0x01020304u,  // all four bytes non-zero
        0x0A0B0C0Du,  // all four bytes non-zero
        0x11223344u,  // all four bytes non-zero
        0xDEADBEEFu,  // all four bytes non-zero
        0xCAFEBABEu,  // all four bytes non-zero
    };
    for (unsigned int x : values)
    {
        REQUIRE(Tools::convertStringByteArrayToUInt(Tools::convertUIntToStringByteArray(x)) == x);
    }
}

TEST_CASE("trimCR, no CR present", "[u]")
{
    std::string text = "DEAD";
    REQUIRE(Tools::trimCR(text) == "DEAD");
    text = "";
    REQUIRE(Tools::trimCR(text) == "");
    text = "no carriage return here";
    REQUIRE(Tools::trimCR(text) == "no carriage return here");
}

TEST_CASE("replace non-readable chars with dot", "[u]")
{
    SECTION("printable-only string is preserved")
    {
        std::string printable;
        for (int c = 32; c <= 126; ++c)
        {
            printable += static_cast<char>(c);
        }
        REQUIRE(Tools::replaceNonReadableCharsWithDot(printable) == printable);
    }

    SECTION("non-printable bytes are replaced with dot")
    {
        std::string input = "\x01\x1F\x7F\x80\xFF";
        std::string expected = ".....";
        REQUIRE(Tools::replaceNonReadableCharsWithDot(input) == expected);
    }
}

TEST_CASE("replaceNonReadableCharsWithDot property: printable chars preserved", "[u]")
{
    // Validates: Requirements 2.8
    // Property 2: Printable Characters Are Preserved
    for (int c = 32; c <= 126; ++c)
    {
        std::string single(1, static_cast<char>(c));
        REQUIRE(Tools::replaceNonReadableCharsWithDot(single) == single);
    }

    std::string all_printable;
    for (int c = 32; c <= 126; ++c)
    {
        all_printable += static_cast<char>(c);
    }
    REQUIRE(Tools::replaceNonReadableCharsWithDot(all_printable) == all_printable);
}

TEST_CASE("replaceNonReadableCharsWithDot property: all bytes", "[u]")
{
    // Validates: Requirements 2.9
    // Property 3: Non-Printable Characters Are Replaced with Dot
    for (int i = 0; i <= 255; ++i)
    {
        std::string single(1, static_cast<char>(i));
        std::string result = Tools::replaceNonReadableCharsWithDot(single);
        if (std::isprint(static_cast<unsigned char>(i)))
        {
            REQUIRE(result == single);
        }
        else
        {
            REQUIRE(result == ".");
        }
    }
}

TEST_CASE("initializeDict has all expected keys", "[u]")
{
    Tools::Dict dict = Tools::initializeDict();

    REQUIRE(dict.count(Tools::RecType::CELL) == 1);
    REQUIRE(dict.count(Tools::RecType::DIAL) == 1);
    REQUIRE(dict.count(Tools::RecType::INDX) == 1);
    REQUIRE(dict.count(Tools::RecType::RNAM) == 1);
    REQUIRE(dict.count(Tools::RecType::DESC) == 1);
    REQUIRE(dict.count(Tools::RecType::GMST) == 1);
    REQUIRE(dict.count(Tools::RecType::FNAM) == 1);
    REQUIRE(dict.count(Tools::RecType::INFO) == 1);
    REQUIRE(dict.count(Tools::RecType::TEXT) == 1);
    REQUIRE(dict.count(Tools::RecType::BNAM) == 1);
    REQUIRE(dict.count(Tools::RecType::SCTX) == 1);
    REQUIRE(dict.count(Tools::RecType::Glossary) == 1);
    REQUIRE(dict.count(Tools::RecType::NPC_FLAG) == 1);

    REQUIRE(dict.size() == 13);
}

TEST_CASE("initializeDict all chapters empty", "[u]")
{
    // Validates: Requirements 5.2
    // Property 5: Fresh Dict Has All Chapters Empty
    Tools::Dict dict = Tools::initializeDict();

    for (const auto & chapter : dict)
    {
        REQUIRE(chapter.second.empty());
    }
}

TEST_CASE("getNumberOfElementsInDict zero for empty dict", "[u]")
{
    Tools::Dict dict = Tools::initializeDict();
    REQUIRE(Tools::getNumberOfElementsInDict(dict) == 0);
}

TEST_CASE("getNumberOfElementsInDict counts inserted entries", "[u]")
{
    Tools::Dict dict = Tools::initializeDict();
    dict.at(Tools::RecType::CELL).insert({ "Balmora", "Balmora", "Balmora", "translated" });
    dict.at(Tools::RecType::CELL).insert({ "Vivec", "Vivec", "Vivec", "translated" });
    dict.at(Tools::RecType::DIAL).insert({ "clanfear", "clanfear", "postrach klanów", "translated" });
    REQUIRE(Tools::getNumberOfElementsInDict(dict) == 3);
}

TEST_CASE("getNumberOfElementsInDict correct total", "[u]")
{
    Tools::Dict dict = Tools::initializeDict();
    dict.at(Tools::RecType::CELL).insert({ "Balmora", "Balmora", "Balmora", "translated" });
    dict.at(Tools::RecType::CELL).insert({ "Vivec", "Vivec", "Vivec", "translated" });
    dict.at(Tools::RecType::DIAL).insert({ "clanfear", "clanfear", "postrach klanów", "translated" });
    dict.at(Tools::RecType::INFO).insert({ "info_key", "info_orig", "info_val", "untranslated" });
    REQUIRE(Tools::getNumberOfElementsInDict(dict) == 4);
}
