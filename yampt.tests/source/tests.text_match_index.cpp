#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <creator/text_match_index.hpp>
#include <set>

TEST_CASE("text_match_index_t::find, not found for absent keys", "[u][pbt]")
{
	rc::prop(
		"querying a key never inserted returns not_found",
		[]()
	{
		const auto entry_count = *rc::gen::inRange(1, 10);
		tools_t::dict_t dict;
		auto & chapter = dict[tools_t::rec_type_t::fnam];

		std::set<std::string> inserted_texts;
		for (int i = 0; i < entry_count; ++i)
		{
			auto old_text = "old_" + std::to_string(i) + "_" + std::to_string(*rc::gen::inRange(0, 10000));
			auto new_text = "new_" + std::to_string(i) + "_" + std::to_string(*rc::gen::inRange(0, 10000));

			tools_t::record_entry_t entry;
			entry.key_text = "key_" + std::to_string(i);
			entry.old_text = old_text;
			entry.new_text = new_text;
			entry.status = status_t::translated;
			chapter.records.push_back(std::move(entry));
			inserted_texts.insert(old_text);
		}

		std::string query = "NEVER_INSERTED_" + std::to_string(*rc::gen::inRange(0, 100000));
		while (inserted_texts.count(query))
			query += "Q";

		text_match_index_t index;
		index.build(dict);

		const auto outcome = index.find(query);
		RC_ASSERT(outcome.result == text_match_index_t::find_result_t::not_found);
	});
}

TEST_CASE("text_match_index_t::find, found for unique entries", "[u][pbt]")
{
	rc::prop(
		"unique old_text with different new_text returns found",
		[]()
	{
		const auto entry_count = *rc::gen::inRange(1, 10);
		tools_t::dict_t dict;
		auto & chapter = dict[tools_t::rec_type_t::fnam];

		std::vector<std::pair<std::string, std::string>> pairs;
		std::set<std::string> used_texts;

		for (int i = 0; i < entry_count; ++i)
		{
			std::string old_text = "old_unique_" + std::to_string(i);
			while (used_texts.count(old_text))
				old_text += "x";
			used_texts.insert(old_text);

			std::string new_text = "translated_" + std::to_string(i);

			tools_t::record_entry_t entry;
			entry.key_text = "key_" + std::to_string(i);
			entry.old_text = old_text;
			entry.new_text = new_text;
			entry.status = status_t::translated;
			chapter.records.push_back(std::move(entry));
			pairs.emplace_back(old_text, new_text);
		}

		text_match_index_t index;
		index.build(dict);

		const auto which = *rc::gen::inRange(0, entry_count);
		const auto & [query, expected_translation] = pairs[which];
		const auto outcome = index.find(query);

		RC_ASSERT(outcome.result == text_match_index_t::find_result_t::found);
		RC_ASSERT(outcome.translation == expected_translation);
	});
}

TEST_CASE("text_match_index_t::find, ambiguous for conflicts", "[u][pbt]")
{
	rc::prop(
		"same old_text with different new_text values returns ambiguous",
		[]()
	{
		std::string old_text = "shared_old_" + std::to_string(*rc::gen::inRange(0, 10000));
		std::string new_text_1 = "translation_A_" + std::to_string(*rc::gen::inRange(0, 10000));
		std::string new_text_2 = "translation_B_" + std::to_string(*rc::gen::inRange(0, 10000));
		RC_PRE(new_text_1 != new_text_2);
		RC_PRE(new_text_1 != old_text);
		RC_PRE(new_text_2 != old_text);

		tools_t::dict_t dict;
		auto & chapter = dict[tools_t::rec_type_t::fnam];

		tools_t::record_entry_t entry_1;
		entry_1.key_text = "key_1";
		entry_1.old_text = old_text;
		entry_1.new_text = new_text_1;
		entry_1.status = status_t::translated;
		chapter.records.push_back(std::move(entry_1));

		tools_t::record_entry_t entry_2;
		entry_2.key_text = "key_2";
		entry_2.old_text = old_text;
		entry_2.new_text = new_text_2;
		entry_2.status = status_t::translated;
		chapter.records.push_back(std::move(entry_2));

		text_match_index_t index;
		index.build(dict);

		const auto outcome = index.find(old_text);
		RC_ASSERT(outcome.result == text_match_index_t::find_result_t::ambiguous);
		RC_ASSERT(outcome.translation == new_text_1);
		RC_ASSERT(outcome.conflicts.find(new_text_1) != std::string::npos);
		RC_ASSERT(outcome.conflicts.find(new_text_2) != std::string::npos);
	});
}

TEST_CASE("text_match_index_t::find, skips untranslated entries", "[u][pbt]")
{
	rc::prop(
		"entries with old_text == new_text are not indexed",
		[]()
	{
		std::string text = "identical_" + std::to_string(*rc::gen::inRange(0, 10000));

		tools_t::dict_t dict;
		auto & chapter = dict[tools_t::rec_type_t::fnam];

		tools_t::record_entry_t entry;
		entry.key_text = "key_identical";
		entry.old_text = text;
		entry.new_text = text;
		entry.status = status_t::translated;
		chapter.records.push_back(std::move(entry));

		text_match_index_t index;
		index.build(dict);

		const auto outcome = index.find(text);
		RC_ASSERT(outcome.result == text_match_index_t::find_result_t::not_found);
	});
}
