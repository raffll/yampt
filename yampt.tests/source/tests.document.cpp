#include <catch2/catch_all.hpp>
#include <io/dict_writer.hpp>
#include <model/dict_document.hpp>
#include <rapidcheck/catch.h>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <rapidcheck.h>
#include <set>

namespace {

std::string create_temp_dict_json(const dict_t & data)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_test_dict.json").string();
	path = string_utils::normalize_path(path);

	app_logger_t::reset_log();
	dict_writer_t::write(data, path);
	app_logger_t::reset_log();
	return path;
}

void cleanup_temp_dict(const std::string & path)
{
	app_logger_t::reset_log();
	std::error_code ec;
	std::filesystem::remove(path, ec);
}

} // anonymous namespace

TEST_CASE("dict_document_t::path, round-trip", "[i]")
{
	rc::prop(
	    "path() returns normalized path",
	    []()
	{
		dict_t data;
		auto & chapter = data[rec_type_t::cell];
		record_entry_t entry;
		entry.key_text = "test_key";
		entry.old_text = "old";
		entry.new_text = "new";
		entry.status = status_t::untranslated;
		chapter.records.push_back(std::move(entry));

		const auto path = create_temp_dict_json(data);
		dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

		auto expected = string_utils::normalize_path(path);
		RC_ASSERT(doc.path() == expected);

		cleanup_temp_dict(path);
	});
}

TEST_CASE("dict_document_t::build_rows, count invariant", "[i]")
{
	rc::prop(
	    "build_rows().size() == total_count()",
	    []()
	{
		dict_t data;

		const auto type_count = *rc::gen::inRange(0, 4);
		const std::vector<rec_type_t> types = {
			rec_type_t::cell, rec_type_t::dial, rec_type_t::info, rec_type_t::fnam
		};

		for (int t = 0; t < type_count; ++t)
		{
			const auto rec_count = *rc::gen::inRange(0, 20);
			auto & chapter = data[types[t]];
			for (int r = 0; r < rec_count; ++r)
			{
				record_entry_t entry;
				entry.key_text = "m_key" + std::to_string(t) + "_" + std::to_string(r);
				entry.old_text = *rc::gen::arbitrary<std::string>();
				entry.new_text = *rc::gen::arbitrary<std::string>();
				entry.status = status_t::untranslated;
				chapter.records.push_back(std::move(entry));
			}
		}

		const auto path = create_temp_dict_json(data);
		dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

		const auto rows = doc.build_rows();
		RC_ASSERT(static_cast<int>(rows.size()) == doc.total_count());

		cleanup_temp_dict(path);
	});
}

TEST_CASE("dict_document_t::commit_edit, modifies data", "[i]")
{
	rc::prop(
	    "commit_edit changes the target record new_text",
	    []()
	{
		dict_t data;
		auto & chapter = data[rec_type_t::cell];

		const auto rec_count = *rc::gen::inRange(1, 10);
		for (int i = 0; i < rec_count; ++i)
		{
			record_entry_t entry;
			entry.key_text = "m_key" + std::to_string(i);
			entry.old_text = "old_" + std::to_string(i);
			entry.new_text = "new_" + std::to_string(i);
			entry.status = status_t::untranslated;
			chapter.records.push_back(std::move(entry));
		}

		const auto path = create_temp_dict_json(data);
		dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::base);

		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(rec_count));
		const auto new_text = *rc::gen::arbitrary<std::string>();
		doc.commit_edit(rec_type_t::cell, idx, new_text);

		const auto rows_after = doc.build_rows();
		RC_ASSERT(rows_after[idx].new_text == new_text);
		RC_ASSERT(doc.is_dirty());

		cleanup_temp_dict(path);
	});
}

#include <model/yaml_document.hpp>
#include <filesystem>
#include <fstream>

namespace {

std::string create_temp_yaml(const std::vector<std::pair<std::string, std::string>> & entries)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_test_doc.yaml").string();
	path = string_utils::normalize_path(path);

	std::ofstream out(path);
	for (const auto & [key, value] : entries)
		out << key << ": " << value << "\n";

	return path;
}

void cleanup_temp_yaml(const std::string & path)
{
	std::error_code ec;
	std::filesystem::remove(path, ec);
	std::filesystem::remove(path + ".tmp", ec);
}

} // anonymous namespace

TEST_CASE("yaml_document_t::path, round-trip", "[i]")
{
	rc::prop(
	    "path() returns normalized file path",
	    []()
	{
		const auto entries =
		    std::vector<std::pair<std::string, std::string>> { { "key1", "value1" }, { "key2", "value2" } };
		const auto path = create_temp_yaml(entries);

		yaml_document_t doc(path, "xx");
		RC_ASSERT(doc.path() == path);

		cleanup_temp_yaml(path);
	});
}

