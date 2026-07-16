#include <catch2/catch_all.hpp>
#include <io/dict_reader.hpp>
#include <io/dict_writer.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string temp_path(const std::string & name)
{
	auto path = (fs::temp_directory_path() / name).string();
	return string_utils::normalize_path(path);
}

static void cleanup(const std::string & path)
{
	app_logger_t::reset_log();
	std::error_code ec;
	fs::remove(path, ec);
}

TEST_CASE("dict_writer_t::write, empty dict round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_empty.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());
	REQUIRE(domain_types::get_number_of_elements_in_dict(reader.get_dict()) == 0);

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, multiple types round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_multi.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	dict.at(rec_type_t::cell).insert({ "key_cell", "Balmora", "Balmora_PL", status_t::translated });
	dict.at(rec_type_t::dial).insert({ "key_dial", "background", "tlo", status_t::translated });
	dict.at(rec_type_t::fnam).insert({ "key_fnam", "Iron Dagger", "Zelazny Sztylet", status_t::translated });
	dict.at(rec_type_t::info).insert({ "key_info", "Hello there", "Witaj", status_t::translated, "Fargoth", "M" });

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());
	REQUIRE(domain_types::get_number_of_elements_in_dict(reader.get_dict()) == 4);

	const auto * cell = reader.get_dict().at(rec_type_t::cell).find("key_cell");
	REQUIRE(cell != nullptr);
	REQUIRE(cell->old_text == "Balmora");
	REQUIRE(cell->new_text == "Balmora_PL");
	REQUIRE(cell->status == status_t::translated);

	const auto * info = reader.get_dict().at(rec_type_t::info).find("key_info");
	REQUIRE(info != nullptr);
	REQUIRE(info->speaker_name == "Fargoth");
	REQUIRE(info->gender == "M");

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, special characters round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_special.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	dict.at(rec_type_t::gmst)
	    .insert({ "key_quotes", "He said \"hello\"", "On powiedzial \"czesc\"", status_t::translated });
	dict.at(rec_type_t::gmst)
	    .insert({ "key_newlines", "Line1\r\nLine2\r\nLine3", "Linia1\r\nLinia2\r\nLinia3", status_t::translated });
	dict.at(rec_type_t::gmst).insert({ "key_backslash", "path\\to\\file", "sciezka\\do\\pliku", status_t::translated });
	dict.at(rec_type_t::gmst).insert({ "key_tab", "col1\tcol2", "kol1\tkol2", status_t::translated });

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());
	REQUIRE(domain_types::get_number_of_elements_in_dict(reader.get_dict()) == 4);

	const auto * quotes = reader.get_dict().at(rec_type_t::gmst).find("key_quotes");
	REQUIRE(quotes != nullptr);
	REQUIRE(quotes->old_text == "He said \"hello\"");
	REQUIRE(quotes->new_text == "On powiedzial \"czesc\"");

	const auto * newlines = reader.get_dict().at(rec_type_t::gmst).find("key_newlines");
	REQUIRE(newlines != nullptr);
	REQUIRE(newlines->old_text == "Line1\r\nLine2\r\nLine3");

	const auto * backslash = reader.get_dict().at(rec_type_t::gmst).find("key_backslash");
	REQUIRE(backslash != nullptr);
	REQUIRE(backslash->old_text == "path\\to\\file");

	const auto * tab = reader.get_dict().at(rec_type_t::gmst).find("key_tab");
	REQUIRE(tab != nullptr);
	REQUIRE(tab->old_text == "col1\tcol2");

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, adapted_from field round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_adapted.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	record_entry_t entry;
	entry.key_text = "key_adapted";
	entry.old_text = "Old Text";
	entry.new_text = "New Text";
	entry.status = status_t::adapted;
	entry.details = "Original Translation|Alternative";
	dict.at(rec_type_t::cell).insert(entry);

	record_entry_t ambiguous;
	ambiguous.key_text = "key_ambiguous";
	ambiguous.old_text = "Ambiguous";
	ambiguous.new_text = "Option A";
	ambiguous.status = status_t::ambiguous;
	ambiguous.details = "Option A / Option B / Option C";
	dict.at(rec_type_t::cell).insert(ambiguous);

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto * adapted = reader.get_dict().at(rec_type_t::cell).find("key_adapted");
	REQUIRE(adapted != nullptr);
	REQUIRE(adapted->details == "Original Translation|Alternative");

	const auto * amb = reader.get_dict().at(rec_type_t::cell).find("key_ambiguous");
	REQUIRE(amb != nullptr);
	REQUIRE(amb->details == "Option A / Option B / Option C");

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, enchantment field round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_enchant.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	record_entry_t entry;
	entry.key_text = "key_enchanted";
	entry.old_text = "Glass Dagger";
	entry.new_text = "Szklany Sztylet";
	entry.status = status_t::translated;
	entry.enchantment = "fire_damage_en";
	dict.at(rec_type_t::fnam).insert(entry);

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto * e = reader.get_dict().at(rec_type_t::fnam).find("key_enchanted");
	REQUIRE(e != nullptr);
	REQUIRE(e->enchantment == "fire_damage_en");

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, all statuses round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_statuses.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	const std::vector<status_t> statuses = {
		status_t::translated, status_t::reused,  status_t::untranslated, status_t::in_progress,
		status_t::outdated,   status_t::missing, status_t::duplicate,    status_t::mismatch,
		status_t::adapted,    status_t::changed, status_t::ambiguous,    status_t::model,
		status_t::propagated, status_t::error,   status_t::heuristic,    status_t::to_verify,
	};

	int idx = 0;
	for (const auto & status : statuses)
	{
		std::string key = "m_key" + std::to_string(idx++);
		dict.at(rec_type_t::gmst).insert({ key, "old", "new", status });
	}

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());
	REQUIRE(domain_types::get_number_of_elements_in_dict(reader.get_dict()) == statuses.size());

	idx = 0;
	for (const auto & status : statuses)
	{
		std::string key = "m_key" + std::to_string(idx++);
		const auto * e = reader.get_dict().at(rec_type_t::gmst).find(key);
		REQUIRE(e != nullptr);
		REQUIRE(e->status == status);
	}

	cleanup(path);
}

