#include <catch2/catch_all.hpp>
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
	const auto subs = make_sub("NAME", make_cstr("repro_sword")) + make_sub("FNAM", make_cstr("Repro Sword")) +
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

TEST_CASE("sub_record_merge_t::merge_fields_three_way, multi-byte field is not spliced across two edits", "[u]")
{
	const auto base = make_wpdt(0x0100);
	auto inter = make_wpdt(0x0100);
	inter[10] = static_cast<char>(0xFF); // low byte of Health only
	auto winner = make_wpdt(0x0100);
	winner[11] = static_cast<char>(0x64); // high byte of Health only

	const sub_record_merge_t::field_merge_input_t merge_input {
		"WEAP", "WPDT", base.data(), inter.data(), winner.data(), winner.data(), base.size()
	};
	const auto merged = sub_record_merge_t::merge_fields_three_way(merge_input);

	uint16_t merged_health = 0;
	std::memcpy(&merged_health, merged.data() + 10, 2);

	REQUIRE(merged_health != 0x64FF);
	REQUIRE(merged_health == 0x6400);
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

static std::string make_fadt(uint32_t rank1_attr1, uint32_t rank2_attr1)
{
	std::string data(240, '\0');
	std::memcpy(data.data() + 8, &rank1_attr1, 4);
	std::memcpy(data.data() + 28, &rank2_attr1, 4);
	return data;
}

static std::string make_fact(uint32_t rank1_attr1, uint32_t rank2_attr1)
{
	const auto subs = make_sub("NAME", make_cstr("test_faction")) + make_sub("FNAM", make_cstr("Test Faction")) +
	                  make_sub("FADT", make_fadt(rank1_attr1, rank2_attr1));
	return make_record("FACT", subs);
}

static uint32_t read_fadt_field(const std::string & merged_record, size_t field_offset)
{
	const auto pos = merged_record.find("FADT");
	REQUIRE(pos != std::string::npos);

	const size_t data_offset = pos + 4 + 4 + field_offset;
	uint32_t value = 0;
	std::memcpy(&value, merged_record.data() + data_offset, 4);
	return value;
}

TEST_CASE("sub_record_merge_t::merge, FACT FADT rank requirements merge per field across two mods", "[u]")
{
	const auto base = make_fact(10, 20);
	const auto mod_rank1 = make_fact(50, 20);
	const auto mod_rank2 = make_fact(10, 60);

	merge_input_t input;
	input.rec_type = "FACT";
	input.record_id = "test_faction";
	input.version_contents = { base, mod_rank1, mod_rank2 };

	const auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_fadt_field(result.content, 8) == 50);
	REQUIRE(read_fadt_field(result.content, 28) == 60);
}

static std::string make_bkdt(uint32_t value, uint32_t enchant_points)
{
	std::string data(20, '\0');
	const float weight = 2.0f;
	std::memcpy(data.data() + 0, &weight, 4);
	std::memcpy(data.data() + 4, &value, 4);
	std::memcpy(data.data() + 16, &enchant_points, 4);
	return data;
}

static std::string make_book(uint32_t value, uint32_t enchant_points)
{
	const auto subs = make_sub("NAME", make_cstr("tome")) + make_sub("FNAM", make_cstr("Tome")) +
	                  make_sub("BKDT", make_bkdt(value, enchant_points));
	return make_record("BOOK", subs);
}

static uint32_t read_bkdt_field(const std::string & merged_record, size_t field_offset)
{
	const auto pos = merged_record.find("BKDT");
	REQUIRE(pos != std::string::npos);

	uint32_t value = 0;
	std::memcpy(&value, merged_record.data() + pos + 8 + field_offset, 4);
	return value;
}

TEST_CASE("sub_record_merge_t::merge, BKDT two mods change different fields both land", "[u]")
{
	const auto base = make_book(100, 10);
	const auto mod_value = make_book(555, 10);
	const auto mod_enchant = make_book(100, 77);
	const auto winner = make_book(100, 10);

	merge_input_t input;
	input.rec_type = "BOOK";
	input.record_id = "tome";
	input.version_contents = { base, mod_value, mod_enchant, winner };

	const auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_bkdt_field(result.content, 4) == 555);
	REQUIRE(read_bkdt_field(result.content, 16) == 77);
}

static std::string make_npc_flag_record(uint32_t flags)
{
	const auto subs =
	    make_sub("NAME", make_cstr("guard")) +
	    make_sub("FLAG", std::string(reinterpret_cast<const char *>(&flags), 4));
	return make_record("NPC_", subs);
}

