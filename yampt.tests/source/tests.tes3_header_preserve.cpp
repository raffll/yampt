#include <catch2/catch_all.hpp>
#include <patcher/patch_builder.hpp>
#include <utility/domain_types.hpp>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

constexpr size_t record_header_length = 16;
constexpr size_t sub_header_length = 8;
constexpr size_t hedr_data_start = record_header_length + sub_header_length;
constexpr size_t hedr_flags_at = hedr_data_start + 4;
constexpr size_t hedr_author_at = hedr_data_start + 8;
constexpr size_t hedr_desc_at = hedr_data_start + 40;
constexpr size_t hedr_numrec_at = hedr_data_start + 296;
constexpr size_t hedr_length = 300;

struct master_t
{
	std::string name;
	uint64_t size = 0;
};

std::string make_sub(const std::string & type, const std::string & data)
{
	return type + domain_types::convert_uint_to_string_byte_array(data.size()) + data;
}

std::string make_master_pair(const master_t & master)
{
	std::string size_data(8, '\0');
	std::memcpy(size_data.data(), &master.size, 8);
	return make_sub("MAST", master.name + '\0') + make_sub("DATA", size_data);
}

std::string make_original_header(const std::vector<master_t> & masters, uint32_t record_count)
{
	std::string hedr(hedr_length, '\0');
	const float version = 1.2f;
	const uint32_t flags = 1;
	std::memcpy(hedr.data(), &version, 4);
	std::memcpy(hedr.data() + 4, &flags, 4);
	std::memcpy(hedr.data() + 8, "Some Modder", 11);
	std::memcpy(hedr.data() + 40, "My hand-written description", 27);
	std::memcpy(hedr.data() + 296, &record_count, 4);

	std::string body = make_sub("HEDR", hedr);
	for (const auto & master : masters)
		body += make_master_pair(master);

	std::string record = "TES3" + domain_types::convert_uint_to_string_byte_array(body.size());
	record += std::string("\x44\x33\x22\x11", 4);
	record += std::string("\x05\x00\x00\x00", 4);
	return record + body;
}

std::string with_zeroed_count(std::string header)
{
	std::memset(header.data() + hedr_numrec_at, 0, 4);
	return header;
}

uint32_t read_count(const std::string & header)
{
	uint32_t value = 0;
	std::memcpy(&value, header.data() + hedr_numrec_at, 4);
	return value;
}

std::vector<master_t> read_masters(const std::string & header)
{
	std::vector<master_t> masters;
	size_t pos = record_header_length;
	while (pos + sub_header_length <= header.size())
	{
		const auto type = header.substr(pos, 4);
		const auto size = domain_types::convert_string_byte_array_to_uint(header.substr(pos + 4, 4));
		if (type == "MAST")
		{
			master_t master;
			master.name = header.substr(pos + sub_header_length, size - 1);
			std::memcpy(&master.size, header.data() + pos + sub_header_length + size + sub_header_length, 8);
			masters.push_back(master);
		}
		pos += sub_header_length + size;
	}
	return masters;
}

patch_builder_t::master_entry_t entry_for(const master_t & master)
{
	return { master.name, master.size };
}

const master_t morrowind { "Morrowind.esm", 79837557 };
const master_t tribunal { "Tribunal.esm", 4411659 };
const master_t bloodmoon { "Bloodmoon.esm", 9162666 };

} // namespace

TEST_CASE("update_tes3_header, preserves author description version flags and masters", "[u]")
{
	const auto original = make_original_header({ morrowind, tribunal }, 7);

	const auto result = patch_builder_t::update_tes3_header(original, { 9, {} });

	REQUIRE(result.has_value());
	REQUIRE(read_count(*result) == 9);
	REQUIRE(with_zeroed_count(*result) == with_zeroed_count(original));
	REQUIRE(result->substr(hedr_author_at, 11) == "Some Modder");
	REQUIRE(result->substr(hedr_desc_at, 27) == "My hand-written description");
	REQUIRE(result->substr(hedr_flags_at, 4) == original.substr(hedr_flags_at, 4));
	REQUIRE(result->substr(0, hedr_data_start + 4) == original.substr(0, hedr_data_start + 4));

	const auto masters = read_masters(*result);
	REQUIRE(masters.size() == 2);
	REQUIRE(masters[0].name == "Morrowind.esm");
	REQUIRE(masters[0].size == morrowind.size);
	REQUIRE(masters[1].name == "Tribunal.esm");
	REQUIRE(masters[1].size == tribunal.size);
}

