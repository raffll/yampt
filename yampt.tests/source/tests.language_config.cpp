#include <catch2/catch_all.hpp>
#include <utility/language_config.hpp>

namespace {

std::vector<language_entry_t> make_languages()
{
	return {
	    { "EN", "English", "eng_Latn", "en_US", codepage_t::windows_1252 },
	    { "ES", "Spanish", "spa_Latn", "es_ES", codepage_t::windows_1252 },
	    { "PT", "Portuguese", "por_Latn", "pt_PT", codepage_t::windows_1252 },
	    { "CS", "Czech", "ces_Latn", "cs_CZ", codepage_t::windows_1250 },
	    { "UK", "Ukrainian", "ukr_Cyrl", "uk_UA", codepage_t::windows_1251 },
	    { "NL", "Dutch", "nld_Latn", "nl_NL", codepage_t::windows_1252 },
	    { "FI", "Finnish", "fin_Latn", "fi_FI", codepage_t::windows_1252 },
	};
}

} // namespace

TEST_CASE("language_config::find_by_code, ES and PT resolve", "[u]")
{
	const auto languages = make_languages();

	const auto * spanish = language_config::find_by_code(languages, "ES");
	REQUIRE(spanish != nullptr);
	REQUIRE(spanish->nllb_code == "spa_Latn");
	REQUIRE(spanish->dictionary_prefix == "es_ES");
	REQUIRE(spanish->codepage == codepage_t::windows_1252);

	const auto * portuguese = language_config::find_by_code(languages, "PT");
	REQUIRE(portuguese != nullptr);
	REQUIRE(portuguese->nllb_code == "por_Latn");
	REQUIRE(portuguese->dictionary_prefix == "pt_PT");
	REQUIRE(portuguese->codepage == codepage_t::windows_1252);
}

TEST_CASE("language_config::find_by_code, unknown code returns nullptr", "[u]")
{
	const auto languages = make_languages();

	REQUIRE(language_config::find_by_code(languages, "ZZ") == nullptr);
}

TEST_CASE("language_config::resolve_codepage, across three codepages", "[u]")
{
	const auto languages = make_languages();

	REQUIRE(language_config::resolve_codepage(languages, "ES") == codepage_t::windows_1252);
	REQUIRE(language_config::resolve_codepage(languages, "PT") == codepage_t::windows_1252);
	REQUIRE(language_config::resolve_codepage(languages, "NL") == codepage_t::windows_1252);
	REQUIRE(language_config::resolve_codepage(languages, "CS") == codepage_t::windows_1250);
	REQUIRE(language_config::resolve_codepage(languages, "UK") == codepage_t::windows_1251);
}

TEST_CASE("language_config::resolve_codepage, unknown code falls back to 1252", "[u]")
{
	const auto languages = make_languages();

	REQUIRE(language_config::resolve_codepage(languages, "ZZ") == codepage_t::windows_1252);
}

TEST_CASE("language_config::resolve_dictionary_prefix, new codes resolve", "[u]")
{
	const auto languages = make_languages();

	REQUIRE(language_config::resolve_dictionary_prefix(languages, "ES") == "es_ES");
	REQUIRE(language_config::resolve_dictionary_prefix(languages, "PT") == "pt_PT");
	REQUIRE(language_config::resolve_dictionary_prefix(languages, "CS") == "cs_CZ");
	REQUIRE(language_config::resolve_dictionary_prefix(languages, "UK") == "uk_UA");
	REQUIRE(language_config::resolve_dictionary_prefix(languages, "NL") == "nl_NL");
	REQUIRE(language_config::resolve_dictionary_prefix(languages, "FI") == "fi_FI");
}

TEST_CASE("language_config::resolve_dictionary_prefix, unknown code returns empty", "[u]")
{
	const auto languages = make_languages();

	REQUIRE(language_config::resolve_dictionary_prefix(languages, "ZZ").empty());
}
