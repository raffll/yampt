#include <catch2/catch_all.hpp>
#include <creator/creator_helpers.hpp>
#include <utility/status_types.hpp>
#include <memory>

static std::unique_ptr<creator_context_t> make_context(base_mode_t base_mode)
{
	auto ctx = std::make_unique<creator_context_t>("");
	ctx->base_mode = base_mode;
	ctx->english_dict = nullptr;
	return ctx;
}

TEST_CASE("creator_helpers::determine_status, differing text is translated", "[u]")
{
	const auto ctx_full = make_context(base_mode_t::full);
	REQUIRE(creator_helpers::determine_status(*ctx_full, "Blades", "Ostrza") == status_t::translated);

	const auto ctx_partial = make_context(base_mode_t::partial);
	REQUIRE(creator_helpers::determine_status(*ctx_partial, "Blades", "Ostrza") == status_t::translated);
}

TEST_CASE("creator_helpers::determine_status, identical text in full mode is translated", "[u]")
{
	const auto ctx = make_context(base_mode_t::full);
	REQUIRE(creator_helpers::determine_status(*ctx, "Balmora", "Balmora") == status_t::translated);
}

TEST_CASE("creator_helpers::determine_status, identical text in partial mode without dict is to_verify", "[u]")
{
	const auto ctx = make_context(base_mode_t::partial);
	REQUIRE(creator_helpers::determine_status(*ctx, "Balmora", "Balmora") == status_t::to_verify);
}

TEST_CASE("creator_helpers::is_proper_noun, null dict treats every text as proper noun", "[u]")
{
	const auto ctx = make_context(base_mode_t::partial);
	REQUIRE(creator_helpers::is_proper_noun(*ctx, "Balmora"));
	REQUIRE(creator_helpers::is_proper_noun(*ctx, "the sword"));
}

TEST_CASE("creator_helpers::insert_duplicate, duplicate entry always gets duplicate status", "[u]")
{
	const auto ctx = make_context(base_mode_t::full);

	creator_helpers::insert_entry_single(*ctx, "KEY", "old", "new_one", rec_type_t::fnam);
	creator_helpers::insert_entry_single(*ctx, "KEY", "old", "new_two", rec_type_t::fnam);

	const auto * entry = ctx->dict.at(rec_type_t::fnam).find("KEY");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->status == status_t::duplicate);
	REQUIRE(ctx->counter_doubled == 1);
}
