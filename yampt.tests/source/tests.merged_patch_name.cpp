#include <catch2/catch_all.hpp>
#include <session/merged_patch_name.hpp>
#include <string>

TEST_CASE("merged_patch::filename, matches the canonical output name", "[u]")
{
	REQUIRE(merged_patch::filename == "Merged Patch.esp");
}

TEST_CASE("merged_patch::locks_suffix, composes the sidecar name", "[u]")
{
	const auto locks_name = std::string(merged_patch::filename) + std::string(merged_patch::locks_suffix);
	REQUIRE(locks_name == "Merged Patch.esp.locks");
}

TEST_CASE("merged_patch::filename, comparable to std::string both directions", "[u]")
{
	const std::string as_string(merged_patch::filename);
	REQUIRE(as_string == merged_patch::filename);
	REQUIRE(merged_patch::filename == as_string);
}
