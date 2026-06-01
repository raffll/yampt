#include "catch.hpp"
#include "../yampt/tools.hpp"

TEST_CASE("convert string byte array to uint", "[u]")
{
    std::string text = "DEAD";
    REQUIRE(tools_t::convertStringByteArrayToUInt(text) == 1145128260);
    text = "D";
    REQUIRE(tools_t::convertStringByteArrayToUInt(text) == 68);
    for (int i = 0; i < 3; ++i)
    {
        text += '\0';
    }
    REQUIRE(text.size() == 4);
    REQUIRE(tools_t::convertStringByteArrayToUInt(text) == 68);
}

TEST_CASE("convert uint to string byte array", "[u]")
{
    REQUIRE(tools_t::convertUIntToStringByteArray(1145128260) == "DEAD");
}

TEST_CASE("case insensitive string comparison", "[u]")
{
    REQUIRE(tools_t::caseInsensitiveStringCmp("DEAD", "dead") == true);
    REQUIRE(tools_t::caseInsensitiveStringCmp("DEAD", "BEEF") == false);
}

TEST_CASE("erase null chars from first found", "[u]")
{
    std::string text = "DEAD";
    text.resize(8);
    REQUIRE(tools_t::eraseNullChars(text) == "DEAD");
    text = "DEAD";
    text.resize(8);
    text += "BEEF";
    REQUIRE(tools_t::eraseNullChars(text) == "DEAD");
}

TEST_CASE("erase only last \\r char", "[u]")
{
    std::string text = "DEAD\r";
    REQUIRE(tools_t::trimCR(text) == "DEAD");
    text = "DE\rAD\r";
    REQUIRE(tools_t::trimCR(text) == "DE\rAD");
}

TEST_CASE("Chapter::insert", "[u]")
{
    tools_t::Chapter chapter;

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
    tools_t::Chapter chapter;
    chapter.insert({ "key1", "orig1", "trans1", "untranslated" });

    SECTION("finding existing id returns non-null pointer with correct data")
    {
        auto * entry = chapter.find("key1");
        REQUIRE(entry != nullptr);
        REQUIRE(entry->key_text == "key1");
        REQUIRE(entry->old_text == "orig1");
        REQUIRE(entry->new_text == "trans1");
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
    tools_t::Chapter chapter;
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
    tools_t::Chapter chapter;
    REQUIRE(chapter.empty() == true);

    chapter.insert({ "key1", "orig1", "trans1", "untranslated" });
    REQUIRE(chapter.empty() == false);
}

TEST_CASE("type2Str and str2Type round-trip", "[u]")
{
    const std::vector<tools_t::rec_type_t> defined_types
    {
        tools_t::rec_type_t::CELL,
        tools_t::rec_type_t::DIAL,
        tools_t::rec_type_t::INDX,
        tools_t::rec_type_t::RNAM,
        tools_t::rec_type_t::DESC,
        tools_t::rec_type_t::GMST,
        tools_t::rec_type_t::FNAM,
        tools_t::rec_type_t::INFO,
        tools_t::rec_type_t::TEXT,
        tools_t::rec_type_t::BNAM,
        tools_t::rec_type_t::SCTX,
        tools_t::rec_type_t::Glossary,
        tools_t::rec_type_t::NPC_FLAG,
    };

    for (const auto & type : defined_types)
    {
        REQUIRE(tools_t::str2Type(tools_t::type2Str(type)) == type);
    }

    SECTION("str2Type returns Unknown for unrecognized string")
    {
        REQUIRE(tools_t::str2Type("XYZZY") == tools_t::rec_type_t::Unknown);
        REQUIRE(tools_t::str2Type("") == tools_t::rec_type_t::Unknown);
        REQUIRE(tools_t::str2Type("cell") == tools_t::rec_type_t::Unknown);
    }
}

TEST_CASE("getDialogType all values", "[u]")
{
    REQUIRE(tools_t::getDialogType(std::string(1, '\x00')) == "T");
    REQUIRE(tools_t::getDialogType(std::string(1, '\x01')) == "V");
    REQUIRE(tools_t::getDialogType(std::string(1, '\x02')) == "G");
    REQUIRE(tools_t::getDialogType(std::string(1, '\x03')) == "P");
    REQUIRE(tools_t::getDialogType(std::string(1, '\x04')) == "J");
}

TEST_CASE("getINDX zero-padded output", "[u]")
{
    // 4-byte little-endian encoding of 1 → "001"
    std::string one = tools_t::convertUIntToStringByteArray(1);
    REQUIRE(tools_t::getINDX(one) == "001");

    // 4-byte little-endian encoding of 42 → "042"
    std::string fortytwo = tools_t::convertUIntToStringByteArray(42);
    REQUIRE(tools_t::getINDX(fortytwo) == "042");

    // 4-byte little-endian encoding of 255 → "255"
    std::string twofiftyfive = tools_t::convertUIntToStringByteArray(255);
    REQUIRE(tools_t::getINDX(twofiftyfive) == "255");
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
        REQUIRE(tools_t::isFNAM(id) == true);
    }
}