static uint32_t read_npc_flag(const std::string & merged_record)
{
	const auto pos = merged_record.find("FLAG");
	REQUIRE(pos != std::string::npos);

	uint32_t flags = 0;
	std::memcpy(&flags, merged_record.data() + pos + 8, 4);
	return flags;
}

TEST_CASE("sub_record_merge_t::merge, NPC FLAG two mods set different bits both land", "[u]")
{
	const auto base = make_npc_flag_record(0x0000);
	const auto mod_female = make_npc_flag_record(0x0001);
	const auto mod_essential = make_npc_flag_record(0x0002);
	const auto winner = make_npc_flag_record(0x0000);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "guard";
	input.version_contents = { base, mod_female, mod_essential, winner };

	const auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_npc_flag(result.content) == 0x0003);
}

static std::string make_stat_modl(const std::string & model)
{
	const auto subs = make_sub("NAME", make_cstr("rock")) + make_sub("MODL", make_cstr(model));
	return make_record("STAT", subs);
}

static std::string read_modl(const std::string & merged_record)
{
	const auto pos = merged_record.find("MODL");
	REQUIRE(pos != std::string::npos);

	uint32_t size = 0;
	std::memcpy(&size, merged_record.data() + pos + 4, 4);
	std::string value(merged_record.data() + pos + 8, size);
	const auto null_pos = value.find('\0');
	if (null_pos != std::string::npos)
		value.resize(null_pos);

	return value;
}

TEST_CASE("sub_record_merge_t::merge, MODL string last listed changer wins", "[u]")
{
	const auto base = make_stat_modl("base.nif");
	const auto mod_a = make_stat_modl("first.nif");
	const auto mod_b = make_stat_modl("second.nif");
	const auto winner = make_stat_modl("base.nif");

	merge_input_t input;
	input.rec_type = "STAT";
	input.record_id = "rock";
	input.version_contents = { base, mod_a, mod_b, winner };

	const auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_modl(result.content) == "second.nif");
}

TEST_CASE("sub_record_merge_t::merge, WPDT size mismatch intermediate falls back to whole value", "[u]")
{
	const auto base = make_weap(0x0100);
	auto shrunk_subs = make_sub("NAME", make_cstr("repro_sword")) + make_sub("FNAM", make_cstr("Repro Sword")) +
	                   make_sub("WPDT", std::string(16, '\1'));
	const auto mod_small = make_record("WEAP", shrunk_subs);
	const auto winner = make_weap(0x0100);

	merge_input_t input;
	input.rec_type = "WEAP";
	input.record_id = "repro_sword";
	input.version_contents = { base, mod_small, winner };

	const auto result = sub_record_merge_t::merge(input);

	const auto pos = result.content.find("WPDT");
	REQUIRE(pos != std::string::npos);
	uint32_t wpdt_size = 0;
	std::memcpy(&wpdt_size, result.content.data() + pos + 4, 4);
	REQUIRE(wpdt_size == 16);
}

static std::string make_spdt(uint32_t flags)
{
	std::string data(12, '\0');
	const uint32_t type = 0;
	const uint32_t cost = 5;
	std::memcpy(data.data() + 0, &type, 4);
	std::memcpy(data.data() + 4, &cost, 4);
	std::memcpy(data.data() + 8, &flags, 4);
	return data;
}

static std::string make_spel(uint32_t flags)
{
	const auto subs = make_sub("NAME", make_cstr("spell_id")) + make_sub("FNAM", make_cstr("Spell")) +
	                  make_sub("SPDT", make_spdt(flags));
	return make_record("SPEL", subs);
}

static uint32_t read_spdt_flags(const std::string & merged_record)
{
	const auto pos = merged_record.find("SPDT");
	REQUIRE(pos != std::string::npos);

	uint32_t flags = 0;
	std::memcpy(&flags, merged_record.data() + pos + 8 + 8, 4);
	return flags;
}

TEST_CASE("sub_record_merge_t::merge, SPEL SPDT two mods set different flag bits both land", "[u]")
{
	const auto base = make_spel(0x0000);
	const auto mod_bit1 = make_spel(0x0002);
	const auto mod_bit2 = make_spel(0x0004);
	const auto winner = make_spel(0x0000);

	merge_input_t input;
	input.rec_type = "SPEL";
	input.record_id = "spell_id";
	input.version_contents = { base, mod_bit1, mod_bit2, winner };

	const auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_spdt_flags(result.content) == 0x0006);
}
