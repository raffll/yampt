#include "catch.hpp"
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include "../yampt.gui/dict_document.hpp"
#include "../yampt.gui/dict_workspace.hpp"

#include <algorithm>

// **Validates: Requirements 2.2**
TEST_CASE("dict_document_t property: path round trip", "[u]")
{
	rc::prop("path() returns normalized slot_path", []()
	{
		dict_slot_t slot;
		const auto raw_path = *rc::gen::nonEmpty(rc::gen::arbitrary<std::string>());

		dict_document_t doc(&slot, raw_path, [](const std::string &) {}, false);

		auto expected = raw_path;
		std::replace(expected.begin(), expected.end(), '\\', '/');
		RC_ASSERT(doc.path() == expected);
	});
}

// **Validates: Requirements 2.5, 2.7**
TEST_CASE("dict_document_t property: build_rows count invariant", "[u]")
{
	rc::prop("build_rows().size() == total_count()", []()
	{
		dict_slot_t slot;

		const auto type_count = *rc::gen::inRange(0, 4);
		const std::vector<tools_t::rec_type_t> types = {
			tools_t::rec_type_t::cell,
			tools_t::rec_type_t::dial,
			tools_t::rec_type_t::info,
			tools_t::rec_type_t::fnam
		};

		for (int t = 0; t < type_count; ++t)
		{
			const auto rec_count = *rc::gen::inRange(0, 20);
			auto & chapter = slot.data[types[t]];
			for (int r = 0; r < rec_count; ++r)
			{
				tools_t::record_entry_t entry;
				entry.key_text = *rc::gen::arbitrary<std::string>();
				entry.old_text = *rc::gen::arbitrary<std::string>();
				entry.new_text = *rc::gen::arbitrary<std::string>();
				entry.status = "untranslated";
				chapter.records.push_back(std::move(entry));
			}
		}

		dict_document_t doc(&slot, "test/path.json", [](const std::string &) {}, false);

		const auto rows = doc.build_rows();
		RC_ASSERT(static_cast<int>(rows.size()) == doc.total_count());
	});
}

// **Validates: Requirements 2.10**
TEST_CASE("dict_document_t property: read-only commit is no-op", "[u]")
{
	rc::prop("commit_edit on read-only doc does not modify data", []()
	{
		dict_slot_t slot;
		auto & chapter = slot.data[tools_t::rec_type_t::cell];

		const auto rec_count = *rc::gen::inRange(1, 10);
		for (int i = 0; i < rec_count; ++i)
		{
			tools_t::record_entry_t entry;
			entry.key_text = "key_" + std::to_string(i);
			entry.old_text = "old_" + std::to_string(i);
			entry.new_text = "new_" + std::to_string(i);
			entry.status = "untranslated";
			chapter.records.push_back(std::move(entry));
		}

		dict_document_t doc(&slot, "test/path.json", [](const std::string &) {}, true);

		const auto rows_before = doc.build_rows();

		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(rec_count));
		const auto new_text = *rc::gen::arbitrary<std::string>();
		doc.commit_edit(tools_t::rec_type_t::cell, idx, new_text);

		const auto rows_after = doc.build_rows();
		RC_ASSERT(rows_before.size() == rows_after.size());
		for (size_t i = 0; i < rows_before.size(); ++i)
		{
			RC_ASSERT(rows_before[i].new_text == rows_after[i].new_text);
		}
	});
}

#include "../yampt.gui/yaml_document.hpp"

#include <filesystem>
#include <fstream>

namespace
{

std::string create_temp_yaml(const std::vector<std::pair<std::string, std::string>> & entries)
{
	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_test_doc.yaml").string();
	std::replace(path.begin(), path.end(), '\\', '/');

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

// **Validates: Requirements 3.2**
TEST_CASE("yaml_document_t property: path round trip", "[u]")
{
	rc::prop("path() returns normalized file path", []()
	{
		const auto entries = std::vector<std::pair<std::string, std::string>>{
			{"key1", "value1"}, {"key2", "value2"}};
		const auto path = create_temp_yaml(entries);

		yaml_document_t doc(path);
		RC_ASSERT(doc.path() == path);

		cleanup_temp_yaml(path);
	});
}

// **Validates: Requirements 3.5**
TEST_CASE("yaml_document_t property: dirty state consistency", "[u]")
{
	rc::prop("set_dirty round-trips; commit_edit sets dirty", []()
	{
		const auto count = *rc::gen::inRange(1, 10);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({"key_" + std::to_string(i), "val_" + std::to_string(i)});

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path);

		const auto b = *rc::gen::arbitrary<bool>();
		doc.set_dirty(b);
		RC_ASSERT(doc.is_dirty() == b);

		doc.set_dirty(false);
		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
		doc.commit_edit(tools_t::rec_type_t::lua, idx, "edited");
		RC_ASSERT(doc.is_dirty() == true);

		cleanup_temp_yaml(path);
	});
}

// **Validates: Requirements 3.8**
TEST_CASE("yaml_document_t property: commit-edit round trip", "[u]")
{
	rc::prop("after commit_edit, build_rows()[idx].new_text == text", []()
	{
		const auto count = *rc::gen::inRange(1, 10);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({"key_" + std::to_string(i), "val_" + std::to_string(i)});

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path);

		const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
		const auto new_text = *rc::gen::nonEmpty(rc::gen::arbitrary<std::string>());
		doc.commit_edit(tools_t::rec_type_t::lua, idx, new_text);

		const auto rows = doc.build_rows();
		RC_ASSERT(rows[idx].new_text == new_text);

		cleanup_temp_yaml(path);
	});
}

// **Validates: Requirements 3.9**
TEST_CASE("yaml_document_t property: translated count invariant", "[u]")
{
	rc::prop("after K distinct edits, translated_count() == K", []()
	{
		const auto count = *rc::gen::inRange(2, 15);
		std::vector<std::pair<std::string, std::string>> entries;
		for (int i = 0; i < count; ++i)
			entries.push_back({"key_" + std::to_string(i), "val_" + std::to_string(i)});

		const auto path = create_temp_yaml(entries);
		yaml_document_t doc(path);

		const auto edit_count = *rc::gen::inRange(1, count);
		std::set<size_t> edited;
		for (int e = 0; e < edit_count; ++e)
		{
			const auto idx = *rc::gen::inRange<size_t>(0, static_cast<size_t>(count));
			doc.commit_edit(tools_t::rec_type_t::lua, idx, "edited_" + std::to_string(e));
			edited.insert(idx);
		}

		RC_ASSERT(doc.translated_count() == static_cast<int>(edited.size()));

		cleanup_temp_yaml(path);
	});
}
