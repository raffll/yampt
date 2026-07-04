#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <patcher/patch_builder.hpp>
#include <cstring>
#include <string>

namespace {

rc::Gen<std::string> gen_short_string()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(0, 40);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange(static_cast<char>(0x20), static_cast<char>(0x7E));
		return result;
	});
}

uint32_t read_uint32(const std::string & data, size_t offset)
{
	uint32_t value = 0;
	std::memcpy(&value, data.data() + offset, 4);
	return value;
}

}

TEST_CASE("patch_builder_t::build_tes3_header, starts with TES3", "[pbt]")
{
	rc::prop(
		"first 4 bytes are TES3",
		[]()
	{
		const auto author = *gen_short_string();
		const auto description = *gen_short_string();
		const auto record_count = static_cast<size_t>(*rc::gen::inRange(1, 100));

		const auto header = patch_builder_t::build_tes3_header(author, description, record_count, {});

		RC_ASSERT(header.size() >= 4);
		RC_ASSERT(header.substr(0, 4) == "TES3");
	});
}

TEST_CASE("patch_builder_t::build_tes3_header, contains HEDR sub-record", "[pbt]")
{
	rc::prop(
		"HEDR sub-record has valid size",
		[]()
	{
		const auto author = *gen_short_string();
		const auto description = *gen_short_string();
		const auto record_count = static_cast<size_t>(*rc::gen::inRange(1, 100));

		const auto header = patch_builder_t::build_tes3_header(author, description, record_count, {});

		RC_ASSERT(header.size() >= 16 + 8 + 300);

		const auto hedr_pos = header.find("HEDR", 16);
		RC_ASSERT(hedr_pos != std::string::npos);

		const auto hedr_size = read_uint32(header, hedr_pos + 4);
		RC_ASSERT(hedr_size == 300);
	});
}

TEST_CASE("patch_builder_t::build_tes3_header, author is null-terminated in HEDR", "[pbt]")
{
	rc::prop(
		"author appears within HEDR data",
		[]()
	{
		const auto author = *gen_short_string();
		const auto description = *gen_short_string();

		const auto header = patch_builder_t::build_tes3_header(author, description, 1, {});

		const auto hedr_pos = header.find("HEDR", 16);
		RC_ASSERT(hedr_pos != std::string::npos);

		const auto hedr_data_start = hedr_pos + 8;
		const auto author_offset = hedr_data_start + 8;
		const auto max_author_len = std::min(author.size(), static_cast<size_t>(32));

		for (size_t index = 0; index < max_author_len; ++index)
			RC_ASSERT(header[author_offset + index] == author[index]);

		if (author.size() < 32)
			RC_ASSERT(header[author_offset + author.size()] == '\0');
	});
}

TEST_CASE("patch_builder_t::build_tes3_header, record count stored at correct offset", "[pbt]")
{
	rc::prop(
		"numrecords field matches input",
		[]()
	{
		const auto record_count = static_cast<size_t>(*rc::gen::inRange(1, 5000));

		const auto header = patch_builder_t::build_tes3_header("", "", record_count, {});

		const auto hedr_pos = header.find("HEDR", 16);
		RC_ASSERT(hedr_pos != std::string::npos);

		const auto hedr_data_start = hedr_pos + 8;
		const auto stored_count = read_uint32(header, hedr_data_start + 296);
		RC_ASSERT(stored_count == static_cast<uint32_t>(record_count));
	});
}

TEST_CASE("patch_builder_t::build_tes3_header, MAST sub-records for masters", "[pbt]")
{
	rc::prop(
		"each master produces MAST + DATA sub-records",
		[]()
	{
		const auto master_count = *rc::gen::inRange(1, 5);
		std::vector<patch_builder_t::master_entry_t> masters;

		for (int index = 0; index < master_count; ++index)
		{
			patch_builder_t::master_entry_t entry;
			entry.filename = "Master" + std::to_string(index) + ".esm";
			entry.file_size = static_cast<uint64_t>(*rc::gen::inRange(1000, 100000000));
			masters.push_back(entry);
		}

		const auto header = patch_builder_t::build_tes3_header("author", "desc", 1, masters);

		size_t mast_count = 0;
		size_t pos = 0;
		while ((pos = header.find("MAST", pos)) != std::string::npos)
		{
			++mast_count;
			pos += 4;
		}

		RC_ASSERT(static_cast<int>(mast_count) == master_count);

		size_t data_count = 0;
		pos = header.find("HEDR");
		while ((pos = header.find("DATA", pos + 1)) != std::string::npos)
			++data_count;

		RC_ASSERT(static_cast<int>(data_count) == master_count);
	});
}
