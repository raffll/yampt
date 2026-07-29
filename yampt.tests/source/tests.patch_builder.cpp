#include <catch2/catch_all.hpp>
#include <patcher/patch_builder.hpp>
#include <rapidcheck/catch.h>
#include <filesystem>
#include <rapidcheck.h>
#include <string>
#include <tuple>
#include <vector>

namespace {

rc::Gen<std::string> gen_type_string()
{
	return rc::gen::exec([]()
	{
		std::string result;
		result.reserve(4);
		for (int index = 0; index < 4; ++index)
			result += *rc::gen::inRange('A', 'Z');
		return result;
	});
}

rc::Gen<std::string> gen_record_id()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(3, 16);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange('a', 'z');
		return result;
	});
}

rc::Gen<std::string> gen_content()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(1, 64);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange(static_cast<char>(0x20), static_cast<char>(0x7E));
		return result;
	});
}

struct operation_t
{
	enum class kind_t
	{
		add,
		remove,
		pin
	};

	kind_t kind;
	std::string rec_type;
	std::string record_id;
	std::string content;
};

rc::Gen<operation_t> gen_operation()
{
	return rc::gen::exec([]()
	{
		const auto kind_val = *rc::gen::inRange(0, 3);
		auto kind = static_cast<operation_t::kind_t>(kind_val);
		auto rec_type = *gen_type_string();
		auto record_id = *gen_record_id();
		auto content = *gen_content();
		return operation_t { kind, rec_type, record_id, content };
	});
}

} // namespace

TEST_CASE("patch_builder_t::add_record, collection invariants", "[pbt]")
{
	rc::prop(
	    "Validates: Requirements 5.1 - add increases count and find_content returns content",
	    []()
	{
		patch_builder_t builder;

		const auto rec_type = *gen_type_string();
		const auto record_id = *gen_record_id();
		const auto content = *gen_content();

		const auto count_before = builder.record_count();
		builder.add_record(rec_type, record_id, content);

		RC_ASSERT(builder.record_count() == count_before + 1);

		const auto * found = builder.find_content(rec_type, record_id);
		RC_ASSERT(found != nullptr);
		RC_ASSERT(*found == content);
	});

	rc::prop(
	    "Validates: Requirements 5.1 - remove_record makes find_content return nullptr",
	    []()
	{
		patch_builder_t builder;

		const auto rec_type = *gen_type_string();
		const auto record_id = *gen_record_id();
		const auto content = *gen_content();

		builder.add_record(rec_type, record_id, content);
		RC_ASSERT(builder.find_content(rec_type, record_id) != nullptr);

		builder.remove_record(rec_type, record_id);
		RC_ASSERT(builder.find_content(rec_type, record_id) == nullptr);
	});

	rc::prop(
	    "Validates: Requirements 5.1 - pin_record makes is_pinned return true",
	    []()
	{
		patch_builder_t builder;

		const auto rec_type = *gen_type_string();
		const auto record_id = *gen_record_id();
		const auto content = *gen_content();

		builder.pin_record(rec_type, record_id, content);
		RC_ASSERT(builder.is_pinned(rec_type, record_id));

		const auto * found = builder.find_content(rec_type, record_id);
		RC_ASSERT(found != nullptr);
		RC_ASSERT(*found == content);
	});

	rc::prop(
	    "Validates: Requirements 5.1 - pinned records survive collect + clear + restore",
	    []()
	{
		patch_builder_t builder;

		const auto pin_count = *rc::gen::inRange(1, 6);
		std::vector<std::tuple<std::string, std::string, std::string>> pinned_entries;

		for (int index = 0; index < pin_count; ++index)
		{
			auto rec_type = *gen_type_string();
			auto record_id = *gen_record_id() + std::to_string(index);
			auto content = *gen_content();
			builder.pin_record(rec_type, record_id, content);
			pinned_entries.emplace_back(rec_type, record_id, content);
		}

		const auto extra_count = *rc::gen::inRange(0, 5);
		for (int index = 0; index < extra_count; ++index)
		{
			auto rec_type = *gen_type_string();
			auto record_id = *gen_record_id() + "_extra" + std::to_string(index);
			auto content = *gen_content();
			builder.add_record(rec_type, record_id, content);
		}

		const auto pinned = builder.collect_pinned_records();
		builder.clear();

		RC_ASSERT(builder.record_count() == 0);

		builder.restore_pinned_records(pinned);

		for (const auto & [rec_type, record_id, content] : pinned_entries)
		{
			const auto * found = builder.find_content(rec_type, record_id);
			RC_ASSERT(found != nullptr);
			RC_ASSERT(*found == content);
			RC_ASSERT(builder.is_pinned(rec_type, record_id));
		}
	});

	rc::prop(
	    "Validates: Requirements 5.1 - random operations maintain consistent record_count",
	    []()
	{
		patch_builder_t builder;

		const auto operation_count = *rc::gen::inRange(5, 20);
		std::vector<operation_t> operations;
		operations.reserve(operation_count);

		for (int index = 0; index < operation_count; ++index)
			operations.push_back(*gen_operation());

		for (const auto & operation : operations)
		{
			const auto count_before = builder.record_count();
			const auto * existing = builder.find_content(operation.rec_type, operation.record_id);

			switch (operation.kind)
			{
			case operation_t::kind_t::add:
				builder.add_record(operation.rec_type, operation.record_id, operation.content);
				if (existing == nullptr)
					RC_ASSERT(builder.record_count() == count_before + 1);
				else
					RC_ASSERT(builder.record_count() == count_before);
				break;

			case operation_t::kind_t::remove:
				builder.remove_record(operation.rec_type, operation.record_id);
				if (existing != nullptr)
					RC_ASSERT(builder.record_count() == count_before - 1);
				else
					RC_ASSERT(builder.record_count() == count_before);
				break;

			case operation_t::kind_t::pin:
				builder.pin_record(operation.rec_type, operation.record_id, operation.content);
				if (existing == nullptr)
					RC_ASSERT(builder.record_count() == count_before + 1);
				else
					RC_ASSERT(builder.record_count() == count_before);
				break;
			}
		}
	});
}

