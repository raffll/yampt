#include <catch2/catch_all.hpp>
#include <decoder/sub_record_schema.hpp>

namespace {

int index_of(const std::string & record_type, const std::string & sub_type)
{
	const auto & composition = record_composition(record_type);
	for (size_t position = 0; position < composition.size(); ++position)
	{
		if (composition[position].sub_type == sub_type)
			return static_cast<int>(position);
	}

	return -1;
}

bool schema_has_sub_type(const std::string & sub_type)
{
	for (const auto & schema : all_schemas())
	{
		if (sub_type == schema.sub_type)
			return true;
	}

	return false;
}

const record_sub_record_t * entry_of(const std::string & record_type, const std::string & sub_type)
{
	for (const auto & entry : record_composition(record_type))
	{
		if (entry.sub_type == sub_type)
			return &entry;
	}

	return nullptr;
}

const std::vector<std::string> all_record_types = {
	"ACTI", "ALCH", "APPA", "ARMO", "BODY", "BOOK", "BSGN", "CLAS", "CLOT", "CONT", "CREA",
	"DOOR", "ENCH", "GLOB", "GMST", "INGR", "LIGH", "LOCK", "MGEF", "MISC", "NPC_",
	"PROB", "RACE", "REPA", "SKIL", "SNDG", "SOUN", "SPEL", "SSCR", "STAT", "WEAP",
	"DIAL", "INFO", "FACT", "LEVI", "LEVC", "REGN", "SCPT", "PGRD", "LAND", "CELL", "CELL@ref", "LTEX"
};

} // namespace

TEST_CASE("record_composition, every composed sub-record type has a schema", "[u]")
{
	for (const auto & record_type : all_record_types)
	{
		for (const auto & entry : record_composition(record_type))
		{
			INFO("composed sub-record without schema: " << record_type << " " << entry.sub_type);
			REQUIRE(schema_has_sub_type(entry.sub_type));
		}
	}
}

TEST_CASE("record_composition, unknown record type has empty composition", "[u]")
{
	REQUIRE(record_composition("ZZZZ").empty());
}

TEST_CASE("record_composition, kind flags match known sub-records", "[u]")
{
	REQUIRE(entry_of("NPC_", "NPCO")->kind == sub_record_kind_t::repeatable);
	REQUIRE(entry_of("NPC_", "NPDT")->kind == sub_record_kind_t::multi_value);
	REQUIRE(entry_of("NPC_", "NAME")->kind == sub_record_kind_t::single_value);
	REQUIRE(entry_of("FACT", "RNAM")->kind == sub_record_kind_t::repeatable);
	REQUIRE(entry_of("SPEL", "ENAM")->kind == sub_record_kind_t::repeatable);
	REQUIRE(entry_of("ARMO", "AODT")->kind == sub_record_kind_t::multi_value);
}

TEST_CASE("record_composition, labels are set for shared per-parent sub-records", "[u]")
{
	REQUIRE(std::string(entry_of("DOOR", "ANAM")->label) == "Close Sound");
	REQUIRE(std::string(entry_of("NPC_", "ANAM")->label) == "Faction");
	REQUIRE(std::string(entry_of("FACT", "ANAM")->label) == "Reaction Faction");
	REQUIRE(std::string(entry_of("ALCH", "TEXT")->label) == "Icon");
}

TEST_CASE("record_composition, MGEF and SKIL and SCPT have no NAME", "[u]")
{
	REQUIRE(index_of("MGEF", "NAME") < 0);
	REQUIRE(index_of("SKIL", "NAME") < 0);
	REQUIRE(index_of("SCPT", "NAME") < 0);
}
