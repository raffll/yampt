#include <catch2/catch_all.hpp>
#include <translator/web_translator_config.hpp>
#include <filesystem>
#include <fstream>
#include <QCoreApplication>

namespace {

std::string temp_providers_dir()
{
	auto path = std::filesystem::temp_directory_path() / "yampt_test_providers";
	std::filesystem::create_directories(path);
	return path.string();
}

void write_file(const std::string & directory, const std::string & filename, const std::string & content)
{
	auto full_path = std::filesystem::path(directory) / filename;
	std::ofstream stream(full_path);
	stream << content;
}

void cleanup_dir(const std::string & directory)
{
	std::filesystem::remove_all(directory);
}

} // namespace

TEST_CASE("web_translator_config::load_single, parses simple provider", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "deepl.json", R"({
		"name": "DeepL Free",
		"kind": "simple",
		"endpoint": "https://api-free.deepl.com/v2/translate",
		"method": "POST",
		"body_format": "form",
		"headers": {
			"Authorization": "DeepL-Auth-Key {{api_key}}"
		},
		"body": {
			"text": "{{text}}",
			"target_lang": "{{target_lang_upper}}"
		},
		"response_path": "translations[0].text",
		"quota_limit": 500000
	})");

	auto config = web_translator_config::load_single(directory + "/deepl.json");

	REQUIRE(config.identifier == "deepl");
	REQUIRE(config.display_name == "DeepL Free");
	REQUIRE(config.endpoint == "https://api-free.deepl.com/v2/translate");
	REQUIRE(config.method == "POST");
	REQUIRE(config.body_format == body_format_t::form);
	REQUIRE(config.kind == provider_kind_t::simple);
	REQUIRE(config.headers.size() == 1);
	REQUIRE(config.headers.at("Authorization") == "DeepL-Auth-Key {{api_key}}");
	REQUIRE(config.body_fields.at("text") == "{{text}}");
	REQUIRE(config.body_fields.at("target_lang") == "{{target_lang_upper}}");
	REQUIRE(config.response_path == "translations[0].text");
	REQUIRE(config.quota_limit == 500000);

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_single, parses chat_completion provider", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "claude.json", R"({
		"name": "Claude",
		"kind": "chat_completion",
		"endpoint": "https://api.anthropic.com/v1/messages",
		"method": "POST",
		"body_format": "json",
		"headers": {
			"Content-Type": "application/json",
			"x-api-key": "{{api_key}}"
		},
		"body": {
			"model": "claude-sonnet-4-20250514",
			"max_tokens": "4096"
		},
		"response_path": "content[0].text",
		"system_prompt": "Translate to {{target_lang}}."
	})");

	auto config = web_translator_config::load_single(directory + "/claude.json");

	REQUIRE(config.identifier == "claude");
	REQUIRE(config.display_name == "Claude");
	REQUIRE(config.kind == provider_kind_t::chat_completion);
	REQUIRE(config.body_format == body_format_t::json);
	REQUIRE(config.headers.size() == 2);
	REQUIRE(config.headers.at("x-api-key") == "{{api_key}}");
	REQUIRE(config.body_fields.at("model") == "claude-sonnet-4-20250514");
	REQUIRE(config.response_path == "content[0].text");
	REQUIRE(config.system_prompt == "Translate to {{target_lang}}.");
	REQUIRE(config.quota_limit == 0);

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_single, empty file returns empty config", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "empty.json", "");

	auto config = web_translator_config::load_single(directory + "/empty.json");

	REQUIRE(config.display_name.empty());
	REQUIRE(config.identifier == "empty");

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_single, nonexistent file returns empty config", "[u]")
{
	auto config = web_translator_config::load_single("/nonexistent/path/provider.json");
	REQUIRE(config.display_name.empty());
}

TEST_CASE("web_translator_config::load_single, defaults for missing fields", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "minimal.json", R"({
		"name": "Minimal",
		"endpoint": "https://example.com/translate"
	})");

	auto config = web_translator_config::load_single(directory + "/minimal.json");

	REQUIRE(config.display_name == "Minimal");
	REQUIRE(config.method == "POST");
	REQUIRE(config.body_format == body_format_t::json);
	REQUIRE(config.kind == provider_kind_t::simple);
	REQUIRE(config.quota_limit == 0);
	REQUIRE(config.headers.empty());
	REQUIRE(config.body_fields.empty());
	REQUIRE(config.system_prompt.empty());

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_all, loads multiple configs from directory", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "alpha.json", R"({"name": "Alpha", "endpoint": "https://a.com"})");
	write_file(directory, "beta.json", R"({"name": "Beta", "endpoint": "https://b.com"})");
	write_file(directory, "not_json.txt", "plain text file");

	auto configs = web_translator_config::load_all(directory);

	REQUIRE(configs.size() == 2);

	bool found_alpha = false;
	bool found_beta = false;
	for (const auto & config : configs)
	{
		if (config.display_name == "Alpha")
			found_alpha = true;

		if (config.display_name == "Beta")
			found_beta = true;
	}

	REQUIRE(found_alpha);
	REQUIRE(found_beta);

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_all, empty directory returns empty vector", "[u]")
{
	const auto directory = temp_providers_dir();

	for (const auto & entry : std::filesystem::directory_iterator(directory))
		std::filesystem::remove(entry.path());

	auto configs = web_translator_config::load_all(directory);

	REQUIRE(configs.empty());

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_all, nonexistent directory returns empty vector", "[u]")
{
	auto configs = web_translator_config::load_all("/nonexistent/directory");
	REQUIRE(configs.empty());
}

TEST_CASE("web_translator_config::load_all, skips invalid json files", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "good.json", R"({"name": "Good", "endpoint": "https://good.com"})");
	write_file(directory, "bad.json", "{ invalid json }}}}");

	auto configs = web_translator_config::load_all(directory);

	REQUIRE(configs.size() == 1);
	REQUIRE(configs[0].display_name == "Good");

	cleanup_dir(directory);
}

TEST_CASE("web_translator_config::load_single, identifier is file stem", "[u]")
{
	const auto directory = temp_providers_dir();
	write_file(directory, "my-custom-provider.json", R"({"name": "Custom", "endpoint": "https://x.com"})");

	auto config = web_translator_config::load_single(directory + "/my-custom-provider.json");

	REQUIRE(config.identifier == "my-custom-provider");
	REQUIRE(config.display_name == "Custom");

	cleanup_dir(directory);
}
