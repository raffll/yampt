#include <catch2/catch_all.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>

TEST_CASE("dict_merger_t::add_record, inserts entry", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "Balmora", "BalmoraTrans");
	const auto & dict = merger.get_dict();
	const auto * entry = dict.at(rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "BalmoraTrans");
}

TEST_CASE("dict_merger_t::add_record, first-wins for duplicate keys", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "Balmora", "First");
	merger.add_record(rec_type_t::cell, "Balmora", "Second");

	const auto * entry = merger.get_dict().at(rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "First");
}

TEST_CASE("dict_merger_t::add_record, different types same key", "[u]")
{
	dict_merger_t merger;
	merger.add_record(rec_type_t::cell, "Balmora", "CellTranslation");
	merger.add_record(rec_type_t::fnam, "Balmora", "FnamTranslation");

	const auto * cell_entry = merger.get_dict().at(rec_type_t::cell).find("Balmora");
	const auto * fnam_entry = merger.get_dict().at(rec_type_t::fnam).find("Balmora");
	REQUIRE(cell_entry != nullptr);
	REQUIRE(fnam_entry != nullptr);
	REQUIRE(cell_entry->new_text == "CellTranslation");
	REQUIRE(fnam_entry->new_text == "FnamTranslation");
}

TEST_CASE("dict_merger_t, default constructor creates empty dict", "[u]")
{
	dict_merger_t merger;
	REQUIRE(domain_types::get_number_of_elements_in_dict(merger.get_dict()) == 0);
}