TEST_CASE("yaml_document_t::is_dirty, state consistency", "[i]")
{
	rc::prop(
	    "set_dirty round-trips; commit_edit sets dirty",
	    []()
	{
		const auto count = *rc::gen::inRange(1, 10);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({ "m_key" + std::to_string(i), "val_" + std::to_string(i) });

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path, "xx");

		const auto b = *rc::gen::arbitrary<bool>();
		doc.set_dirty(b);
		RC_ASSERT(doc.is_dirty() == b);

		doc.set_dirty(false);
		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
		doc.commit_edit(rec_type_t::yaml, idx, "edited");
		RC_ASSERT(doc.is_dirty() == true);

		cleanup_temp_yaml(path);
	});
}

TEST_CASE("yaml_document_t::commit_edit, round-trip", "[i]")
{
	rc::prop(
	    "after commit_edit, build_rows()[idx].new_text == text",
	    []()
	{
		const auto count = *rc::gen::inRange(1, 10);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({ "m_key" + std::to_string(i), "val_" + std::to_string(i) });

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path, "xx");

		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
		const auto new_text = *rc::gen::nonEmpty(rc::gen::arbitrary<std::string>());
		doc.commit_edit(rec_type_t::yaml, idx, new_text);

		const auto rows = doc.build_rows();
		RC_ASSERT(rows[idx].new_text == new_text);

		cleanup_temp_yaml(path);
	});
}

TEST_CASE("yaml_document_t::translated_count, invariant", "[i]")
{
	rc::prop(
	    "after K distinct edits, translated_count() == K",
	    []()
	{
		const auto count = *rc::gen::inRange(2, 15);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({ "m_key" + std::to_string(i), "val_" + std::to_string(i) });

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path, "xx");

		const auto edit_count = *rc::gen::inRange(1, count);
		std::set<size_t> edited;
		for (int e = 0; e < edit_count; ++e)
		{
			const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
			doc.commit_edit(rec_type_t::yaml, idx, "edited_" + std::to_string(e));
			edited.insert(idx);
		}

		RC_ASSERT(doc.translated_count() == static_cast<int>(edited.size()));

		cleanup_temp_yaml(path);
	});
}

TEST_CASE("yaml_document_t::is_native_file, foreign file detection", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_native_test";
	fs::create_directories(temp_dir);

	const auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream(en_path) << "greeting: Hello\n";

	yaml_document_t doc(en_path, "pl");
	REQUIRE(doc.is_native_file() == false);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::is_native_file, native file detection", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_native_test2";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "greeting: Hello\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "greeting: Czesc\n";

	yaml_document_t doc(pl_path, "pl");
	REQUIRE(doc.is_native_file() == true);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::permissions, foreign file is exportable", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_perm_test";
	fs::create_directories(temp_dir);

	const auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream(en_path) << "key: value\n";

	yaml_document_t doc(en_path, "pl");
	const auto perms = doc.permissions();
	REQUIRE(perms.exportable == true);
	REQUIRE(perms.editable == false);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::permissions, native file is editable", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_perm_test2";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "key: value\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "key: wartosc\n";

	yaml_document_t doc(pl_path, "pl");
	const auto perms = doc.permissions();
	REQUIRE(perms.exportable == false);
	REQUIRE(perms.editable == true);
	REQUIRE(perms.saveable == true);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::export_native, creates native file", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_export_test";
	fs::create_directories(temp_dir);

	const auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream(en_path) << "greeting: Hello\nfarewell: Goodbye\n";

	yaml_document_t doc(en_path, "de");
	doc.export_native();

	const auto de_path = (temp_dir / "de.yaml").string();
	REQUIRE(fs::exists(de_path));

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::total_count, matches key count", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_count_test";
	fs::create_directories(temp_dir);

	const auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream(en_path) << "alpha: one\nbeta: two\ngamma: three\n";

	yaml_document_t doc(en_path, "pl");
	REQUIRE(doc.total_count() == 3);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("dict_document_t::is_dirty, false after construction", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::fnam];
	record_entry_t entry;
	entry.key_text = "test_key";
	entry.old_text = "original";
	entry.new_text = "translated";
	entry.status = status_t::translated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	REQUIRE(doc.is_dirty() == false);
	REQUIRE(doc.is_read_only() == false);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::permissions, always editable", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "cell_key";
	entry.old_text = "old";
	entry.new_text = "new";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);

	dict_document_t user_doc(path, codepage_t::windows_1252, dict_kind_t::user);
	REQUIRE(user_doc.permissions().editable == true);

	dict_document_t base_doc(path, codepage_t::windows_1252, dict_kind_t::base);
	REQUIRE(base_doc.permissions().editable == true);

	cleanup_temp_dict(path);
}
