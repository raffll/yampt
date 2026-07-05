#include <catch2/catch_all.hpp>
#include <editor/glossary.hpp>

namespace {

tools_t::dict_t make_dict_with_dial(const std::vector<std::pair<std::string, std::string>> & entries)
{
	tools_t::dict_t dict;
	auto & chapter = dict[tools_t::rec_type_t::dial];
	for (const auto & [old_val, new_val] : entries)
	{
		tools_t::record_entry_t entry;
		entry.old_text = old_val;
		entry.new_text = new_val;
		entry.status = status_t::translated;
		chapter.records.push_back(std::move(entry));
	}
	return dict;
}

tools_t::dict_t make_dict_with_fnam(
    const std::vector<std::pair<std::string, std::string>> & entries,
    status_t status = status_t::translated)
{
	tools_t::dict_t dict;
	auto & chapter = dict[tools_t::rec_type_t::fnam];
	for (const auto & [old_val, new_val] : entries)
	{
		tools_t::record_entry_t entry;
		entry.old_text = old_val;
		entry.new_text = new_val;
		entry.status = status;
		chapter.records.push_back(std::move(entry));
	}
	return dict;
}

} // anonymous namespace

TEST_CASE("glossary_t::annotate, finds dial topic in text", "[u]")
{
	auto dict = make_dict_with_dial({ { "kwama forager", "zwiadowca kwama" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "test.json" } });

	const auto results = manager.annotate("I saw a kwama forager today", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 8);
	REQUIRE(results[0].end == 21);
	REQUIRE(results[0].kind == annotation_t::dial_topic);
	REQUIRE(results[0].new_text == "zwiadowca kwama");
	REQUIRE(results[0].source == "test.json");
}

