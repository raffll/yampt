#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_writer.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const std::string reader_test_dir = "tests/reader";

TEST_CASE("dict_reader setup", "[u][reader]")
{
	fs::create_directories(reader_test_dir);
	REQUIRE(fs::is_directory(reader_test_dir));
}

TEST_CASE("dict_reader parses all new status values", "[u][reader]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();

	dict.at(tools_t::rec_type_t::cell).insert({ "cell_1", "Balmora", "Balmora", tools_t::status_t::missing });
	dict.at(tools_t::rec_type_t::cell).insert({ "cell_2", "Vivec", "Vivec", tools_t::status_t::duplicate });
	dict.at(tools_t::rec_type_t::cell)
	    .insert({ "cell_3", "Ald-ruhn", "Ald-ruhn", tools_t::status_t::matched_by_coords });
	dict.at(tools_t::rec_type_t::cell)
	    .insert({ "cell_4", "Sadrith Mora", "Sadrith Mora", tools_t::status_t::wilderness });
	dict.at(tools_t::rec_type_t::cell).insert({ "cell_5", "Gnisis", "Gnisis", tools_t::status_t::region });
	dict.at(tools_t::rec_type_t::dial).insert({ "dial_1", "topic", "temat", tools_t::status_t::matched_by_info });
	dict.at(tools_t::rec_type_t::dial).insert({ "dial_2", "hello", "hello", tools_t::status_t::matched_by_name });
	dict.at(tools_t::rec_type_t::info).insert({ "info_1", "Hello", "Hello", tools_t::status_t::auto_identical });
	dict.at(tools_t::rec_type_t::info).insert({ "info_2", "Goodbye", "Do widzenia", tools_t::status_t::auto_base });
	dict.at(tools_t::rec_type_t::info).insert({ "info_3", "Yes", "Tak", tools_t::status_t::auto_translated });
	dict.at(tools_t::rec_type_t::info).insert({ "info_4", "No", "Nie", tools_t::status_t::auto_heuristic });
	dict.at(tools_t::rec_type_t::info).insert({ "info_5", "Maybe", "Może", tools_t::status_t::auto_changed });
	dict.at(tools_t::rec_type_t::info).insert({ "info_6", "Quest", "Quest", tools_t::status_t::untranslated });
	dict.at(tools_t::rec_type_t::info).insert({ "info_7", "Done", "Gotowe", tools_t::status_t::in_progress });
	dict.at(tools_t::rec_type_t::info).insert({ "info_8", "Final", "Końcowy", tools_t::status_t::translated });
	dict.at(tools_t::rec_type_t::info).insert({ "info_9", "Bad", "Źle", tools_t::status_t::has_errors });

	auto path = reader_test_dir + "/all_statuses.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();

	auto * e1 = loaded.at(tools_t::rec_type_t::cell).find("cell_1");
	REQUIRE(e1 != nullptr);
	REQUIRE(e1->status == tools_t::status_t::missing);

	auto * e2 = loaded.at(tools_t::rec_type_t::cell).find("cell_2");
	REQUIRE(e2 != nullptr);
	REQUIRE(e2->status == tools_t::status_t::duplicate);

	auto * e3 = loaded.at(tools_t::rec_type_t::cell).find("cell_3");
	REQUIRE(e3 != nullptr);
	REQUIRE(e3->status == tools_t::status_t::matched_by_coords);

	auto * e4 = loaded.at(tools_t::rec_type_t::dial).find("dial_1");
	REQUIRE(e4 != nullptr);
	REQUIRE(e4->status == tools_t::status_t::matched_by_info);

	auto * e5 = loaded.at(tools_t::rec_type_t::info).find("info_1");
	REQUIRE(e5 != nullptr);
	REQUIRE(e5->status == tools_t::status_t::auto_identical);

	auto * e6 = loaded.at(tools_t::rec_type_t::info).find("info_2");
	REQUIRE(e6 != nullptr);
	REQUIRE(e6->status == tools_t::status_t::auto_base);

	auto * e7 = loaded.at(tools_t::rec_type_t::info).find("info_3");
	REQUIRE(e7 != nullptr);
	REQUIRE(e7->status == tools_t::status_t::auto_translated);

	auto * e8 = loaded.at(tools_t::rec_type_t::info).find("info_7");
	REQUIRE(e8 != nullptr);
	REQUIRE(e8->status == tools_t::status_t::in_progress);

	auto * e9 = loaded.at(tools_t::rec_type_t::info).find("info_8");
	REQUIRE(e9 != nullptr);
	REQUIRE(e9->status == tools_t::status_t::translated);

	auto * e10 = loaded.at(tools_t::rec_type_t::info).find("info_9");
	REQUIRE(e10 != nullptr);
	REQUIRE(e10->status == tools_t::status_t::has_errors);
}

