#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esm_converter.hpp"
#include "../yampt/dict_merger.hpp"

#include <fstream>
#include <cstdio>

using namespace std;

static std::string make_subrecord(const std::string & id, const std::string & data)
{
	return id + tools_t::convert_uint_to_string_byte_array(data.size()) + data;
}

static std::string make_record(const std::string & id, const std::string & subrecords)
{
	std::string flags(8, '\0');
	std::string size_bytes = tools_t::convert_uint_to_string_byte_array(subrecords.size() + 8);
	return id + size_bytes + flags + subrecords;
}

static std::string make_tes3_header()
{
	std::string hedr_data(300, '\0');
	std::string hedr_sub = make_subrecord("HEDR", hedr_data);
	return make_record("TES3", hedr_sub);
}

static std::string make_npc_record(const std::string & name_id, const std::string & fnam_text)
{
	std::string name_data = name_id + '\0';
	std::string fnam_data = fnam_text + '\0';
	std::string subs = make_subrecord("NAME", name_data) + make_subrecord("FNAM", fnam_data);
	return make_record("NPC_", subs);
}

static void write_temp_esm(const std::string & path, const std::string & content)
{
	std::ofstream f(path, std::ios::binary);
	f << content;
}

static void remove_temp_file(const std::string & path)
{
	std::remove(path.c_str());
}

TEST_CASE("esm_converter_t applies FNAM translation to NPC record", "[i]")
{
	tools_t::reset_log();

	const std::string esm_path = "temp_esmconv_fnam.esm";
	const std::string dict_path = "temp_esmconv_fnam.yml";

	std::string esm_content = make_tes3_header() + make_npc_record("TestNPC", "OldName");
	write_temp_esm(esm_path, esm_content);

	std::string dict_content = "<record>\r\n"
	                           "\t<_id>FNAM</_id>\r\n"
	                           "\t<key>NPC_^TestNPC</key>\r\n"
	                           "\t<val>NewName</val>\r\n"
	                           "</record>\r\n";

	std::ofstream df(dict_path, std::ios::binary);
	df << dict_content;
	df.close();

	dict_merger_t merger({ dict_path });
	esm_converter_t converter(esm_path, merger, false, "", tools_t::encoding_t::unknown, false);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records.size() == 2);

	const auto & npc_record = records[1];
	REQUIRE(npc_record.id == "NPC_");

	std::string expected_fnam = "NewName";
	expected_fnam += '\0';
	size_t fnam_pos = npc_record.content.find("FNAM");
	REQUIRE(fnam_pos != std::string::npos);

	size_t fnam_data_size = tools_t::convert_string_byte_array_to_uint(npc_record.content.substr(fnam_pos + 4, 4));
	std::string fnam_data = npc_record.content.substr(fnam_pos + 8, fnam_data_size);
	REQUIRE(fnam_data == expected_fnam);

	remove_temp_file(esm_path);
	remove_temp_file(dict_path);
}

TEST_CASE("esm_converter_t recalculates record size after FNAM replacement", "[i]")
{
	tools_t::reset_log();

	const std::string esm_path = "temp_esmconv_size.esm";
	const std::string dict_path = "temp_esmconv_size.yml";

	std::string esm_content = make_tes3_header() + make_npc_record("SizeNPC", "Short");
	write_temp_esm(esm_path, esm_content);

	std::string dict_content = "<record>\r\n"
	                           "\t<_id>FNAM</_id>\r\n"
	                           "\t<key>NPC_^SizeNPC</key>\r\n"
	                           "\t<val>MuchLongerTranslatedName</val>\r\n"
	                           "</record>\r\n";

	std::ofstream df(dict_path, std::ios::binary);
	df << dict_content;
	df.close();

	dict_merger_t merger({ dict_path });
	esm_converter_t converter(esm_path, merger, false, "", tools_t::encoding_t::unknown, false);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & npc_record = records[1];

	size_t stored_size = tools_t::convert_string_byte_array_to_uint(npc_record.content.substr(4, 4));
	size_t expected_size = npc_record.content.size() - 16;
	REQUIRE(stored_size == expected_size);

	REQUIRE(npc_record.size == npc_record.content.size());

	remove_temp_file(esm_path);
	remove_temp_file(dict_path);
}

TEST_CASE("esm_converter_t preserves null-termination for FNAM after replacement", "[i]")
{
	tools_t::reset_log();

	const std::string esm_path = "temp_esmconv_null.esm";
	const std::string dict_path = "temp_esmconv_null.yml";

	std::string esm_content = make_tes3_header() + make_npc_record("NullNPC", "Original");
	write_temp_esm(esm_path, esm_content);

	std::string dict_content = "<record>\r\n"
	                           "\t<_id>FNAM</_id>\r\n"
	                           "\t<key>NPC_^NullNPC</key>\r\n"
	                           "\t<val>Translated</val>\r\n"
	                           "</record>\r\n";

	std::ofstream df(dict_path, std::ios::binary);
	df << dict_content;
	df.close();

	dict_merger_t merger({ dict_path });
	esm_converter_t converter(esm_path, merger, false, "", tools_t::encoding_t::unknown, false);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & npc_record = records[1];

	size_t fnam_pos = npc_record.content.find("FNAM");
	REQUIRE(fnam_pos != std::string::npos);

	size_t fnam_data_size = tools_t::convert_string_byte_array_to_uint(npc_record.content.substr(fnam_pos + 4, 4));
	std::string fnam_data = npc_record.content.substr(fnam_pos + 8, fnam_data_size);

	REQUIRE(fnam_data.back() == '\0');

	std::string text_without_null = fnam_data.substr(0, fnam_data.size() - 1);
	REQUIRE(text_without_null == "Translated");
	REQUIRE(text_without_null.find('\0') == std::string::npos);

	remove_temp_file(esm_path);
	remove_temp_file(dict_path);
}