TEST_CASE("isFNAM false IDs", "[u]")
{
    REQUIRE(tools_t::isFNAM("CELL") == false);
    REQUIRE(tools_t::isFNAM("INFO") == false);
    REQUIRE(tools_t::isFNAM("DIAL") == false);
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
        REQUIRE(tools_t::convertStringByteArrayToUInt(tools_t::convertUIntToStringByteArray(x)) == x);
    }
}

TEST_CASE("trimCR, no CR present", "[u]")
{
    std::string text = "DEAD";
    REQUIRE(tools_t::trimCR(text) == "DEAD");
    text = "";
    REQUIRE(tools_t::trimCR(text) == "");
    text = "no carriage return here";
    REQUIRE(tools_t::trimCR(text) == "no carriage return here");
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
        REQUIRE(tools_t::replaceNonReadableCharsWithDot(printable) == printable);
    }

    SECTION("non-printable bytes are replaced with dot")
    {
        std::string input = "\x01\x1F\x7F\x80\xFF";
        std::string expected = ".....";
        REQUIRE(tools_t::replaceNonReadableCharsWithDot(input) == expected);
    }
}

TEST_CASE("replaceNonReadableCharsWithDot property: printable chars preserved", "[u]")
{
    // Validates: Requirements 2.8
    // Property 2: Printable Characters Are Preserved
    for (int c = 32; c <= 126; ++c)
    {
        std::string single(1, static_cast<char>(c));
        REQUIRE(tools_t::replaceNonReadableCharsWithDot(single) == single);
    }

    std::string all_printable;
    for (int c = 32; c <= 126; ++c)
    {
        all_printable += static_cast<char>(c);
    }
    REQUIRE(tools_t::replaceNonReadableCharsWithDot(all_printable) == all_printable);
}

TEST_CASE("replaceNonReadableCharsWithDot property: all bytes", "[u]")
{
    // Validates: Requirements 2.9
    // Property 3: Non-Printable Characters Are Replaced with Dot
    for (int i = 0; i <= 255; ++i)
    {
        std::string single(1, static_cast<char>(i));
        std::string result = tools_t::replaceNonReadableCharsWithDot(single);
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
    tools_t::dict_t dict = tools_t::initializeDict();

    REQUIRE(dict.count(tools_t::rec_type_t::CELL) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::DIAL) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::INDX) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::RNAM) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::DESC) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::GMST) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::FNAM) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::INFO) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::TEXT) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::BNAM) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::SCTX) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::Glossary) == 1);
    REQUIRE(dict.count(tools_t::rec_type_t::NPC_FLAG) == 1);

    REQUIRE(dict.size() == 13);
}

TEST_CASE("initializeDict all chapters empty", "[u]")
{
    // Validates: Requirements 5.2
    // Property 5: Fresh dict_t Has All Chapters Empty
    tools_t::dict_t dict = tools_t::initializeDict();

    for (const auto & chapter : dict)
    {
        REQUIRE(chapter.second.empty());
    }
}

TEST_CASE("getNumberOfElementsInDict zero for empty dict", "[u]")
{
    tools_t::dict_t dict = tools_t::initializeDict();
    REQUIRE(tools_t::getNumberOfElementsInDict(dict) == 0);
}

TEST_CASE("getNumberOfElementsInDict counts inserted entries", "[u]")
{
    tools_t::dict_t dict = tools_t::initializeDict();
    dict.at(tools_t::rec_type_t::CELL).insert({ "Balmora", "Balmora", "Balmora", "translated" });
    dict.at(tools_t::rec_type_t::CELL).insert({ "Vivec", "Vivec", "Vivec", "translated" });
    dict.at(tools_t::rec_type_t::DIAL).insert({ "clanfear", "clanfear", "postrach klanów", "translated" });
    REQUIRE(tools_t::getNumberOfElementsInDict(dict) == 3);
}

TEST_CASE("getNumberOfElementsInDict correct total", "[u]")
{
    tools_t::dict_t dict = tools_t::initializeDict();
    dict.at(tools_t::rec_type_t::CELL).insert({ "Balmora", "Balmora", "Balmora", "translated" });
    dict.at(tools_t::rec_type_t::CELL).insert({ "Vivec", "Vivec", "Vivec", "translated" });
    dict.at(tools_t::rec_type_t::DIAL).insert({ "clanfear", "clanfear", "postrach klanów", "translated" });
    dict.at(tools_t::rec_type_t::INFO).insert({ "info_key", "info_orig", "info_val", "untranslated" });
    REQUIRE(tools_t::getNumberOfElementsInDict(dict) == 4);
}