TEST_CASE("update_tes3_header, appends new master after existing masters", "[u]")
{
	const auto original = make_original_header({ morrowind, tribunal }, 7);

	const auto result = patch_builder_t::update_tes3_header(original, { 8, { entry_for(bloodmoon) } });

	REQUIRE(result.has_value());
	const auto masters = read_masters(*result);
	REQUIRE(masters.size() == 3);
	REQUIRE(masters[0].name == "Morrowind.esm");
	REQUIRE(masters[1].name == "Tribunal.esm");
	REQUIRE(masters[2].name == "Bloodmoon.esm");
	REQUIRE(masters[2].size == bloodmoon.size);
	REQUIRE(domain_types::convert_string_byte_array_to_uint(result->substr(4, 4)) == result->size() - 16);
	REQUIRE(result->substr(8, hedr_numrec_at - 8) == original.substr(8, hedr_numrec_at - 8));
}

TEST_CASE("update_tes3_header, does not duplicate an existing master", "[u]")
{
	const auto original = make_original_header({ morrowind, tribunal }, 7);

	const auto result = patch_builder_t::update_tes3_header(
	    original, { 7, { { "tribunal.ESM", 1 }, entry_for(bloodmoon), entry_for(bloodmoon) } });

	REQUIRE(result.has_value());
	const auto masters = read_masters(*result);
	REQUIRE(masters.size() == 3);
	REQUIRE(masters[1].name == "Tribunal.esm");
	REQUIRE(masters[1].size == tribunal.size);
	REQUIRE(masters[2].name == "Bloodmoon.esm");
}

TEST_CASE("update_tes3_header, never drops or reorders masters when no extras are given", "[u]")
{
	const auto original = make_original_header({ tribunal, morrowind, bloodmoon }, 3);

	const auto result = patch_builder_t::update_tes3_header(original, { 3, {} });

	REQUIRE(result.has_value());
	REQUIRE(*result == original);
}

TEST_CASE("update_tes3_header, header without masters gets extras appended", "[u]")
{
	const auto original = make_original_header({}, 1);

	const auto result = patch_builder_t::update_tes3_header(original, { 2, { entry_for(morrowind) } });

	REQUIRE(result.has_value());
	const auto masters = read_masters(*result);
	REQUIRE(masters.size() == 1);
	REQUIRE(masters[0].name == "Morrowind.esm");
}

TEST_CASE("update_tes3_header, rejects a header it cannot parse", "[u]")
{
	REQUIRE_FALSE(patch_builder_t::update_tes3_header("", { 1, {} }).has_value());
	REQUIRE_FALSE(patch_builder_t::update_tes3_header(std::string(40, 'x'), { 1, {} }).has_value());
}

TEST_CASE("build_tes3_header, yEditor-created plugin header is unchanged", "[u]")
{
	const auto merged = patch_builder_t::build_tes3_header(
	    "yEditor", "Auto-generated merged patch", 4, { entry_for(morrowind) });
	const auto created = patch_builder_t::build_tes3_header("yEditor", "Created with yEditor", 4, {});

	float version = 0.0f;
	std::memcpy(&version, merged.data() + hedr_data_start, 4);
	REQUIRE(version == 1.3f);
	REQUIRE(merged.substr(hedr_flags_at, 4) == std::string(4, '\0'));
	REQUIRE(merged.substr(hedr_author_at, 7) == "yEditor");
	REQUIRE(merged.substr(hedr_desc_at, 27) == "Auto-generated merged patch");
	REQUIRE(created.substr(hedr_desc_at, 20) == "Created with yEditor");
	REQUIRE(read_count(merged) == 4);
	REQUIRE(read_masters(merged).size() == 1);
}

TEST_CASE("patch_builder_t::save_with_header, writes the preserved header to disk", "[u]")
{
	const auto path = (std::filesystem::temp_directory_path() / "yampt_header_preserve_test.esp").string();
	const auto original = make_original_header({ morrowind, tribunal }, 7);

	patch_builder_t builder;
	builder.add_record_raw("GMST", "sTest", std::string(16, '\0'));
	const auto updated = patch_builder_t::update_tes3_header(original, { builder.record_count(), {} });
	REQUIRE(updated.has_value());
	REQUIRE(builder.save_with_header(path, *updated));

	std::ifstream stream(path, std::ios::binary);
	const std::string saved((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	stream.close();
	std::filesystem::remove(path);

	REQUIRE(saved.size() >= updated->size());
	REQUIRE(saved.substr(0, updated->size()) == *updated);
	REQUIRE(saved.substr(hedr_author_at, 11) == "Some Modder");
	REQUIRE(read_count(saved) == 1);
	REQUIRE(read_masters(saved).size() == 2);
}