TEST_CASE("glossary_t::annotate, case insensitive matching", "[u]")
{
	auto dict = make_dict_with_dial({ { "Balmora", "Balmora" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("Welcome to balmora", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 11);
	REQUIRE(results[0].end == 18);
}

TEST_CASE("glossary_t::annotate, no match inside word", "[u]")
{
	auto dict = make_dict_with_dial({ { "art", "sztuka" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("He started running", tools_t::rec_type_t::info);

	REQUIRE(results.empty());
}

TEST_CASE("glossary_t::annotate, empty text returns empty", "[u]")
{
	auto dict = make_dict_with_dial({ { "test", "test_pl" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("", tools_t::rec_type_t::info);

	REQUIRE(results.empty());
}

TEST_CASE("glossary_t::annotate, glossary does not overlap hyperlinks", "[u]")
{
	tools_t::dict_t dict;

	auto & dial_chapter = dict[tools_t::rec_type_t::dial];
	tools_t::record_entry_t dial_entry;
	dial_entry.old_text = "kwama forager";
	dial_entry.new_text = "zwiadowca kwama";
	dial_entry.status = status_t::translated;
	dial_chapter.records.push_back(std::move(dial_entry));

	auto & fnam_chapter = dict[tools_t::rec_type_t::fnam];
	tools_t::record_entry_t fnam_entry;
	fnam_entry.old_text = "kwama";
	fnam_entry.new_text = "kwama_pl";
	fnam_entry.status = status_t::translated;
	fnam_chapter.records.push_back(std::move(fnam_entry));

	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("the kwama forager attacks", tools_t::rec_type_t::info);

	bool has_topic = false;
	bool has_glossary_overlap = false;
	for (const auto & result : results)
	{
		if (result.kind == annotation_t::dial_topic && result.old_text == "kwama forager")
			has_topic = true;

		if (result.kind == annotation_t::glossary_term && result.start >= 4 && result.start < 17)
			has_glossary_overlap = true;
	}

	REQUIRE(has_topic);
	REQUIRE_FALSE(has_glossary_overlap);
}

TEST_CASE("glossary_t::annotate, multiple matches in text", "[u]")
{
	auto dict = make_dict_with_dial({ { "test", "test_pl" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("test and test again", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 2);
	REQUIRE(results[0].start == 0);
	REQUIRE(results[1].start == 9);
}

TEST_CASE("glossary_t::annotate, skips excluded statuses for glossary", "[u]")
{
	auto dict = make_dict_with_fnam({ { "Sword", "Miecz" } }, status_t::changed);
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("the Sword is sharp", tools_t::rec_type_t::info);

	REQUIRE(results.empty());
}

TEST_CASE("glossary_t::annotate, includes adapted status in glossary", "[u]")
{
	auto dict = make_dict_with_fnam({ { "Sword", "Miecz" } }, status_t::adapted);
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("the Sword is sharp", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].kind == annotation_t::glossary_term);
}

TEST_CASE("glossary_t::find_glossary_matches, word boundary check", "[u]")
{
	auto dict = make_dict_with_fnam({ { "sword", "miecz" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto matches = manager.find_glossary_matches("broadsword is not a sword");

	REQUIRE(matches.size() == 1);
	REQUIRE(matches[0].start == 20);
	REQUIRE(matches[0].length == 5);
	REQUIRE(matches[0].replacement == "miecz");
}

TEST_CASE("glossary_t::find_glossary_matches, no overlap between terms", "[u]")
{
	auto dict = make_dict_with_fnam({ { "long sword", "dlugi miecz" }, { "sword", "miecz" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto matches = manager.find_glossary_matches("a long sword here");

	REQUIRE(matches.size() == 1);
	REQUIRE(matches[0].replacement == "dlugi miecz");
}

TEST_CASE("glossary_t::apply_glossary, replaces matched terms", "[u]")
{
	auto dict = make_dict_with_fnam({ { "sword", "miecz" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto result = manager.apply_glossary("take the sword now");

	REQUIRE(result == "take the miecz now");
}

TEST_CASE("glossary_t::apply_glossary, replaces multiple occurrences", "[u]")
{
	auto dict = make_dict_with_fnam({ { "gold", "zloto" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto result = manager.apply_glossary("gold and more gold");

	REQUIRE(result == "zloto and more zloto");
}

TEST_CASE("glossary_t::apply_glossary, respects word boundaries", "[u]")
{
	auto dict = make_dict_with_fnam({ { "old", "stary" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto result = manager.apply_glossary("the golden ring is old");

	REQUIRE(result == "the golden ring is stary");
}

TEST_CASE("glossary_t::update_term, updates existing glossary entry", "[u]")
{
	auto dict = make_dict_with_fnam({ { "sword", "miecz" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	manager.update_term(tools_t::rec_type_t::fnam, "sword", "szpada");
	const auto result = manager.apply_glossary("take the sword");

	REQUIRE(result == "take the szpada");
}

TEST_CASE("glossary_t::update_term, removes entry when new equals old", "[u]")
{
	auto dict = make_dict_with_fnam({ { "sword", "miecz" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	manager.update_term(tools_t::rec_type_t::fnam, "sword", "sword");
	const auto result = manager.apply_glossary("take the sword");

	REQUIRE(result == "take the sword");
}

TEST_CASE("glossary_t::update_term, adds new dial topic", "[u]")
{
	tools_t::dict_t dict;
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	manager.update_term(tools_t::rec_type_t::dial, "thieves guild", "gildia zlodziei");
	const auto results = manager.annotate("join the thieves guild", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].kind == annotation_t::dial_topic);
	REQUIRE(results[0].new_text == "gildia zlodziei");
}

TEST_CASE("glossary_t::annotate_translated, finds translated topic in text", "[u]")
{
	auto dict = make_dict_with_dial({ { "kwama forager", "zwiadowca kwama" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate_translated("widzialem zwiadowca kwama", tools_t::rec_type_t::info);

	REQUIRE(results.size() == 1);
	REQUIRE(results[0].start == 10);
	REQUIRE(results[0].end == 25);
	REQUIRE(results[0].kind == annotation_t::dial_topic);
}

TEST_CASE("glossary_t::rebuild, longer terms match first", "[u]")
{
	auto dict = make_dict_with_dial({ { "ax", "topor" }, { "battle ax", "topor bojowy" } });
	glossary_t manager;
	manager.rebuild({ { &dict, "base.json" } });

	const auto results = manager.annotate("use the battle ax", tools_t::rec_type_t::info);

	bool found_long = false;
	for (const auto & result : results)
	{
		if (result.old_text == "battle ax")
			found_long = true;
	}
	REQUIRE(found_long);
}

TEST_CASE("glossary_t::get_speaker_gender, returns empty for unknown npc", "[u]")
{
	glossary_t manager;

	const auto & gender = manager.get_speaker_gender("unknown_npc_id");

	REQUIRE(gender.empty());
}

TEST_CASE("glossary_t::has_enchantment, returns false when empty", "[u]")
{
	glossary_t manager;

	REQUIRE_FALSE(manager.has_enchantment("some_enchantment"));
}
