#pragma once

#include <string>
#include <unordered_map>
#include <vector>

enum class body_format_t
{
	json,
	form,
	query
};

enum class provider_kind_t
{
	simple,
	chat_completion
};

enum class message_style_t
{
	openai,
	anthropic
};

enum class setting_type_t
{
	password,
	text,
	choice
};

struct provider_setting_t
{
	std::string key;
	std::string label;
	setting_type_t type = setting_type_t::text;
	std::vector<std::string> choices;
	std::string default_value;
	bool required = true;
};

struct web_translator_config_t
{
	std::string identifier;
	std::string display_name;
	std::string endpoint;
	body_format_t body_format = body_format_t::json;
	provider_kind_t kind = provider_kind_t::simple;
	message_style_t message_style = message_style_t::openai;
	std::unordered_map<std::string, std::string> headers;
	std::unordered_map<std::string, std::string> body_fields;
	std::string response_path;
	std::string system_prompt;
	std::string models_endpoint;
	std::string models_path;
	std::string models_id_key;
	int quota_limit = 0;
	std::vector<provider_setting_t> settings;
};

namespace web_translator_config {

std::vector<web_translator_config_t> load_all(const std::string & providers_dir);
web_translator_config_t load_single(const std::string & json_path);
web_translator_config_t parse_string(const std::string & json_content, const std::string & identifier);

} // namespace web_translator_config
