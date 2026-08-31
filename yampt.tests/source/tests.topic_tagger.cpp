#include <catch2/catch_all.hpp>
#include <creator/topic_tagger.hpp>

TEST_CASE("topic_tagger_t::strip_tags, empty line unchanged", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("") == "");
}
TEST_CASE("topic_tagger_t::strip_tags, single link", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("Talk to @Caius Cosades# now") == "Talk to Caius Cosades now");
}

TEST_CASE("topic_tagger_t::strip_tags, multiple links", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("Ask @Caius# about the @Blades# order")
		== "Ask Caius about the Blades order");
}

TEST_CASE("topic_tagger_t::strip_tags, malformed lone at sign unchanged", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("Hello @world") == "Hello @world");
}

TEST_CASE("topic_tagger_t::strip_tags, text outside links preserved", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("before @topic# after") == "before topic after");
}

TEST_CASE("topic_tagger_t::strip_tags, pseudo asterisk inner text preserved", "[u]")
{
	REQUIRE(topic_tagger_t::strip_tags("@topic*#") == "topic*");
}

namespace
{
topic_tagger_t make_tagger(const std::vector<std::pair<std::string, std::string>> & dial_topics)
{
	dict_t dict;
	auto & chapter = dict[rec_type_t::dial];
	for (const auto & [old_text, new_text] : dial_topics)
		chapter.insert({ old_text, old_text, new_text, status_t::translated });

	topic_tagger_t tagger;
	tagger.seed_topics(dict);
	return tagger;
}
}

TEST_CASE("topic_tagger_t::tag_line, single topic wrapped", "[u]")
{
	const auto tagger = make_tagger({ { "Caius Cosades", "Caius Cosades" } });

	const auto result = tagger.tag_line("Talk to Caius Cosades now");

	REQUIRE(result.text == "Talk to @Caius Cosades# now");
	REQUIRE(result.tags_inserted == 1);
}

TEST_CASE("topic_tagger_t::tag_line, multiple topics wrapped", "[u]")
{
	const auto tagger = make_tagger({ { "Caius", "Caius" }, { "Blades", "Blades" } });

	const auto result = tagger.tag_line("Ask Caius about the Blades order");

	REQUIRE(result.text == "Ask @Caius# about the @Blades# order");
	REQUIRE(result.tags_inserted == 2);
}

TEST_CASE("topic_tagger_t::tag_line, substring inside a word not tagged", "[u]")
{
	const auto tagger = make_tagger({ { "orc", "orc" } });

	const auto result = tagger.tag_line("Use the Force wisely");

	REQUIRE(result.text == "Use the Force wisely");
	REQUIRE(result.tags_inserted == 0);
}

TEST_CASE("topic_tagger_t::tag_line, longest match wins", "[u]")
{
	const auto tagger = make_tagger({ { "Caius", "Caius" }, { "Caius Cosades", "Caius Cosades" } });

	const auto result = tagger.tag_line("Find Caius Cosades today");

	REQUIRE(result.text == "Find @Caius Cosades# today");
	REQUIRE(result.tags_inserted == 1);
}

TEST_CASE("topic_tagger_t::tag_line, idempotent re-run", "[u]")
{
	const auto tagger = make_tagger({ { "Caius", "Caius" } });

	const auto first = tagger.tag_line("Talk to Caius now");
	const auto second = tagger.tag_line(first.text);

	REQUIRE(first.text == "Talk to @Caius# now");
	REQUIRE(second.text == first.text);
	REQUIRE(second.tags_inserted == first.tags_inserted);
}

TEST_CASE("topic_tagger_t::tag_line, pre-existing tags stripped and reinserted", "[u]")
{
	const auto tagger = make_tagger({ { "Blades", "Blades" } });

	const auto result = tagger.tag_line("Join the @Caius# and the Blades");

	REQUIRE(result.text == "Join the Caius and the @Blades#");
	REQUIRE(result.tags_inserted == 1);
}