TEST_CASE("dict_reader_t::load, malformed JSON returns not loaded", "[i]")
{
	const auto path = temp_path("yampt_test_malformed.json");
	{
		std::ofstream f(path, std::ios::binary);
		f << "{ \"CELL\": [ { \"key\": \"k\", \"old\": \"o\", \"new\": \"n\" ";
	}
	app_logger_t::reset_log();

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded() == false);

	cleanup(path);
}

TEST_CASE("dict_reader_t::load, empty JSON object loaded no entries", "[i]")
{
	const auto path = temp_path("yampt_test_empty_obj.json");
	{
		std::ofstream f(path, std::ios::binary);
		f << "{}";
	}
	app_logger_t::reset_log();

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());
	REQUIRE(domain_types::get_number_of_elements_in_dict(reader.get_dict()) == 0);

	cleanup(path);
}

TEST_CASE("dict_merger_t::merge_dict, file-based last-listed wins", "[i]")
{
	const auto path1 = temp_path("yampt_test_merge1.json");
	const auto path2 = temp_path("yampt_test_merge2.json");
	app_logger_t::reset_log();

	dict_t dict1 = domain_types::initialize_dict();
	dict1.at(rec_type_t::cell).insert({ "Balmora", "Balmora", "First", status_t::translated });
	dict1.at(rec_type_t::cell).insert({ "Vivec", "Vivec", "OnlyInFirst", status_t::translated });
	dict_writer_t::write(dict1, path1);

	app_logger_t::reset_log();

	dict_t dict2 = domain_types::initialize_dict();
	dict2.at(rec_type_t::cell).insert({ "Balmora", "Balmora", "Second", status_t::translated });
	dict2.at(rec_type_t::cell).insert({ "Ald-ruhn", "Ald-ruhn", "OnlyInSecond", status_t::translated });
	dict_writer_t::write(dict2, path2);

	app_logger_t::reset_log();

	dict_merger_t merger({ path1, path2 });
	const auto & merged = merger.get_dict();

	const auto * balmora = merged.at(rec_type_t::cell).find("Balmora");
	REQUIRE(balmora != nullptr);
	REQUIRE(balmora->new_text == "Second");

	const auto * vivec = merged.at(rec_type_t::cell).find("Vivec");
	REQUIRE(vivec != nullptr);
	REQUIRE(vivec->new_text == "OnlyInFirst");

	const auto * aldruhn = merged.at(rec_type_t::cell).find("Ald-ruhn");
	REQUIRE(aldruhn != nullptr);
	REQUIRE(aldruhn->new_text == "OnlyInSecond");

	cleanup(path1);
	cleanup(path2);
}

