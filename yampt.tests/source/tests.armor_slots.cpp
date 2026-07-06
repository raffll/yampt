#include <catch2/catch_all.hpp>
#include <decoder/conflict_slots.hpp>
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

static std::string make_string(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string make_body_part(uint32_t index, const std::string & male, const std::string & female)
{
	return make_sub("INDX", make_uint32(index)) + make_sub("BNAM", make_string(male)) +
	       make_sub("CNAM", make_string(female));
}

TEST_CASE("conflict_slots::build, aligns body parts by INDX value", "[u]")
{
	auto v1_subs = make_sub("NAME", make_string("armor_id")) + make_body_part(0, "head_m", "head_f") +
	               make_body_part(2, "chest_m", "chest_f");

	auto v2_subs = make_sub("NAME", make_string("armor_id")) + make_body_part(2, "chest_m_v2", "chest_f_v2") +
	               make_body_part(0, "head_m", "head_f");

	auto v1 = make_record("ARMO", v1_subs);
	auto v2 = make_record("ARMO", v2_subs);

	std::vector<bool> deleted = { false, false };
	auto result = conflict_slots::build("ARMO", { v1, v2 }, deleted);

	bool found_indx_0 = false;
	bool found_indx_2 = false;

	for (const auto & slot : result.aligned)
	{
		if (slot.key.type != "INDX")
			continue;

		bool has_v1 = slot.indices[0] != SIZE_MAX;
		bool has_v2 = slot.indices[1] != SIZE_MAX;

		if (has_v1 && has_v2)
		{
			uint32_t v1_val = 0;
			std::memcpy(&v1_val, result.parsed[0][slot.indices[0]].data, 4);

			uint32_t v2_val = 0;
			std::memcpy(&v2_val, result.parsed[1][slot.indices[1]].data, 4);

			REQUIRE(v1_val == v2_val);

			if (v1_val == 0)
				found_indx_0 = true;
			if (v1_val == 2)
				found_indx_2 = true;
		}
	}

	REQUIRE(found_indx_0);
	REQUIRE(found_indx_2);
}

TEST_CASE("conflict_slots::build, missing part shows SIZE_MAX", "[u]")
{
	auto v1_subs = make_sub("NAME", make_string("armor_id")) + make_body_part(0, "head_m", "head_f") +
	               make_body_part(1, "hair_m", "hair_f");

	auto v2_subs = make_sub("NAME", make_string("armor_id")) + make_body_part(0, "head_m", "head_f");

	auto v1 = make_record("ARMO", v1_subs);
	auto v2 = make_record("ARMO", v2_subs);

	std::vector<bool> deleted = { false, false };
	auto result = conflict_slots::build("ARMO", { v1, v2 }, deleted);

	bool found_missing = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type != "INDX")
			continue;

		if (slot.indices[0] != SIZE_MAX && slot.indices[1] == SIZE_MAX)
		{
			uint32_t val = 0;
			std::memcpy(&val, result.parsed[0][slot.indices[0]].data, 4);
			if (val == 1)
				found_missing = true;
		}
	}

	REQUIRE(found_missing);
}

TEST_CASE("conflict_slots::build, header sub-records not in body part groups", "[u]")
{
	auto subs = make_sub("NAME", make_string("armor_id")) + make_sub("FNAM", make_string("Display Name")) +
	            make_body_part(0, "head_m", "head_f");

	auto content = make_record("ARMO", subs);

	std::vector<bool> deleted = { false };
	auto result = conflict_slots::build("ARMO", { content }, deleted);

	bool found_name = false;
	bool found_fnam = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "NAME" && slot.indices[0] != SIZE_MAX)
			found_name = true;
		if (slot.key.type == "FNAM" && slot.indices[0] != SIZE_MAX)
			found_fnam = true;
	}

	REQUIRE(found_name);
	REQUIRE(found_fnam);
}
