#include <scanner/sub_record_merge.hpp>
#include <cstring>
#include <string>

static std::string make_sub(const std::string & type, const std::string & data)
{
	std::string result;
	result += type;
	uint32_t size_val = static_cast<uint32_t>(data.size());
	result.append(reinterpret_cast<const char *>(&size_val), 4);
	result += data;
	return result;
}

static std::string make_record(const std::string & rec_type, const std::string & subs)
{
	std::string header;
	header += rec_type;
	uint32_t body_size = static_cast<uint32_t>(subs.size());
	header.append(reinterpret_cast<const char *>(&body_size), 4);
	uint32_t zero = 0;
	header.append(reinterpret_cast<const char *>(&zero), 4);
	header.append(reinterpret_cast<const char *>(&zero), 4);
	return header + subs;
}

static std::string make_cstr(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_wpdt(uint16_t health)
{
	std::string data(32, '\0');
	const float weight = 10.0f;
	const uint32_t value = 100;
	const uint16_t type = 3;
	const float speed = 1.0f;
	const float reach = 1.0f;
	std::memcpy(data.data() + 0, &weight, 4);
	std::memcpy(data.data() + 4, &value, 4);
	std::memcpy(data.data() + 8, &type, 2);
	std::memcpy(data.data() + 10, &health, 2);
	std::memcpy(data.data() + 12, &speed, 4);
	std::memcpy(data.data() + 16, &reach, 4);
	data[22] = 5;
	data[23] = 10;
	data[24] = 5;
	data[25] = 10;
	data[26] = 5;
	data[27] = 10;
	return data;
}

static std::string make_weap(uint16_t health)
{
	const auto subs =
	    make_sub("NAME", make_cstr("repro_sword")) + make_sub("FNAM", make_cstr("Repro Sword")) +
	    make_sub("WPDT", make_wpdt(health));
	return make_record("WEAP", subs);
}

static uint16_t read_merged_health(const std::string & merged_record)
{
	const auto pos = merged_record.find("WPDT");
	REQUIRE(pos != std::string::npos);

	const size_t data_offset = pos + 4 + 4 + 10;
	uint16_t health = 0;
	std::memcpy(&health, merged_record.data() + data_offset, 2);
	return health;
}

TEST_CASE("sub_record_merge_t::merge_bytes_three_way, multi-byte field byte-splices between two edits", "[u]")
{
	const std::string first(2, '\0');
	std::string base = first;
	base[0] = static_cast<char>(0x00);
	base[1] = static_cast<char>(0x01);

	std::string inter = base;
	inter[0] = static_cast<char>(0xFF);

	std::string winner = base;
	winner[1] = static_cast<char>(0x64);

	const auto merged =
	    sub_record_merge_t::merge_bytes_three_way(base.data(), inter.data(), winner.data(), base.size());

	uint16_t merged_value = 0;
	std::memcpy(&merged_value, merged.data(), 2);

	uint16_t inter_value = 0;
	uint16_t winner_value = 0;
	std::memcpy(&inter_value, inter.data(), 2);
	std::memcpy(&winner_value, winner.data(), 2);

	REQUIRE(merged_value != 0x64FF);
	REQUIRE((merged_value == inter_value || merged_value == winner_value));
}

TEST_CASE("sub_record_merge_t::merge, WEAP u16 field is not byte-spliced across two mods", "[u]")
{
	const auto base = make_weap(0x0100);
	const auto mod_low = make_weap(0x01FF);
	const auto mod_high = make_weap(0x6400);

	merge_input_t input;
	input.rec_type = "WEAP";
	input.record_id = "repro_sword";
	input.version_contents = { base, mod_low, mod_high };

	const auto result = sub_record_merge_t::merge(input);
	const auto health = read_merged_health(result.content);

	REQUIRE(health != 0x64FF);
	REQUIRE((health == 0x01FF || health == 0x6400));
}
