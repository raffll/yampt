#include <catch2/catch_all.hpp>
#include "../yampt/tools.hpp"
#include "../yampt/dict_merger.hpp"

TEST_CASE("dict_merger_t::add_record, inserts entry", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, "Balmora", "BalmoraTrans");
	const auto & dict = merger.get_dict();
	const auto * entry = dict.at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "BalmoraTrans");
}
