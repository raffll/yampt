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

std::string create_temp_yaml_native(const std::vector<std::pair<std::string, std::string>> & entries)
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_doc_test";
	fs::create_directories(temp_dir);

	auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream en_out(en_path);
	for (const auto & [key, value] : entries)
		en_out << key << ": " << value << "\n";

	auto native_path = string_utils::normalize_path((temp_dir / "xx.yaml").string());
	std::ofstream(native_path) << "";

	return native_path;
}

void cleanup_temp_yaml(const std::string & path)
{
	namespace fs = std::filesystem;
	std::error_code ec;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_doc_test";
	fs::remove_all(temp_dir, ec);
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
		const auto path = create_temp_yaml_native(entries);

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

		const auto path = create_temp_yaml_native(entries);
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

		const auto path = create_temp_yaml_native(entries);
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

		const auto path = create_temp_yaml_native(entries);
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

TEST_CASE("dict_document_t::commit, intent in_progress sets status", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "key_a";
	entry.old_text = "Original";
	entry.new_text = "Original";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Original";
	row.record_index = 0;

	const auto result = doc.commit(row, "Translated", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::in_progress);
	REQUIRE(result.new_text == "Translated");
	REQUIRE(doc.is_dirty());

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, intent model sets status", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::info];
	record_entry_t entry;
	entry.key_text = "key_b";
	entry.old_text = "Hello";
	entry.new_text = "Hello";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::info;
	row.key_text = "key_b";
	row.old_text = "Hello";
	row.record_index = 0;

	const auto result = doc.commit(row, "Czesc", status_t::model);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::model);
	REQUIRE(result.new_text == "Czesc");

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, propagates to matching entries", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Same Text";
	entry_a.new_text = "Same Text";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Same Text";
	entry_b.new_text = "Same Text";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Same Text";
	row.record_index = 0;

	const auto result = doc.commit(row, "New Value", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 1);
	REQUIRE(doc.data().at(rec_type_t::cell).records[1].new_text == "New Value");
	REQUIRE(doc.data().at(rec_type_t::cell).records[1].status == status_t::propagated);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, model intent skips propagation", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::fnam];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Shared";
	entry_a.new_text = "Shared";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Shared";
	entry_b.new_text = "Shared";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::fnam;
	row.key_text = "key_a";
	row.old_text = "Shared";
	row.record_index = 0;

	const auto result = doc.commit(row, "Model Result", status_t::model);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 0);
	REQUIRE(doc.data().at(rec_type_t::fnam).records[1].new_text == "Shared");

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, invalid record_index fails", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "key_a";
	entry.old_text = "Text";
	entry.new_text = "Text";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "bad";
	row.old_text = "x";
	row.record_index = 999;

	const auto result = doc.commit(row, "Anything", status_t::in_progress);

	REQUIRE_FALSE(result.success);
	REQUIRE_FALSE(doc.is_dirty());

	cleanup_temp_dict(path);
}

TEST_CASE("yaml_document_t::commit, intent in_progress sets status", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_commit_test1";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "greeting: Hello\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	yaml_document_t doc(pl_path, "pl");

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "greeting";
	row.old_text = "Hello";
	row.record_index = 0;

	const auto result = doc.commit(row, "Czesc", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::in_progress);
	REQUIRE(result.new_text == "Czesc");
	REQUIRE(doc.is_dirty());

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::commit, intent model sets status", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_commit_test2";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "farewell: Goodbye\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	yaml_document_t doc(pl_path, "pl");

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "farewell";
	row.old_text = "Goodbye";
	row.record_index = 0;

	const auto result = doc.commit(row, "Do widzenia", status_t::model);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::model);
	REQUIRE(result.new_text == "Do widzenia");
	REQUIRE(result.propagated_count == 0);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::commit, read-only foreign file fails", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_commit_test3";
	fs::create_directories(temp_dir);

	const auto en_path = string_utils::normalize_path((temp_dir / "en.yaml").string());
	std::ofstream(en_path) << "key: value\n";

	yaml_document_t doc(en_path, "pl");

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "key";
	row.old_text = "value";
	row.record_index = 0;

	const auto result = doc.commit(row, "wartosc", status_t::in_progress);

	REQUIRE_FALSE(result.success);
	REQUIRE_FALSE(doc.is_dirty());

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("yaml_document_t::commit, invalid record_index fails", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_commit_test4";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "key: value\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	yaml_document_t doc(pl_path, "pl");

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "bad";
	row.old_text = "x";
	row.record_index = 999;

	const auto result = doc.commit(row, "anything", status_t::in_progress);

	REQUIRE_FALSE(result.success);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("dict_document_t::kind, returns dict", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "key";
	entry.old_text = "old";
	entry.new_text = "old";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	REQUIRE(doc.kind() == document_kind_t::dict);

	cleanup_temp_dict(path);
}

TEST_CASE("yaml_document_t::kind, returns yaml", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_kind_test";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "key: value\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "";

	yaml_document_t doc(pl_path, "pl");
	REQUIRE(doc.kind() == document_kind_t::yaml);

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("dict_document_t::commit_status, changes status only", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::info];
	record_entry_t entry;
	entry.key_text = "key_a";
	entry.old_text = "Hello";
	entry.new_text = "Czesc";
	entry.status = status_t::in_progress;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::info;
	row.key_text = "key_a";
	row.old_text = "Hello";
	row.new_text = "Czesc";
	row.record_index = 0;

	const auto result = doc.commit_status(row, status_t::translated);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::translated);
	REQUIRE(result.new_text == "Czesc");
	REQUIRE(doc.data().at(rec_type_t::info).records[0].status == status_t::translated);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::reset_to_original, restores old_text and sets untranslated", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::fnam];
	record_entry_t entry;
	entry.key_text = "key_a";
	entry.old_text = "Sword";
	entry.new_text = "Miecz";
	entry.status = status_t::translated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::fnam;
	row.key_text = "key_a";
	row.old_text = "Sword";
	row.new_text = "Miecz";
	row.record_index = 0;

	const auto result = doc.reset_to_original(row);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::untranslated);
	REQUIRE(result.new_text == "Sword");
	REQUIRE(doc.data().at(rec_type_t::fnam).records[0].new_text == "Sword");
	REQUIRE(doc.data().at(rec_type_t::fnam).records[0].status == status_t::untranslated);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::dict_kind, returns correct kind", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];
	record_entry_t entry;
	entry.key_text = "key";
	entry.old_text = "old";
	entry.new_text = "old";
	entry.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry));

	const auto path = create_temp_dict_json(data);

	dict_document_t user_doc(path, codepage_t::windows_1252, dict_kind_t::user);
	REQUIRE(user_doc.dict_kind() == dict_kind_t::user);

	dict_document_t base_doc(path, codepage_t::windows_1252, dict_kind_t::base);
	REQUIRE(base_doc.dict_kind() == dict_kind_t::base);

	cleanup_temp_dict(path);
}

