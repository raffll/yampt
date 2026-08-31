#include <catch2/catch_all.hpp>
#include <creator/topic_link_splitter.hpp>

TEST_CASE("topic_link_splitter::split, one link ordered segments", "[u]")
{
	const auto segments = topic_link_splitter::split("Talk to @Caius Cosades# now");

	REQUIRE(segments.size() == 3);
	REQUIRE_FALSE(segments[0].is_link);
	REQUIRE(segments[0].plain_text == "Talk to ");
	REQUIRE(segments[1].is_link);
	REQUIRE(segments[1].inner_phrase == "Caius Cosades");
	REQUIRE(segments[1].pseudo_asterisks == "");
	REQUIRE_FALSE(segments[2].is_link);
	REQUIRE(segments[2].plain_text == " now");
}

TEST_CASE("topic_link_splitter::split, single link at line start", "[u]")
{
	const auto segments = topic_link_splitter::split("@Blades# order");

	REQUIRE(segments.size() == 2);
	REQUIRE(segments[0].is_link);
	REQUIRE(segments[0].inner_phrase == "Blades");
	REQUIRE_FALSE(segments[1].is_link);
	REQUIRE(segments[1].plain_text == " order");
}

TEST_CASE("topic_link_splitter::split, multiple links order and count", "[u]")
{
	const auto segments = topic_link_splitter::split("Ask @Caius# about the @Blades# order");

	REQUIRE(segments.size() == 5);
	REQUIRE_FALSE(segments[0].is_link);
	REQUIRE(segments[0].plain_text == "Ask ");
	REQUIRE(segments[1].is_link);
	REQUIRE(segments[1].inner_phrase == "Caius");
	REQUIRE_FALSE(segments[2].is_link);
	REQUIRE(segments[2].plain_text == " about the ");
	REQUIRE(segments[3].is_link);
	REQUIRE(segments[3].inner_phrase == "Blades");
	REQUIRE_FALSE(segments[4].is_link);
	REQUIRE(segments[4].plain_text == " order");
}

TEST_CASE("topic_link_splitter::split, adjacent links preserve order and count", "[u]")
{
	const auto segments = topic_link_splitter::split("@one#@two#");

	REQUIRE(segments.size() == 2);
	REQUIRE(segments[0].is_link);
	REQUIRE(segments[0].inner_phrase == "one");
	REQUIRE(segments[1].is_link);
	REQUIRE(segments[1].inner_phrase == "two");
}

TEST_CASE("topic_link_splitter::split, pseudo asterisks separated from phrase", "[u]")
{
	const auto segments = topic_link_splitter::split("Kill @Fjol**# already");

	REQUIRE(segments.size() == 3);
	REQUIRE(segments[1].is_link);
	REQUIRE(segments[1].inner_phrase == "Fjol");
	REQUIRE(segments[1].pseudo_asterisks == "**");
}

TEST_CASE("topic_link_splitter::split, only asterisks inner keeps them as pseudo", "[u]")
{
	const auto segments = topic_link_splitter::split("@**#");

	REQUIRE(segments.size() == 1);
	REQUIRE(segments[0].is_link);
	REQUIRE(segments[0].inner_phrase == "");
	REQUIRE(segments[0].pseudo_asterisks == "**");
}

TEST_CASE("topic_link_splitter::split, lone at sign with no hash is one plain segment", "[u]")
{
	const auto segments = topic_link_splitter::split("Hello @world no close");

	REQUIRE(segments.size() == 1);
	REQUIRE_FALSE(segments[0].is_link);
	REQUIRE(segments[0].plain_text == "Hello @world no close");
}

TEST_CASE("topic_link_splitter::split, plain text with no link is one segment", "[u]")
{
	const auto segments = topic_link_splitter::split("just plain text");

	REQUIRE(segments.size() == 1);
	REQUIRE_FALSE(segments[0].is_link);
	REQUIRE(segments[0].plain_text == "just plain text");
}

TEST_CASE("topic_link_splitter::reassemble, one link round trip", "[u]")
{
	const std::string line = "Talk to @Caius Cosades# now";

	REQUIRE(topic_link_splitter::reassemble(topic_link_splitter::split(line)) == line);
}

TEST_CASE("topic_link_splitter::reassemble, multiple links round trip", "[u]")
{
	const std::string line = "Ask @Caius# about the @Blades# order";

	REQUIRE(topic_link_splitter::reassemble(topic_link_splitter::split(line)) == line);
}

TEST_CASE("topic_link_splitter::reassemble, pseudo asterisks preserved round trip", "[u]")
{
	const std::string line = "Kill @Fjol**# already";

	REQUIRE(topic_link_splitter::reassemble(topic_link_splitter::split(line)) == line);
}

TEST_CASE("topic_link_splitter::reassemble, mixed plain and link round trip", "[u]")
{
	const std::string line = "before @topic# middle @Fjol**# after";
	const auto segments = topic_link_splitter::split(line);

	REQUIRE(segments.size() == 5);
	REQUIRE(topic_link_splitter::reassemble(segments) == line);
}

TEST_CASE("topic_link_splitter::reassemble, substituted inner phrase rewraps with tags", "[u]")
{
	auto segments = topic_link_splitter::split("Talk to @Caius Cosades# now");
	segments[1].inner_phrase = "Caius Cosades PL";

	REQUIRE(topic_link_splitter::reassemble(segments) == "Talk to @Caius Cosades PL# now");
}

TEST_CASE("topic_link_splitter::reassemble, substituted inner phrase keeps asterisk suffix", "[u]")
{
	auto segments = topic_link_splitter::split("Kill @Fjol**# already");
	segments[1].inner_phrase = "Fjolem";

	REQUIRE(topic_link_splitter::reassemble(segments) == "Kill @Fjolem**# already");
}
