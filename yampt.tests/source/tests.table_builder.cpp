#include <catch2/catch_all.hpp>
#include <model/table_builder.hpp>

static tools_t::dict_t make_single_entry_dict(
    tools_t::rec_type_t type,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    status_t status)
{
	tools_t::dict_t result;
	tools_t::record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;
	result[type].records.push_back(entry);
	return result;
}

static table_build_result_t build_filtered_rows(
    const tools_t::dict_t & data,
    const std::set<tools_t::rec_type_t> & type_filter,
    const std::set<std::string> & sub_type_filter,
    const std::set<status_t> & status_filter,
    const row_filter_t & search,
    bool type_filter_solo)
{
	const table_filter_params_t params { type_filter, sub_type_filter, status_filter, search, type_filter_solo };
	return ::build_filtered_rows(data, params);
}

TEST_CASE("build_filtered_rows, empty dict returns empty result", "[u]")
{
	tools_t::dict_t empty_dict;
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(empty_dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.empty());
	REQUIRE(result.counts.progress_total == 0);
	REQUIRE(result.counts.progress_translated == 0);
}

TEST_CASE("build_filtered_rows, single cell entry passes with no filters", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 1);
	REQUIRE(result.rows[0].old_text == "Balmora");
	REQUIRE(result.rows[0].type == tools_t::rec_type_t::cell);
}

TEST_CASE("build_filtered_rows, type filter excludes non-matching types", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter = { tools_t::rec_type_t::fnam };
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.empty());
}

TEST_CASE("build_filtered_rows, type filter includes matching type", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter = { tools_t::rec_type_t::cell };
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 1);
}

TEST_CASE("build_filtered_rows, status filter excludes non-matching status", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::untranslated);
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter = { status_t::translated };

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.empty());
}

TEST_CASE("build_filtered_rows, status filter includes matching status", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter = { status_t::translated };

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 1);
}

TEST_CASE("build_filtered_rows, search filter excludes non-matching text", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	row_filter_t::config_t config;
	config.query = "Vivec";
	search.set_config(config);
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.empty());
}

TEST_CASE("build_filtered_rows, search filter includes matching text", "[u]")
{
	auto dict = make_single_entry_dict(tools_t::rec_type_t::cell, "key_hash_01", "Balmora", "Balmora", status_t::translated);
	row_filter_t search;
	row_filter_t::config_t config;
	config.query = "Balmora";
	search.set_config(config);
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 1);
}

TEST_CASE("build_filtered_rows, counts translated entries correctly", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t entry_one;
	entry_one.key_text = "key_hash_01";
	entry_one.old_text = "Balmora";
	entry_one.new_text = "Balmora";
	entry_one.status = status_t::translated;
	dict[tools_t::rec_type_t::cell].records.push_back(entry_one);

	tools_t::record_entry_t entry_two;
	entry_two.key_text = "key_hash_02";
	entry_two.old_text = "Vivec";
	entry_two.new_text = "Vivec";
	entry_two.status = status_t::untranslated;
	dict[tools_t::rec_type_t::cell].records.push_back(entry_two);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.counts.translated_counts.at(tools_t::rec_type_t::cell) == 1);
	REQUIRE(result.counts.type_counts.at(tools_t::rec_type_t::cell) == 2);
}

TEST_CASE("build_filtered_rows, progress counts respect type filter", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t cell_entry;
	cell_entry.key_text = "key_hash_01";
	cell_entry.old_text = "Balmora";
	cell_entry.new_text = "Balmora";
	cell_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::cell].records.push_back(cell_entry);

	tools_t::record_entry_t fnam_entry;
	fnam_entry.key_text = "NPC_^SomeNPC";
	fnam_entry.old_text = "Farmer";
	fnam_entry.new_text = "Rolnik";
	fnam_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::fnam].records.push_back(fnam_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter = { tools_t::rec_type_t::cell };
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.counts.progress_total == 1);
	REQUIRE(result.counts.progress_translated == 1);
}

TEST_CASE("build_filtered_rows, bnam counts under info type", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t bnam_entry;
	bnam_entry.key_text = "T^topic^info_id^result";
	bnam_entry.old_text = "say \"hello\"";
	bnam_entry.new_text = "say \"czesc\"";
	bnam_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::bnam].records.push_back(bnam_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.counts.type_counts.count(tools_t::rec_type_t::info) == 1);
	REQUIRE(result.counts.type_counts.at(tools_t::rec_type_t::info) == 1);
}

