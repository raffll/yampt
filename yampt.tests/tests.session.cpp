#include <catch2/catch_all.hpp>
#include "../yampt.translator/session.hpp"
#include "../yampt.translator/model/dict_document.hpp"
#include "../yampt.translator/model/plugin_document.hpp"
#include "../yampt.translator/model/yaml_document.hpp"
#include "../yampt/io/dict_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {

std::string create_temp_json(const std::string & filename)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / filename).string();
	std::replace(path.begin(), path.end(), '\\', '/');

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry;
	entry.key_text = "test_key";
	entry.old_text = "old_text";
	entry.new_text = "new_text";
	entry.status = "translated";
	chapter.records.push_back(std::move(entry));

	tools_t::reset_log();
	dict_writer_t::write(data, path);
	tools_t::reset_log();
	return path;
}

std::string create_temp_yaml(const std::string & filename)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / filename).string();
	std::replace(path.begin(), path.end(), '\\', '/');

	std::ofstream out(path);
	out << "greeting: Hello\n";
	out << "farewell: Goodbye\n";
	return path;
}

std::string create_temp_esp(const std::string & filename)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / filename).string();
	std::replace(path.begin(), path.end(), '\\', '/');

	std::ofstream out(path, std::ios::binary);
	out << "TES3";
	const char zeroes[12] = {};
	out.write(zeroes, 12);
	return path;
}

void cleanup_file(const std::string & path)
{
	std::error_code ec;
	std::filesystem::remove(path, ec);
	std::filesystem::remove(path + ".tmp", ec);
}

} // anonymous namespace

TEST_CASE("session_t::open, json extension creates dict_document_t", "[i][qt]")
{
	const auto path = create_temp_json("session_test_dict.json");
	session_t session(codepage_t::windows_1252);

	auto * document = session.open(path);
	REQUIRE(document != nullptr);
	REQUIRE(dynamic_cast<dict_document_t *>(document) != nullptr);

	cleanup_file(path);
}

TEST_CASE("session_t::open, esp extension creates plugin_document_t", "[i][qt]")
{
	const auto path = create_temp_esp("session_test_plugin.esp");
	session_t session(codepage_t::windows_1252);

	auto * document = session.open(path);
	REQUIRE(document != nullptr);
	REQUIRE(dynamic_cast<plugin_document_t *>(document) != nullptr);

	cleanup_file(path);
}

TEST_CASE("session_t::open, yaml extension creates yaml_document_t", "[i][qt]")
{
	const auto path = create_temp_yaml("session_test_l10n.yaml");
	session_t session(codepage_t::windows_1252);

	auto * document = session.open(path);
	REQUIRE(document != nullptr);
	REQUIRE(dynamic_cast<yaml_document_t *>(document) != nullptr);

	cleanup_file(path);
}

TEST_CASE("session_t::open, unknown extension returns nullptr", "[u][qt]")
{
	session_t session(codepage_t::windows_1252);

	auto * document = session.open("some/path/file.bmp");
	REQUIRE(document == nullptr);
}

TEST_CASE("session_t::open, same path twice returns existing document", "[i][qt]")
{
	const auto path = create_temp_json("session_test_dup.json");
	session_t session(codepage_t::windows_1252);

	auto * first_open = session.open(path);
	auto * second_open = session.open(path);

	REQUIRE(first_open == second_open);
	REQUIRE(session.count() == 1);

	cleanup_file(path);
}

TEST_CASE("session_t::close, removes document and find returns nullptr", "[i][qt]")
{
	const auto path = create_temp_json("session_test_close.json");
	session_t session(codepage_t::windows_1252);

	session.open(path);
	REQUIRE(session.find(path) != nullptr);

	session.close(path);
	REQUIRE(session.find(path) == nullptr);
	REQUIRE(session.count() == 0);

	cleanup_file(path);
}

TEST_CASE("session_t::close_if, removes matching documents", "[i][qt]")
{
	const auto path_alpha = create_temp_json("session_closeif_a.json");
	const auto path_beta = create_temp_yaml("session_closeif_b.yaml");
	session_t session(codepage_t::windows_1252);

	session.open(path_alpha);
	session.open(path_beta);
	REQUIRE(session.count() == 2);

	session.close_if([](const document_t & doc) { return dynamic_cast<const yaml_document_t *>(&doc) != nullptr; });

	REQUIRE(session.count() == 1);
	REQUIRE(session.find(path_alpha) != nullptr);
	REQUIRE(session.find(path_beta) == nullptr);

	cleanup_file(path_alpha);
	cleanup_file(path_beta);
}

TEST_CASE("session_t::all_dicts, returns only dict documents", "[i][qt]")
{
	const auto path_dict = create_temp_json("session_alldicts.json");
	const auto path_yaml = create_temp_yaml("session_alldicts.yaml");
	const auto path_plugin = create_temp_esp("session_alldicts.esp");
	session_t session(codepage_t::windows_1252);

	session.open(path_dict);
	session.open(path_yaml);
	session.open(path_plugin);

	const auto & dicts = session.all_dicts();
	REQUIRE(dicts.size() == 1);
	REQUIRE(dicts[0]->path() == path_dict);

	cleanup_file(path_dict);
	cleanup_file(path_yaml);
	cleanup_file(path_plugin);
}

TEST_CASE("session_t::has_any_unsaved, false when no docs are dirty", "[i][qt]")
{
	const auto path = create_temp_json("session_unsaved.json");
	session_t session(codepage_t::windows_1252);

	session.open(path);
	REQUIRE(session.has_any_unsaved() == false);

	cleanup_file(path);
}

TEST_CASE("session_t::dict_version, increments on dict open and close", "[i][qt]")
{
	const auto path = create_temp_json("session_version.json");
	session_t session(codepage_t::windows_1252);

	const auto initial_version = session.dict_version();
	session.open(path);
	REQUIRE(session.dict_version() == initial_version + 1);

	session.close(path);
	REQUIRE(session.dict_version() == initial_version + 2);

	cleanup_file(path);
}
