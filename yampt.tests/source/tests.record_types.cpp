#include <catch2/catch_all.hpp>
#include <utility/record_types.hpp>

TEST_CASE("record_types::is_gendered_fnam_record, true for NPC_ only", "[u]")
{
	REQUIRE(record_types::is_gendered_fnam_record("NPC_"));

	REQUIRE_FALSE(record_types::is_gendered_fnam_record("CREA"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record("ARMO"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record("WEAP"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record("CLOT"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record("BOOK"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record("CELL"));
	REQUIRE_FALSE(record_types::is_gendered_fnam_record(""));
}
