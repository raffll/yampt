#include <catch2/catch_all.hpp>
#include <io/dict_writer.hpp>
#include <model/dict_document.hpp>
#include <model/yaml_document.hpp>
#include <session/session.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <fstream>

namespace {

std::string create_session_dict(const std::string & filename, const dict_t & data)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_session_test" / filename).string();
	fs::create_directories(fs::path(path).parent_path());
	path = string_utils::normalize_path(path);
	app_logger_t::reset_log();
	dict_writer_t::write(data, path);
	app_logger_t::reset_log();
	return path;
}

void cleanup_session()
{
	namespace fs = std::filesystem;
	std::error_code error_code;
	fs::remove_all(fs::temp_directory_path() / "yampt_session_test", error_code);
}

dict_t make_minimal_dict()
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "cell_key";
	entry.old_text = "Old Cell";
	entry.new_text = "Old Cell";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));
	return data;
}

} // anonymous namespace

TEST_CASE("session_t::open, loads dict document by json extension", "[i][qt]")
{
	const auto path = create_session_dict("test_dict.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	auto * doc = session.open(path);

	REQUIRE(doc != nullptr);
	REQUIRE(doc->kind() == document_kind_t::dict);
	REQUIRE(doc->total_count() == 1);

	cleanup_session();
}

TEST_CASE("session_t::open, loads yaml document by yaml extension", "[i][qt]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_session_test";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "key: value\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	session_t session(codepage_t::windows_1252);
	session.set_native_language("pl");

	auto * doc = session.open(pl_path);

	REQUIRE(doc != nullptr);
	REQUIRE(doc->kind() == document_kind_t::yaml);

	cleanup_session();
}

TEST_CASE("session_t::open, returns same document on duplicate open", "[i][qt]")
{
	const auto path = create_session_dict("dup_test.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	auto * first = session.open(path);
	auto * second = session.open(path);

	REQUIRE(first == second);
	REQUIRE(session.count() == 1);

	cleanup_session();
}

TEST_CASE("session_t::close, removes document", "[i][qt]")
{
	const auto path = create_session_dict("close_test.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	session.open(path);
	REQUIRE(session.count() == 1);

	session.close(path);
	REQUIRE(session.count() == 0);
	REQUIRE(session.find(path) == nullptr);

	cleanup_session();
}

TEST_CASE("session_t::close, increments dict_version for dict documents", "[i][qt]")
{
	const auto path = create_session_dict("ver_test.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	const auto version_before = session.dict_version();
	session.open(path);
	const auto version_after_open = session.dict_version();

	session.close(path);
	const auto version_after_close = session.dict_version();

	REQUIRE(version_after_open > version_before);
	REQUIRE(version_after_close > version_after_open);

	cleanup_session();
}

TEST_CASE("session_t::has_any_unsaved, false for clean documents", "[i][qt]")
{
	const auto path = create_session_dict("clean_test.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);
	session.open(path);

	REQUIRE_FALSE(session.has_any_unsaved());

	cleanup_session();
}

TEST_CASE("session_t::has_any_unsaved, true after commit", "[i][qt]")
{
	const auto path = create_session_dict("dirty_test.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	auto * doc = session.open(path);
	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "cell_key";
	row.old_text = "Old Cell";
	row.record_index = 0;

	doc->commit(row, "New Cell", status_t::in_progress);

	REQUIRE(session.has_any_unsaved());

	cleanup_session();
}

TEST_CASE("session_t::all_dicts, returns only dict documents", "[i][qt]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_session_test";
	fs::create_directories(temp_dir);

	const auto dict_path = create_session_dict("dict.json", make_minimal_dict());

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "key: value\n";
	const auto yaml_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(yaml_path) << "";

	session_t session(codepage_t::windows_1252);
	session.set_native_language("pl");
	session.open(dict_path);
	session.open(yaml_path);

	const auto dicts = session.all_dicts();
	REQUIRE(dicts.size() == 1);
	REQUIRE(dicts[0]->kind() == document_kind_t::dict);

	cleanup_session();
}

TEST_CASE("session_t::open, detects BASE dict kind from filename", "[i][qt]")
{
	const auto path = create_session_dict("Morrowind_BASE_EN-PL.json", make_minimal_dict());
	session_t session(codepage_t::windows_1252);

	auto * doc = session.open(path);
	auto * dict_doc = dynamic_cast<dict_document_t *>(doc);

	REQUIRE(dict_doc != nullptr);
	REQUIRE(dict_doc->dict_kind() == dict_kind_t::base);

	cleanup_session();
}
