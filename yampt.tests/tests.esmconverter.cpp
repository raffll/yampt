#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esmconverter.hpp"
#include "../yampt/dictmerger.hpp"

#include <fstream>
#include <cstdio>

using namespace std;

static std::string makeSubrecord(const std::string & id, const std::string & data)
{
    return id + Tools::convertUIntToStringByteArray(data.size()) + data;
}

static std::string makeRecord(const std::string & id, const std::string & subrecords)
{
    std::string flags(8, '\0');
    std::string size_bytes = Tools::convertUIntToStringByteArray(subrecords.size() + 8);
    return id + size_bytes + flags + subrecords;
}

static std::string makeTES3Header()
{
    std::string hedr_data(300, '\0');
    std::string hedr_sub = makeSubrecord("HEDR", hedr_data);
    return makeRecord("TES3", hedr_sub);
}

static std::string makeNpcRecord(const std::string & name_id, const std::string & fnam_text)
{
    std::string name_data = name_id + '\0';
    std::string fnam_data = fnam_text + '\0';
    std::string subs = makeSubrecord("NAME", name_data) + makeSubrecord("FNAM", fnam_data);
    return makeRecord("NPC_", subs);
}

static void writeTempEsm(const std::string & path, const std::string & content)
{
    std::ofstream f(path, std::ios::binary);
    f << content;
}

static void removeTempFile(const std::string & path)
{
    std::remove(path.c_str());
}

TEST_CASE("EsmConverter applies FNAM translation to NPC record", "[i]")
{
    Tools::resetLog();

    const std::string esm_path = "temp_esmconv_fnam.esm";
    const std::string dict_path = "temp_esmconv_fnam.yml";

    std::string esm_content = makeTES3Header() + makeNpcRecord("TestNPC", "OldName");
    writeTempEsm(esm_path, esm_content);

    std::string dict_content =
        "<record>\r\n"
        "\t<_id>FNAM</_id>\r\n"
        "\t<key>NPC_^TestNPC</key>\r\n"
        "\t<val>NewName</val>\r\n"
        "</record>\r\n";

    std::ofstream df(dict_path, std::ios::binary);
    df << dict_content;
    df.close();

    DictMerger merger({ dict_path });
    EsmConverter converter(esm_path, merger, false, "", Tools::Encoding::UNKNOWN, false);

    REQUIRE(converter.isLoaded());

    const auto & records = converter.getRecords();
    REQUIRE(records.size() == 2);

    const auto & npc_record = records[1];
    REQUIRE(npc_record.id == "NPC_");

    std::string expected_fnam = "NewName";
    expected_fnam += '\0';
    size_t fnam_pos = npc_record.content.find("FNAM");
    REQUIRE(fnam_pos != std::string::npos);

    size_t fnam_data_size = Tools::convertStringByteArrayToUInt(
        npc_record.content.substr(fnam_pos + 4, 4));
    std::string fnam_data = npc_record.content.substr(fnam_pos + 8, fnam_data_size);
    REQUIRE(fnam_data == expected_fnam);

    removeTempFile(esm_path);
    removeTempFile(dict_path);
}

TEST_CASE("EsmConverter recalculates record size after FNAM replacement", "[i]")
{
    Tools::resetLog();

    const std::string esm_path = "temp_esmconv_size.esm";
    const std::string dict_path = "temp_esmconv_size.yml";

    std::string esm_content = makeTES3Header() + makeNpcRecord("SizeNPC", "Short");
    writeTempEsm(esm_path, esm_content);

    std::string dict_content =
        "<record>\r\n"
        "\t<_id>FNAM</_id>\r\n"
        "\t<key>NPC_^SizeNPC</key>\r\n"
        "\t<val>MuchLongerTranslatedName</val>\r\n"
        "</record>\r\n";

    std::ofstream df(dict_path, std::ios::binary);
    df << dict_content;
    df.close();

    DictMerger merger({ dict_path });
    EsmConverter converter(esm_path, merger, false, "", Tools::Encoding::UNKNOWN, false);

    REQUIRE(converter.isLoaded());

    const auto & records = converter.getRecords();
    const auto & npc_record = records[1];

    size_t stored_size = Tools::convertStringByteArrayToUInt(npc_record.content.substr(4, 4));
    size_t expected_size = npc_record.content.size() - 16;
    REQUIRE(stored_size == expected_size);

    REQUIRE(npc_record.size == npc_record.content.size());

    removeTempFile(esm_path);
    removeTempFile(dict_path);
}

TEST_CASE("EsmConverter preserves null-termination for FNAM after replacement", "[i]")
{
    Tools::resetLog();

    const std::string esm_path = "temp_esmconv_null.esm";
    const std::string dict_path = "temp_esmconv_null.yml";

    std::string esm_content = makeTES3Header() + makeNpcRecord("NullNPC", "Original");
    writeTempEsm(esm_path, esm_content);

    std::string dict_content =
        "<record>\r\n"
        "\t<_id>FNAM</_id>\r\n"
        "\t<key>NPC_^NullNPC</key>\r\n"
        "\t<val>Translated</val>\r\n"
        "</record>\r\n";

    std::ofstream df(dict_path, std::ios::binary);
    df << dict_content;
    df.close();

    DictMerger merger({ dict_path });
    EsmConverter converter(esm_path, merger, false, "", Tools::Encoding::UNKNOWN, false);

    REQUIRE(converter.isLoaded());

    const auto & records = converter.getRecords();
    const auto & npc_record = records[1];

    size_t fnam_pos = npc_record.content.find("FNAM");
    REQUIRE(fnam_pos != std::string::npos);

    size_t fnam_data_size = Tools::convertStringByteArrayToUInt(
        npc_record.content.substr(fnam_pos + 4, 4));
    std::string fnam_data = npc_record.content.substr(fnam_pos + 8, fnam_data_size);

    REQUIRE(fnam_data.back() == '\0');

    std::string text_without_null = fnam_data.substr(0, fnam_data.size() - 1);
    REQUIRE(text_without_null == "Translated");
    REQUIRE(text_without_null.find('\0') == std::string::npos);

    removeTempFile(esm_path);
    removeTempFile(dict_path);
}