TEST_CASE("patch_builder_t::save, creates ESP file on disk", "[i]")
{
	namespace fs = std::filesystem;
	const auto output_path = (fs::temp_directory_path() / "yampt_pb_save_test.esp").string();

	patch_builder_t builder;
	builder.add_record_raw("NPC_", "test_npc", std::string(32, 'A'));
	builder.add_record_raw("CELL", "test_cell", std::string(64, 'B'));

	std::vector<patch_builder_t::master_entry_t> masters;
	masters.push_back({ "Morrowind.esm", 79837557 });

	const bool saved = builder.save(output_path, "TestAuthor", "TestDesc", masters);

	REQUIRE(saved);
	REQUIRE(fs::exists(output_path));

	const auto file_size = fs::file_size(output_path);
	REQUIRE(file_size > 0);

	std::error_code error_code;
	fs::remove(output_path, error_code);
}

TEST_CASE("patch_builder_t::save, empty builder returns false", "[u]")
{
	patch_builder_t builder;
	const bool saved = builder.save("should_not_exist.esp", "Author", "Desc", {});
	REQUIRE_FALSE(saved);
}

TEST_CASE("patch_builder_t::add_record, duplicate key updates content", "[u]")
{
	patch_builder_t builder;
	builder.add_record("NPC_", "npc_id", "content_v1");
	builder.add_record("NPC_", "npc_id", "content_v2");

	REQUIRE(builder.record_count() == 1);

	const auto * found = builder.find_content("NPC_", "npc_id");
	REQUIRE(found != nullptr);
	REQUIRE(*found == "content_v2");
}

TEST_CASE("patch_builder_t::add_record, pinned record not overwritten by add", "[u]")
{
	patch_builder_t builder;
	builder.pin_record("NPC_", "npc_id", "pinned_content");
	builder.add_record("NPC_", "npc_id", "new_content");

	const auto * found = builder.find_content("NPC_", "npc_id");
	REQUIRE(found != nullptr);
	REQUIRE(*found == "pinned_content");
}

TEST_CASE("patch_builder_t::add_record_raw, overwrites even pinned records", "[u]")
{
	patch_builder_t builder;
	builder.pin_record("NPC_", "npc_id", "pinned_content");
	builder.add_record_raw("NPC_", "npc_id", "raw_override");

	const auto * found = builder.find_content("NPC_", "npc_id");
	REQUIRE(found != nullptr);
	REQUIRE(*found == "raw_override");
}

TEST_CASE("patch_builder_t::clear, empties all records", "[u]")
{
	patch_builder_t builder;
	builder.add_record("NPC_", "a", "content_a");
	builder.pin_record("CELL", "b", "content_b");

	REQUIRE(builder.record_count() == 2);

	builder.clear();

	REQUIRE(builder.record_count() == 0);
	REQUIRE_FALSE(builder.has_records());
}

TEST_CASE("patch_builder_t::record_accessors, correct indexing", "[u]")
{
	patch_builder_t builder;
	builder.add_record_raw("NPC_", "npc_one", "content_one");
	builder.add_record_raw("CELL", "cell_two", "content_two");

	REQUIRE(builder.record_type(0) == "NPC_");
	REQUIRE(builder.record_id(0) == "npc_one");
	REQUIRE(builder.record_content(0) == "content_one");

	REQUIRE(builder.record_type(1) == "CELL");
	REQUIRE(builder.record_id(1) == "cell_two");
	REQUIRE(builder.record_content(1) == "content_two");
}
