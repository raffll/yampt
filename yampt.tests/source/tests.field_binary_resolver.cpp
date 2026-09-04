#include <catch2/catch_all.hpp>

#include <model/field_binary_resolver.hpp>

using view_node_t = view_tree_model_t::view_node_t;

static view_node_t make_sub_record_node(const std::string & type, size_t size, int column_start)
{
	view_node_t node;
	node.type = type;
	node.size = size;
	node.binary_ranges.resize(column_start + 1);
	node.binary_ranges[column_start] = { column_start, column_start + 1 };

	return node;
}

TEST_CASE("field_binary_resolver::resolve, grouped field uses group node inherited range", "[u]")
{
	static constexpr int column = 1;
	static constexpr size_t cldt_size = 60;
	static constexpr int major_three_field_idx = 5;

	auto sub_record = make_sub_record_node("CLDT", cldt_size, column);

	view_node_t group_node;
	group_node.type = "CLDT";
	group_node.size = cldt_size;
	group_node.binary_ranges = sub_record.binary_ranges;

	const std::vector<const view_node_t *> ancestors { &group_node, &sub_record };

	const auto resolved = field_binary_resolver::resolve(ancestors, column, major_three_field_idx);

	REQUIRE(resolved.found);
	REQUIRE(resolved.sub_type == "CLDT");
	REQUIRE(resolved.sub_size == cldt_size);
	REQUIRE(resolved.binary_index == column);
}

TEST_CASE("field_binary_resolver::resolve, grouped field with empty group range fails", "[u]")
{
	static constexpr int column = 1;

	view_node_t group_node;
	group_node.type = "CLDT";
	group_node.size = 60;

	const std::vector<const view_node_t *> ancestors { &group_node };

	const auto resolved = field_binary_resolver::resolve(ancestors, column, 5);

	REQUIRE_FALSE(resolved.found);
	REQUIRE(resolved.binary_index == -1);
}

TEST_CASE("field_binary_resolver::resolve, ungrouped field uses sub-record node", "[u]")
{
	static constexpr int column = 0;

	auto sub_record = make_sub_record_node("DATA", 12, column);

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve(ancestors, column, 1);

	REQUIRE(resolved.found);
	REQUIRE(resolved.sub_type == "DATA");
	REQUIRE(resolved.binary_index == column);
}

TEST_CASE("field_binary_resolver::find_sub_record_node, skips empty-type ancestors", "[u]")
{
	view_node_t empty_type_group;
	empty_type_group.type.clear();

	auto sub_record = make_sub_record_node("CLDT", 60, 0);

	const std::vector<const view_node_t *> ancestors { &empty_type_group, &sub_record };

	const auto * found = field_binary_resolver::find_sub_record_node(ancestors);

	REQUIRE(found == &sub_record);
}

TEST_CASE("field_binary_resolver::resolve, reports parent sub-record occurrence", "[u]")
{
	static constexpr int column = 1;
	static constexpr int third_occurrence = 2;

	auto sub_record = make_sub_record_node("NPCO", 36, column);
	sub_record.occurrence = third_occurrence;

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve(ancestors, column, 1);

	REQUIRE(resolved.found);
	REQUIRE(resolved.occurrence == third_occurrence);
}

TEST_CASE("field_binary_resolver::resolve_bit, reports parent sub-record occurrence", "[u]")
{
	static constexpr int column = 0;
	static constexpr int second_occurrence = 1;

	auto sub_record = make_sub_record_node("BYDT", 4, column);
	sub_record.occurrence = second_occurrence;

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve_bit(ancestors, column, 2, 1);

	REQUIRE(resolved.found);
	REQUIRE(resolved.occurrence == second_occurrence);
}

TEST_CASE("field_binary_resolver::resolve, negative field index fails", "[u]")
{
	auto sub_record = make_sub_record_node("CLDT", 60, 0);

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve(ancestors, 0, -1);

	REQUIRE_FALSE(resolved.found);
}

TEST_CASE("field_binary_resolver::binary_index_for_column, out-of-range column returns -1", "[u]")
{
	auto sub_record = make_sub_record_node("CLDT", 60, 0);

	REQUIRE(field_binary_resolver::binary_index_for_column(sub_record, 5) == -1);
	REQUIRE(field_binary_resolver::binary_index_for_column(sub_record, -1) == -1);
}

TEST_CASE("field_binary_resolver::resolve_bit, resolves bit row under sub-record", "[u]")
{
	static constexpr int column = 0;

	auto sub_record = make_sub_record_node("BYDT", 4, column);

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve_bit(ancestors, column, 2, 1);

	REQUIRE(resolved.found);
	REQUIRE(resolved.sub_type == "BYDT");
	REQUIRE(resolved.binary_index == column);
	REQUIRE(resolved.field_index == 2);
	REQUIRE(resolved.bit_index == 1);
}

TEST_CASE("field_binary_resolver::resolve_bit, negative bit index fails", "[u]")
{
	auto sub_record = make_sub_record_node("BYDT", 4, 0);

	const std::vector<const view_node_t *> ancestors { &sub_record };

	const auto resolved = field_binary_resolver::resolve_bit(ancestors, 0, 2, -1);

	REQUIRE_FALSE(resolved.found);
}