TEST_CASE("topic_tagger_t::tag_line, only DIAL topics seeded", "[u]")
{
	dict_t dict;
	dict[rec_type_t::cell].insert({ "Balmora", "Balmora", "Balmora", status_t::translated });
	dict[rec_type_t::fnam].insert({ "npc_fargoth", "Fargoth", "Fargoth", status_t::translated });

	topic_tagger_t tagger;
	tagger.seed_topics(dict);

	const auto result = tagger.tag_line("Meet Fargoth in Balmora");

	REQUIRE(result.text == "Meet Fargoth in Balmora");
	REQUIRE(result.tags_inserted == 0);
}

namespace
{
dict_t make_apply_dict(const std::vector<std::pair<std::string, std::string>> & dial_topics,
	const std::vector<record_entry_t> & translatable)
{
	dict_t dict;
	auto & dial_chapter = dict[rec_type_t::dial];
	for (const auto & [old_text, new_text] : dial_topics)
		dial_chapter.insert({ old_text, old_text, new_text, status_t::translated });

	auto & info_chapter = dict[rec_type_t::info];
	for (const auto & entry : translatable)
		info_chapter.insert(entry);

	return dict;
}
}

TEST_CASE("topic_tagger_t::apply_topic_tags, counts changed entries and inserted tags", "[u]")
{
	auto dict = make_apply_dict(
		{ { "Caius Cosades", "Caius Cosades" }, { "Blades", "Blades" } },
		{ { "info_one", "Talk to Caius Cosades now", "Talk to Caius Cosades now", status_t::translated },
			{ "info_two", "Join the Blades order", "Join the Blades order", status_t::translated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 2);
	REQUIRE(result.tags_inserted == 2);
	REQUIRE(dict[rec_type_t::info].records[0].new_text == "Talk to @Caius Cosades# now");
	REQUIRE(dict[rec_type_t::info].records[1].new_text == "Join the @Blades# order");
}

TEST_CASE("topic_tagger_t::apply_topic_tags, non-matching entries untouched", "[u]")
{
	auto dict = make_apply_dict(
		{ { "Blades", "Blades" } },
		{ { "info_one", "Nothing to tag here", "Nothing to tag here", status_t::translated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 0);
	REQUIRE(result.tags_inserted == 0);
	REQUIRE(dict[rec_type_t::info].records[0].new_text == "Nothing to tag here");
}

TEST_CASE("topic_tagger_t::apply_topic_tags, non-translated entries skipped", "[u]")
{
	auto dict = make_apply_dict(
		{ { "Caius", "Caius" } },
		{ { "info_one", "Find Caius today", "Find Caius today", status_t::untranslated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 0);
	REQUIRE(result.tags_inserted == 0);
	REQUIRE(dict[rec_type_t::info].records[0].new_text == "Find Caius today");
}

TEST_CASE("topic_tagger_t::apply_topic_tags, entries below minimum length skipped", "[u]")
{
	auto dict = make_apply_dict(
		{ { "a", "a" } },
		{ { "info_one", "a", "a", status_t::translated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 0);
	REQUIRE(result.tags_inserted == 0);
	REQUIRE(dict[rec_type_t::info].records[0].new_text == "a");
}

TEST_CASE("topic_tagger_t::apply_topic_tags, whitespace-only entries skipped", "[u]")
{
	auto dict = make_apply_dict(
		{ { "Caius", "Caius" } },
		{ { "info_one", "   ", "   ", status_t::translated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 0);
	REQUIRE(result.tags_inserted == 0);
}

TEST_CASE("topic_tagger_t::apply_topic_tags, refresh strips and reinserts existing tags", "[u]")
{
	auto dict = make_apply_dict(
		{ { "Blades", "Blades" } },
		{ { "info_one", "Meet @Caius# and the Blades", "Meet @Caius# and the Blades", status_t::translated } });

	const auto result = apply_topic_tags(dict);

	REQUIRE(result.entries_changed == 1);
	REQUIRE(result.tags_inserted == 1);
	REQUIRE(dict[rec_type_t::info].records[0].new_text == "Meet Caius and the @Blades#");
}