TEST_CASE("dict_reader parses speaker fields from info entries", "[u][reader]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();

	dict.at(tools_t::rec_type_t::info)
	    .insert({ "info_speaker", "Hello there", "Witaj", "translated", "fargoth", "Fargoth", "M" });
	dict.at(tools_t::rec_type_t::info)
	    .insert({ "info_no_speaker", "Goodbye", "Do widzenia", "translated", "", "", "" });

	auto path = reader_test_dir + "/speaker_fields.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();

	auto * with_speaker = loaded.at(tools_t::rec_type_t::info).find("info_speaker");
	REQUIRE(with_speaker != nullptr);
	REQUIRE(with_speaker->speaker == "fargoth");
	REQUIRE(with_speaker->speaker_name == "Fargoth");
	REQUIRE(with_speaker->gender == "M");

	auto * no_speaker = loaded.at(tools_t::rec_type_t::info).find("info_no_speaker");
	REQUIRE(no_speaker != nullptr);
	REQUIRE(no_speaker->speaker.empty());
	REQUIRE(no_speaker->speaker_name.empty());
	REQUIRE(no_speaker->gender.empty());
}

TEST_CASE("dict_reader skips legacy npc_flag chapter", "[u][reader]")
{
	std::string json_content = R"({
  "encoding": "windows-1252",
  "CELL": [
    { "key": "Balmora", "old": "Balmora", "new": "Balmora", "status": "untranslated" }
  ],
  "NPC_FLAG": [
    { "key": "fargoth", "old": "1", "new": "1", "status": "" }
  ]
})";

	auto path = reader_test_dir + "/legacy_npc_flag.json";
	std::ofstream file(path, std::ios::binary);
	file << json_content;
	file.close();

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();
	auto * cell_entry = loaded.at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(cell_entry != nullptr);
}

TEST_CASE("dict_reader write-read round trip preserves all fields", "[u][reader]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();

	dict.at(tools_t::rec_type_t::info)
	    .insert(
	        { "info_rt", "Hello adventurer", "Witaj wędrowcze", "auto_base", "caius_cosades", "Caius Cosades", "M" });
	dict.at(tools_t::rec_type_t::cell)
	    .insert({ "cell_rt", "Balmora, Guild of Fighters", "Balmora, Gildia Wojowników", "matched_by_coords" });

	auto path = reader_test_dir + "/round_trip.json";
	dict_writer_t::write(dict, path);

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto & loaded = reader.get_dict();

	auto * info_entry = loaded.at(tools_t::rec_type_t::info).find("info_rt");
	REQUIRE(info_entry != nullptr);
	REQUIRE(info_entry->key_text == "info_rt");
	REQUIRE(info_entry->old_text == "Hello adventurer");
	REQUIRE(info_entry->new_text == "Witaj wędrowcze");
	REQUIRE(info_entry->status == "auto_base");
	REQUIRE(info_entry->speaker == "caius_cosades");
	REQUIRE(info_entry->speaker_name == "Caius Cosades");
	REQUIRE(info_entry->gender == "M");

	auto * cell_entry = loaded.at(tools_t::rec_type_t::cell).find("cell_rt");
	REQUIRE(cell_entry != nullptr);
	REQUIRE(cell_entry->old_text == "Balmora, Guild of Fighters");
	REQUIRE(cell_entry->new_text == "Balmora, Gildia Wojowników");
	REQUIRE(cell_entry->status == "matched_by_coords");
	REQUIRE(cell_entry->speaker.empty());
}
