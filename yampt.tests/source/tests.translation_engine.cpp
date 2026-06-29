#include <catch2/catch_all.hpp>
#include <model/translation_engine.hpp>
#include <iostream>

static const char * model_path = "../../models/en-de";

TEST_CASE("translation_engine_t::load, en-de model", "[i]")
{
	translation_engine_t engine;
	REQUIRE_FALSE(engine.is_loaded());

	bool loaded = engine.load(model_path);
	REQUIRE(loaded);
	REQUIRE(engine.is_loaded());
	REQUIRE(engine.source_language() == "en");
	REQUIRE(engine.target_language() == "de");
}

TEST_CASE("translation_engine_t::translate, test strings", "[i]")
{
	translation_engine_t engine;
	REQUIRE(engine.load(model_path));

	SECTION("Guild of Mages")
	{
		auto result = engine.translate("Guild of Mages");
		CAPTURE(result.text);
		CAPTURE(result.error);
		REQUIRE(result.success);
		REQUIRE_FALSE(result.text.empty());
		std::cout << "[en-de] \"Guild of Mages\" -> \"" << result.text << "\"\n";
	}

	SECTION("Ald-ruhn, Guild of Mages")
	{
		auto result = engine.translate("Ald-ruhn, Guild of Mages");
		CAPTURE(result.text);
		CAPTURE(result.error);
		REQUIRE(result.success);
		REQUIRE_FALSE(result.text.empty());
		std::cout << "[en-de] \"Ald-ruhn, Guild of Mages\" -> \"" << result.text << "\"\n";
	}

	SECTION("Hall of Centrifuge")
	{
		auto result = engine.translate("Hall of Centrifuge");
		CAPTURE(result.text);
		CAPTURE(result.error);
		REQUIRE(result.success);
		REQUIRE_FALSE(result.text.empty());
		std::cout << "[en-de] \"Hall of Centrifuge\" -> \"" << result.text << "\"\n";
	}

	SECTION("empty string")
	{
		auto result = engine.translate("");
		REQUIRE(result.success);
		REQUIRE(result.text.empty());
	}
}

TEST_CASE("translation_engine_t::load, invalid path does not crash", "[i]")
{
	translation_engine_t engine;
	bool loaded = engine.load("nonexistent/path/to/model");
	REQUIRE_FALSE(loaded);
	REQUIRE_FALSE(engine.is_loaded());
}

TEST_CASE("translation_engine_t::translate, without loaded model returns error", "[i]")
{
	translation_engine_t engine;
	auto result = engine.translate("Hello");
	REQUIRE_FALSE(result.success);
	REQUIRE_FALSE(result.error.empty());
}