TEST_CASE("yaml_document_t::reset_to_original, clears native value", "[i]")
{
	namespace fs = std::filesystem;
	const auto temp_dir = fs::temp_directory_path() / "yampt_yaml_reset_test";
	fs::create_directories(temp_dir);

	const auto en_path = (temp_dir / "en.yaml").string();
	std::ofstream(en_path) << "greeting: Hello\n";

	const auto pl_path = string_utils::normalize_path((temp_dir / "pl.yaml").string());
	std::ofstream(pl_path) << "greeting: Czesc\n";

	yaml_document_t doc(pl_path, "pl");

	table_row_t row;
	row.type = rec_type_t::yaml;
	row.key_text = "greeting";
	row.old_text = "Hello";
	row.new_text = "Czesc";
	row.record_index = 0;

	const auto result = doc.reset_to_original(row);

	REQUIRE(result.success);
	REQUIRE(result.status == status_t::untranslated);
	REQUIRE(result.new_text == "Hello");

	std::error_code ec;
	fs::remove_all(temp_dir, ec);
}

TEST_CASE("dict_document_t::commit, propagation trims whitespace", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::sctx];

	record_entry_t entry_a;
	entry_a.key_text = "ScriptA^messagebox \"Hello\"";
	entry_a.old_text = "messagebox \"Hello\"";
	entry_a.new_text = "messagebox \"Hello\"";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "ScriptB^\tmessagebox \"Hello\"";
	entry_b.old_text = "\tmessagebox \"Hello\"";
	entry_b.new_text = "\tmessagebox \"Hello\"";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::sctx;
	row.key_text = "ScriptA^messagebox \"Hello\"";
	row.old_text = "messagebox \"Hello\"";
	row.record_index = 0;

	const auto result = doc.commit(row, "messagebox \"Czesc\"", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 1);
	REQUIRE(doc.data().at(rec_type_t::sctx).records[1].new_text == "messagebox \"Czesc\"");
	REQUIRE(doc.data().at(rec_type_t::sctx).records[1].status == status_t::propagated);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, source gets propagated status when others match", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Shared";
	entry_a.new_text = "Shared";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Shared";
	entry_b.new_text = "Shared";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Shared";
	row.record_index = 0;

	const auto result = doc.commit(row, "Translated", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 1);
	REQUIRE(doc.data().at(rec_type_t::cell).records[0].status == status_t::propagated);
	REQUIRE(doc.data().at(rec_type_t::cell).records[1].status == status_t::propagated);

	cleanup_temp_dict(path);
}

TEST_CASE("dict_document_t::commit, no propagation keeps intent status", "[i]")
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "Unique";
	entry_a.new_text = "Unique";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "Different";
	entry_b.new_text = "Different";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_temp_dict_json(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	table_row_t row;
	row.type = rec_type_t::cell;
	row.key_text = "key_a";
	row.old_text = "Unique";
	row.record_index = 0;

	const auto result = doc.commit(row, "Translated", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 0);
	REQUIRE(doc.data().at(rec_type_t::cell).records[0].status == status_t::in_progress);

	cleanup_temp_dict(path);
}
