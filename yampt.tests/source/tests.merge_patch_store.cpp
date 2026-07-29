#include <catch2/catch_all.hpp>
#include <scanner/merge_patch_store.hpp>

TEST_CASE("merge_patch_store_t::add, inserts record", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "content_bytes");
	REQUIRE(store.count() == 1);
	REQUIRE(store.record_type(0) == "NPC_");
	REQUIRE(store.record_id(0) == "fargoth");
	REQUIRE(store.record_content(0) == "content_bytes");
}

TEST_CASE("merge_patch_store_t::clear, removes all records", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "a");
	store.add("CELL", "Balmora", "b");
	store.clear();
	REQUIRE(store.count() == 0);
	REQUIRE(store.empty());
}

TEST_CASE("merge_patch_store_t::remove, erases by type and id", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "a");
	store.add("NPC_", "caius", "b");
	store.add("CELL", "Balmora", "c");
	store.remove("NPC_", "fargoth");
	REQUIRE(store.count() == 2);
	REQUIRE(store.find_content("NPC_", "fargoth") == nullptr);
	REQUIRE(store.find_content("NPC_", "caius") != nullptr);
}

TEST_CASE("merge_patch_store_t::update_or_add, updates existing", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "old_content");
	store.update_or_add("NPC_", "fargoth", "new_content");
	REQUIRE(store.count() == 1);
	REQUIRE(store.record_content(0) == "new_content");
}

TEST_CASE("merge_patch_store_t::update_or_add, adds if missing", "[u]")
{
	merge_patch_store_t store;
	store.update_or_add("NPC_", "fargoth", "content");
	REQUIRE(store.count() == 1);
	REQUIRE(store.record_content(0) == "content");
}

TEST_CASE("merge_patch_store_t::find_content, returns pointer to content", "[u]")
{
	merge_patch_store_t store;
	store.add("CELL", "Balmora", "cell_data");
	const auto * found = store.find_content("CELL", "Balmora");
	REQUIRE(found != nullptr);
	REQUIRE(*found == "cell_data");
}

TEST_CASE("merge_patch_store_t::find_content, returns nullptr for missing", "[u]")
{
	merge_patch_store_t store;
	store.add("CELL", "Balmora", "data");
	REQUIRE(store.find_content("CELL", "Vivec") == nullptr);
	REQUIRE(store.find_content("NPC_", "Balmora") == nullptr);
}

TEST_CASE("merge_patch_store_t::is_pinned, false by default", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "content");
	REQUIRE(store.is_pinned("NPC_", "fargoth") == false);
}

TEST_CASE("merge_patch_store_t::add_pinned, marks as pinned", "[u]")
{
	merge_patch_store_t store;
	store.add_pinned("NPC_", "fargoth", "content");
	REQUIRE(store.is_pinned("NPC_", "fargoth") == true);
}

TEST_CASE("merge_patch_store_t::update_or_add_pinned, pins existing", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "old");
	REQUIRE(store.is_pinned("NPC_", "fargoth") == false);
	store.update_or_add_pinned("NPC_", "fargoth", "new");
	REQUIRE(store.is_pinned("NPC_", "fargoth") == true);
	REQUIRE(store.record_content(0) == "new");
}

TEST_CASE("merge_patch_store_t::collect_pinned, returns only pinned", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "a");
	store.add_pinned("CELL", "Balmora", "b");
	store.add("NPC_", "caius", "c");
	const auto pinned = store.collect_pinned();
	REQUIRE(pinned.size() == 1);
	REQUIRE(pinned[0].rec_type == "CELL");
	REQUIRE(pinned[0].record_id == "Balmora");
}

TEST_CASE("merge_patch_store_t::restore_pinned, updates existing and adds new", "[u]")
{
	merge_patch_store_t store;
	store.add("NPC_", "fargoth", "old");
	std::vector<merge_record_t> pinned = { { "NPC_", "fargoth", "restored", true }, { "CELL", "Vivec", "new", true } };
	store.restore_pinned(pinned);
	REQUIRE(store.count() == 2);
	REQUIRE(store.record_content(0) == "restored");
	REQUIRE(store.is_pinned("NPC_", "fargoth") == true);
	REQUIRE(store.find_content("CELL", "Vivec") != nullptr);
}
