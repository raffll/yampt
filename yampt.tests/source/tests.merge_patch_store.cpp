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

TEST_CASE("merge_patch_store_t::add_lock, inserts and detects lock", "[u]")
{
	merge_patch_store_t store;
	merge_lock_t lock;
	lock.rec_type = "ARMO";
	lock.record_id = "iron_helm";
	lock.scope = lock_scope_t::field;
	lock.sub_type = "AODT";
	lock.occurrence = 0;
	lock.field_index = 5;
	lock.frozen_content = "frozen";

	store.add_lock(lock);

	REQUIRE(store.has_lock(lock));
	REQUIRE(store.locks().size() == 1);
}

TEST_CASE("merge_patch_store_t::add_lock, same target replaces not duplicates", "[u]")
{
	merge_patch_store_t store;
	merge_lock_t lock;
	lock.rec_type = "ARMO";
	lock.record_id = "iron_helm";
	lock.scope = lock_scope_t::sub_record;
	lock.sub_type = "AODT";
	lock.frozen_content = "first";

	store.add_lock(lock);
	lock.frozen_content = "second";
	store.add_lock(lock);

	REQUIRE(store.locks().size() == 1);
	REQUIRE(store.locks()[0].frozen_content == "second");
}

TEST_CASE("merge_patch_store_t::remove_lock, erases matching target", "[u]")
{
	merge_patch_store_t store;
	merge_lock_t lock;
	lock.rec_type = "ARMO";
	lock.record_id = "iron_helm";
	lock.scope = lock_scope_t::sub_record;
	lock.sub_type = "AODT";

	store.add_lock(lock);
	store.remove_lock(lock);

	REQUIRE_FALSE(store.has_lock(lock));
	REQUIRE(store.locks().empty());
}

TEST_CASE("merge_patch_store_t::clear, preserves locks", "[u]")
{
	merge_patch_store_t store;
	store.add("ARMO", "iron_helm", "content");

	merge_lock_t lock;
	lock.rec_type = "ARMO";
	lock.record_id = "iron_helm";
	lock.scope = lock_scope_t::whole_record;
	store.add_lock(lock);

	store.clear();

	REQUIRE(store.count() == 0);
	REQUIRE(store.locks().size() == 1);
}

TEST_CASE("merge_patch_store_t::locks_for, filters by record", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t lock_a;
	lock_a.rec_type = "ARMO";
	lock_a.record_id = "iron_helm";
	lock_a.scope = lock_scope_t::sub_record;
	lock_a.sub_type = "AODT";
	store.add_lock(lock_a);

	merge_lock_t lock_b;
	lock_b.rec_type = "ARMO";
	lock_b.record_id = "steel_helm";
	lock_b.scope = lock_scope_t::sub_record;
	lock_b.sub_type = "AODT";
	store.add_lock(lock_b);

	REQUIRE(store.locks_for("ARMO", "iron_helm").size() == 1);
	REQUIRE(store.locks_for("ARMO", "iron_helm")[0].record_id == "iron_helm");
}

TEST_CASE("merge_patch_store_t::field and bit locks are distinct targets", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t field_lock;
	field_lock.rec_type = "NPC_";
	field_lock.record_id = "guard";
	field_lock.scope = lock_scope_t::field;
	field_lock.sub_type = "FLAG";
	field_lock.field_index = 0;

	merge_lock_t bit_lock = field_lock;
	bit_lock.scope = lock_scope_t::bit;
	bit_lock.bit_index = 0;

	store.add_lock(field_lock);
	store.add_lock(bit_lock);

	REQUIRE(store.locks().size() == 2);
	REQUIRE(store.has_lock(field_lock));
	REQUIRE(store.has_lock(bit_lock));
}
