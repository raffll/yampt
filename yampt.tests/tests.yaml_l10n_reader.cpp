#include <catch2/catch_all.hpp>
#include "../yampt.translator/io/yaml_l10n_reader.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {

std::string create_temp_yaml(const std::string & filename, const std::string & content)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / filename).string();
	std::replace(path.begin(), path.end(), '\\', '/');

	std::ofstream out(path, std::ios::binary);
	out << content;
	return path;
}

void cleanup_file(const std::string & path)
{
	std::error_code ec;
	std::filesystem::remove(path, ec);
}

} // anonymous namespace

TEST_CASE("yaml_l10n_reader_t::load, simple key-value pairs", "[i]")
{
	const auto path = create_temp_yaml("yaml_reader_simple.yaml",
	    "greeting: Hello\n"
	    "farewell: Goodbye\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(path);

	REQUIRE(loaded == true);
	REQUIRE(reader.source_entries().size() == 2);
	REQUIRE(reader.source_entries()[0].key == "greeting");
	REQUIRE(reader.source_entries()[0].value == "Hello");
	REQUIRE(reader.source_entries()[1].key == "farewell");
	REQUIRE(reader.source_entries()[1].value == "Goodbye");

	cleanup_file(path);
}

TEST_CASE("yaml_l10n_reader_t::load, block scalar with pipe", "[i]")
{
	const auto path = create_temp_yaml("yaml_reader_block.yaml",
	    "description: |\n"
	    "  First line\n"
	    "  Second line\n"
	    "other: value\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(path);

	REQUIRE(loaded == true);
	REQUIRE(reader.source_entries().size() == 2);
	REQUIRE(reader.source_entries()[0].key == "description");
	REQUIRE(reader.source_entries()[0].value == "First line\nSecond line");
	REQUIRE(reader.source_entries()[1].key == "other");
	REQUIRE(reader.source_entries()[1].value == "value");

	cleanup_file(path);
}

TEST_CASE("yaml_l10n_reader_t::load, pipe-minus block scalar", "[i]")
{
	const auto path = create_temp_yaml("yaml_reader_strip.yaml",
	    "message: |-\n"
	    "  Hello world\n"
	    "  End of message\n"
	    "next_key: after\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(path);

	REQUIRE(loaded == true);
	REQUIRE(reader.source_entries().size() == 2);
	REQUIRE(reader.source_entries()[0].key == "message");
	REQUIRE(reader.source_entries()[0].value == "Hello world\nEnd of message");

	cleanup_file(path);
}

TEST_CASE("yaml_l10n_reader_t::load, quoted strings", "[i]")
{
	const auto path = create_temp_yaml("yaml_reader_quoted.yaml",
	    "title: \"A quoted value\"\n"
	    "plain: unquoted value\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(path);

	REQUIRE(loaded == true);
	REQUIRE(reader.source_entries().size() == 2);
	REQUIRE(reader.source_entries()[0].key == "title");
	REQUIRE(reader.source_entries()[0].value == "A quoted value");
	REQUIRE(reader.source_entries()[1].key == "plain");
	REQUIRE(reader.source_entries()[1].value == "unquoted value");

	cleanup_file(path);
}

TEST_CASE("yaml_l10n_reader_t::load, preserves key order", "[i]")
{
	const auto path = create_temp_yaml("yaml_reader_order.yaml",
	    "zebra: last\n"
	    "alpha: first\n"
	    "middle: center\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(path);

	REQUIRE(loaded == true);
	REQUIRE(reader.key_order().size() == 3);
	REQUIRE(reader.key_order()[0] == "zebra");
	REQUIRE(reader.key_order()[1] == "alpha");
	REQUIRE(reader.key_order()[2] == "middle");

	cleanup_file(path);
}

TEST_CASE("yaml_l10n_reader_t::load, source and target files", "[i]")
{
	const auto source_path = create_temp_yaml("yaml_reader_source.yaml",
	    "greeting: Hello\n"
	    "farewell: Bye\n");

	const auto target_path = create_temp_yaml("yaml_reader_target.yaml",
	    "greeting: Hallo\n"
	    "farewell: Tschuss\n");

	yaml_l10n_reader_t reader;
	const auto loaded = reader.load(source_path, target_path);

	REQUIRE(loaded == true);
	REQUIRE(reader.source_entries().size() == 2);
	REQUIRE(reader.target_entries().size() == 2);
	REQUIRE(reader.source_entries()[0].value == "Hello");
	REQUIRE(reader.target_entries()[0].value == "Hallo");

	cleanup_file(source_path);
	cleanup_file(target_path);
}

TEST_CASE("yaml_l10n_reader_t::load, nonexistent file returns false", "[u]")
{
	yaml_l10n_reader_t reader;
	const auto loaded = reader.load("c:/nonexistent/path/missing_file.yaml");

	REQUIRE(loaded == false);
	REQUIRE(reader.source_entries().empty());
}