TEST_CASE("build_filtered_rows, bnam interleaved after matching info", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t info_entry;
	info_entry.key_text = "T^topic^info_id";
	info_entry.old_text = "Hello traveler";
	info_entry.new_text = "Witaj podrozny";
	info_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(info_entry);

	tools_t::record_entry_t bnam_entry;
	bnam_entry.key_text = "T^topic^info_id^result";
	bnam_entry.old_text = "addtopic \"quest\"";
	bnam_entry.new_text = "addtopic \"zadanie\"";
	bnam_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::bnam].records.push_back(bnam_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 2);
	REQUIRE(result.rows[0].type == tools_t::rec_type_t::info);
	REQUIRE(result.rows[1].type == tools_t::rec_type_t::bnam);
	REQUIRE(result.rows[1].is_child == true);
}

TEST_CASE("build_filtered_rows, sub type counts for info records", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t topic_entry;
	topic_entry.key_text = "T^some_topic^info_123";
	topic_entry.old_text = "Hello";
	topic_entry.new_text = "Witaj";
	topic_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(topic_entry);

	tools_t::record_entry_t voice_entry;
	voice_entry.key_text = "V^voice_greeting^info_456";
	voice_entry.old_text = "Greetings";
	voice_entry.new_text = "Pozdrowienia";
	voice_entry.status = status_t::untranslated;
	dict[tools_t::rec_type_t::info].records.push_back(voice_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.counts.sub_type_total_counts.at("Topic") == 1);
	REQUIRE(result.counts.sub_type_total_counts.at("Voice") == 1);
	REQUIRE(result.counts.sub_type_translated_counts.at("Topic") == 1);
	REQUIRE(result.counts.sub_type_translated_counts.count("Voice") == 0);
}

TEST_CASE("build_filtered_rows, filtered status counts ignore status filter", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t entry_one;
	entry_one.key_text = "key_hash_01";
	entry_one.old_text = "Balmora";
	entry_one.new_text = "Balmora";
	entry_one.status = status_t::translated;
	dict[tools_t::rec_type_t::cell].records.push_back(entry_one);

	tools_t::record_entry_t entry_two;
	entry_two.key_text = "key_hash_02";
	entry_two.old_text = "Vivec";
	entry_two.new_text = "Vivec";
	entry_two.status = status_t::untranslated;
	dict[tools_t::rec_type_t::cell].records.push_back(entry_two);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter = { status_t::translated };

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 1);
	REQUIRE(result.counts.filtered_status_counts.at(status_t::translated) == 1);
	REQUIRE(result.counts.filtered_status_counts.at(status_t::untranslated) == 1);
}

TEST_CASE("build_filtered_rows, sub type filter active when solo mode", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t topic_entry;
	topic_entry.key_text = "T^some_topic^info_123";
	topic_entry.old_text = "Hello";
	topic_entry.new_text = "Witaj";
	topic_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(topic_entry);

	tools_t::record_entry_t voice_entry;
	voice_entry.key_text = "V^voice_greeting^info_456";
	voice_entry.old_text = "Greetings";
	voice_entry.new_text = "Pozdrowienia";
	voice_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(voice_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter = { tools_t::rec_type_t::info };
	std::set<std::string> sub_type_filter = { "Topic" };
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, true);

	REQUIRE(result.rows.size() == 1);
	REQUIRE(result.rows[0].old_text == "Hello");
}

TEST_CASE("build_filtered_rows, sub type filter inactive when not solo", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t topic_entry;
	topic_entry.key_text = "T^some_topic^info_123";
	topic_entry.old_text = "Hello";
	topic_entry.new_text = "Witaj";
	topic_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(topic_entry);

	tools_t::record_entry_t voice_entry;
	voice_entry.key_text = "V^voice_greeting^info_456";
	voice_entry.old_text = "Greetings";
	voice_entry.new_text = "Pozdrowienia";
	voice_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::info].records.push_back(voice_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter = { tools_t::rec_type_t::info };
	std::set<std::string> sub_type_filter = { "Topic" };
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 2);
}

TEST_CASE("build_filtered_rows, multiple types counted independently", "[u]")
{
	tools_t::dict_t dict;
	tools_t::record_entry_t cell_entry;
	cell_entry.key_text = "key_hash_01";
	cell_entry.old_text = "Balmora";
	cell_entry.new_text = "Balmora";
	cell_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::cell].records.push_back(cell_entry);

	tools_t::record_entry_t gmst_entry;
	gmst_entry.key_text = "sDefaultCellname";
	gmst_entry.old_text = "Wilderness";
	gmst_entry.new_text = "Dzicz";
	gmst_entry.status = status_t::translated;
	dict[tools_t::rec_type_t::gmst].records.push_back(gmst_entry);

	row_filter_t search;
	std::set<tools_t::rec_type_t> type_filter;
	std::set<std::string> sub_type_filter;
	std::set<status_t> status_filter;

	const auto & result = build_filtered_rows(dict, type_filter, sub_type_filter, status_filter, search, false);

	REQUIRE(result.rows.size() == 2);
	REQUIRE(result.counts.type_counts.at(tools_t::rec_type_t::cell) == 1);
	REQUIRE(result.counts.type_counts.at(tools_t::rec_type_t::gmst) == 1);
}
