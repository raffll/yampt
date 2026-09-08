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

TEST_CASE("merge_lock_scope::is_valid_value, accepts known scopes rejects others", "[u]")
{
	REQUIRE(merge_lock_scope::is_valid_value(static_cast<int>(lock_scope_t::whole_record)));
	REQUIRE(merge_lock_scope::is_valid_value(static_cast<int>(lock_scope_t::group)));
	REQUIRE_FALSE(merge_lock_scope::is_valid_value(-1));
	REQUIRE_FALSE(merge_lock_scope::is_valid_value(static_cast<int>(lock_scope_t::group) + 1));
}

TEST_CASE("merge_lock_scope::scope_from_value, out param set only for valid", "[u]")
{
	lock_scope_t scope = lock_scope_t::whole_record;

	REQUIRE(merge_lock_scope::scope_from_value(static_cast<int>(lock_scope_t::bit), scope));
	REQUIRE(scope == lock_scope_t::bit);

	scope = lock_scope_t::field;
	REQUIRE_FALSE(merge_lock_scope::scope_from_value(99, scope));
	REQUIRE(scope == lock_scope_t::field);
}

TEST_CASE("merge_lock_t::same_target, differing occurrence is a distinct target", "[u]")
{
	merge_lock_t first;
	first.rec_type = "CELL";
	first.record_id = "Balmora";
	first.scope = lock_scope_t::sub_record;
	first.sub_type = "NAME";
	first.occurrence = 0;

	merge_lock_t second = first;
	second.occurrence = 1;

	REQUIRE_FALSE(first.same_target(second));
}

TEST_CASE("merge_lock_t::same_target, differing field_index is a distinct target", "[u]")
{
	merge_lock_t first;
	first.rec_type = "NPC_";
	first.record_id = "guard";
	first.scope = lock_scope_t::field;
	first.sub_type = "FLAG";
	first.field_index = 0;

	merge_lock_t second = first;
	second.field_index = 1;

	REQUIRE_FALSE(first.same_target(second));
}

TEST_CASE("merge_lock_t::same_target, differing group range is a distinct target", "[u]")
{
	merge_lock_t first;
	first.rec_type = "CELL";
	first.record_id = "Balmora";
	first.scope = lock_scope_t::group;
	first.group_start = 0;
	first.group_end = 4;

	merge_lock_t second = first;
	second.group_end = 8;

	REQUIRE_FALSE(first.same_target(second));
}

TEST_CASE("merge_lock_t::same_target, frozen_content does not affect identity", "[u]")
{
	merge_lock_t first;
	first.rec_type = "ARMO";
	first.record_id = "iron_helm";
	first.scope = lock_scope_t::whole_record;
	first.frozen_content = "state_a";

	merge_lock_t second = first;
	second.frozen_content = "state_b";

	REQUIRE(first.same_target(second));
}

TEST_CASE("merge_patch_store_t::add_lock, distinct scopes on same record coexist", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t whole;
	whole.rec_type = "ARMO";
	whole.record_id = "iron_helm";
	whole.scope = lock_scope_t::whole_record;

	merge_lock_t sub = whole;
	sub.scope = lock_scope_t::sub_record;
	sub.sub_type = "AODT";

	store.add_lock(whole);
	store.add_lock(sub);

	REQUIRE(store.locks().size() == 2);
	REQUIRE(store.locks_for("ARMO", "iron_helm").size() == 2);
}

TEST_CASE("merge_patch_store_t::set_locks, replaces the whole set", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t existing;
	existing.rec_type = "ARMO";
	existing.record_id = "iron_helm";
	existing.scope = lock_scope_t::whole_record;
	store.add_lock(existing);

	merge_lock_t replacement;
	replacement.rec_type = "CELL";
	replacement.record_id = "Balmora";
	replacement.scope = lock_scope_t::sub_record;
	replacement.sub_type = "NAME";

	store.set_locks({ replacement });

	REQUIRE(store.locks().size() == 1);
	REQUIRE(store.has_lock(replacement));
	REQUIRE_FALSE(store.has_lock(existing));
}

TEST_CASE("merge_patch_store_t::set_locks, empty vector clears all locks", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t lock;
	lock.rec_type = "ARMO";
	lock.record_id = "iron_helm";
	lock.scope = lock_scope_t::whole_record;
	store.add_lock(lock);

	store.set_locks({});

	REQUIRE(store.locks().empty());
}

TEST_CASE("merge_patch_store_t::locks_for, returns every matching lock", "[u]")
{
	merge_patch_store_t store;

	merge_lock_t sub;
	sub.rec_type = "CELL";
	sub.record_id = "Balmora";
	sub.scope = lock_scope_t::sub_record;
	sub.sub_type = "NAME";
	store.add_lock(sub);

	merge_lock_t field;
	field.rec_type = "CELL";
	field.record_id = "Balmora";
	field.scope = lock_scope_t::field;
	field.sub_type = "DATA";
	field.field_index = 1;
	store.add_lock(field);

	merge_lock_t other;
	other.rec_type = "CELL";
	other.record_id = "Vivec";
	other.scope = lock_scope_t::whole_record;
	store.add_lock(other);

	REQUIRE(store.locks_for("CELL", "Balmora").size() == 2);
	REQUIRE(store.locks_for("CELL", "Vivec").size() == 1);
	REQUIRE(store.locks_for("CELL", "Suran").empty());
}

TEST_CASE("merge_patch_store_t::update_or_add, replaces content for existing record", "[u]")
{
	merge_patch_store_t store;
	store.add("ARMO", "iron_helm", "v1");
	store.update_or_add("ARMO", "iron_helm", "v2");

	REQUIRE(store.count() == 1);
	REQUIRE(*store.find_content("ARMO", "iron_helm") == "v2");
}

TEST_CASE("merge_patch_store_t::add, allows duplicate records unlike update_or_add", "[u]")
{
	merge_patch_store_t store;
	store.add("ARMO", "iron_helm", "v1");
	store.add("ARMO", "iron_helm", "v2");

	REQUIRE(store.count() == 2);
}

TEST_CASE("merge_patch_store_t::remove, erases all copies of a record", "[u]")
{
	merge_patch_store_t store;
	store.add("ARMO", "iron_helm", "v1");
	store.add("ARMO", "iron_helm", "v2");

	store.remove("ARMO", "iron_helm");

	REQUIRE(store.count() == 0);
	REQUIRE(store.find_content("ARMO", "iron_helm") == nullptr);
}