TEST_CASE("dict_merger_t::merge_dict, three-file element count", "[i]")
{
	const auto path1 = temp_path("yampt_test_m3_1.json");
	const auto path2 = temp_path("yampt_test_m3_2.json");
	const auto path3 = temp_path("yampt_test_m3_3.json");
	app_logger_t::reset_log();

	dict_t d1 = domain_types::initialize_dict();
	d1.at(rec_type_t::cell).insert({ "A", "A", "A_val", status_t::translated });
	d1.at(rec_type_t::cell).insert({ "B", "B", "B_val", status_t::translated });
	dict_writer_t::write(d1, path1);
	app_logger_t::reset_log();

	dict_t d2 = domain_types::initialize_dict();
	d2.at(rec_type_t::cell).insert({ "B", "B", "B_override", status_t::translated });
	d2.at(rec_type_t::cell).insert({ "C", "C", "C_val", status_t::translated });
	dict_writer_t::write(d2, path2);
	app_logger_t::reset_log();

	dict_t d3 = domain_types::initialize_dict();
	d3.at(rec_type_t::dial).insert({ "D", "D", "D_val", status_t::translated });
	dict_writer_t::write(d3, path3);
	app_logger_t::reset_log();

	dict_merger_t merger({ path1, path2, path3 });
	REQUIRE(domain_types::get_number_of_elements_in_dict(merger.get_dict()) == 4);

	cleanup(path1);
	cleanup(path2);
	cleanup(path3);
}

TEST_CASE("dict_writer_t::write, raw codepage bytes survive round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_raw_bytes.json");
	app_logger_t::reset_log();

	std::string raw_1250 = "Zdr\xF3j \xB9\x9C\x9F";
	dict_t dict = domain_types::initialize_dict();
	dict.at(rec_type_t::cell).insert({ "key_raw", raw_1250, raw_1250, status_t::translated });

	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto * e = reader.get_dict().at(rec_type_t::cell).find("key_raw");
	REQUIRE(e != nullptr);
	REQUIRE(e->old_text == raw_1250);
	REQUIRE(e->new_text == raw_1250);

	cleanup(path);
}

TEST_CASE("dict_writer_t::write, script type round-trip", "[i]")
{
	const auto path = temp_path("yampt_test_script.json");

	dict_t dict = domain_types::initialize_dict();
	dict.at(rec_type_t::script).insert({ "TestScript", "Begin TestScript\r\n\tmessagebox \"Hello\"\r\nEnd", "", status_t::translated });

	dict_writer_t::write(dict, path);

	dict_reader_t reader(path);
	REQUIRE(reader.is_loaded());

	const auto * entry = reader.get_dict().at(rec_type_t::script).find("TestScript");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->old_text == "Begin TestScript\r\n\tmessagebox \"Hello\"\r\nEnd");
	REQUIRE(entry->new_text.empty());
	REQUIRE(entry->status == status_t::translated);

	cleanup(path);
}
