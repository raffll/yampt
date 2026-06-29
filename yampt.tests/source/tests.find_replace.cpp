#include <catch2/catch_all.hpp>
#include <controller/find_replace.hpp>
#include <io/dict_writer.hpp>
#include <model/dict_document.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>

namespace {

class mock_row_source_t : public row_source_t
{
public:
	std::vector<table_row_t> rows;

	int row_count() const override
	{
		return static_cast<int>(rows.size());
	}

	const table_row_t * row_at(int index) const override
	{
		if (index < 0 || index >= static_cast<int>(rows.size()))
			return nullptr;

		return &rows[index];
	}
};

std::string create_service_dict(const tools_t::dict_t & data)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_frs_test.json").string();
	path = string_utils::normalize_path(path);

	tools_t::reset_log();
	dict_writer_t::write(data, path);
	tools_t::reset_log();
	return path;
}

void cleanup_service_dict(const std::string & path)
{
	tools_t::reset_log();
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

} // anonymous namespace

TEST_CASE("find_replace_t::find_next, finds matching row", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "hello world";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	table_row_t row_second;
	row_second.type = tools_t::rec_type_t::cell;
	row_second.key_text = "key_b";
	row_second.old_text = "beta";
	row_second.new_text = "goodbye";
	row_second.status = status_t::untranslated;
	row_second.record_index = 1;

	source.rows = { row_first, row_second };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "hello world";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	tools_t::record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "beta";
	entry_b.new_text = "goodbye";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	const auto result = service.find_next("goodbye", false, false, 0);

	REQUIRE(result.found);
	REQUIRE(result.row == 1);

	cleanup_service_dict(path);
}

TEST_CASE("find_replace_t::find_next, wraps around to first row", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "target text";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	table_row_t row_second;
	row_second.type = tools_t::rec_type_t::cell;
	row_second.key_text = "key_b";
	row_second.old_text = "beta";
	row_second.new_text = "other";
	row_second.status = status_t::untranslated;
	row_second.record_index = 1;

	source.rows = { row_first, row_second };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "target text";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	tools_t::record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "beta";
	entry_b.new_text = "other";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	const auto result = service.find_next("target", false, false, 1);

	REQUIRE(result.found);
	REQUIRE(result.row == 0);

	cleanup_service_dict(path);
}

TEST_CASE("find_replace_t::find_next, not found returns empty", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "hello";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	source.rows = { row_first };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "hello";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	const auto result = service.find_next("nonexistent", false, false, 0);

	REQUIRE_FALSE(result.found);

	cleanup_service_dict(path);
}

TEST_CASE("find_replace_t::find_next, case insensitive", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "Hello World";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	source.rows = { row_first };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "Hello World";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	const auto result = service.find_next("hello world", false, false, -1);

	REQUIRE(result.found);
	REQUIRE(result.row == 0);

	cleanup_service_dict(path);
}

TEST_CASE("find_replace_t::replace_all, replaces all matching", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "old value here";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	table_row_t row_second;
	row_second.type = tools_t::rec_type_t::cell;
	row_second.key_text = "key_b";
	row_second.old_text = "beta";
	row_second.new_text = "old value there";
	row_second.status = status_t::untranslated;
	row_second.record_index = 1;

	table_row_t row_third;
	row_third.type = tools_t::rec_type_t::cell;
	row_third.key_text = "key_c";
	row_third.old_text = "gamma";
	row_third.new_text = "no match";
	row_third.status = status_t::untranslated;
	row_third.record_index = 2;

	source.rows = { row_first, row_second, row_third };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "old value here";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	tools_t::record_entry_t entry_b;
	entry_b.key_text = "key_b";
	entry_b.old_text = "beta";
	entry_b.new_text = "old value there";
	entry_b.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_b));

	tools_t::record_entry_t entry_c;
	entry_c.key_text = "key_c";
	entry_c.old_text = "gamma";
	entry_c.new_text = "no match";
	entry_c.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_c));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	const auto result = service.replace_all("old value", "new value", false, false);

	REQUIRE(result.count == 2);

	const auto & records = doc.data().at(tools_t::rec_type_t::cell).records;
	REQUIRE(records[0].new_text == "new value here");
	REQUIRE(records[1].new_text == "new value there");
	REQUIRE(records[2].new_text == "no match");

	cleanup_service_dict(path);
}

TEST_CASE("find_replace_t::replace_all, sets status in_progress", "[i][qt]")
{
	mock_row_source_t source;

	table_row_t row_first;
	row_first.type = tools_t::rec_type_t::cell;
	row_first.key_text = "key_a";
	row_first.old_text = "alpha";
	row_first.new_text = "replaceable text";
	row_first.status = status_t::untranslated;
	row_first.record_index = 0;

	source.rows = { row_first };

	tools_t::dict_t data;
	auto & chapter = data[tools_t::rec_type_t::cell];
	tools_t::record_entry_t entry_a;
	entry_a.key_text = "key_a";
	entry_a.old_text = "alpha";
	entry_a.new_text = "replaceable text";
	entry_a.status = status_t::untranslated;
	chapter.records.push_back(std::move(entry_a));

	const auto path = create_service_dict(data);
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);
	document_t * active_doc = &doc;

	find_replace_t service(source, active_doc);
	service.replace_all("replaceable", "replaced", false, false);

	const auto & records = doc.data().at(tools_t::rec_type_t::cell).records;
	REQUIRE(records[0].status == status_t::in_progress);

	cleanup_service_dict(path);
}
