#pragma once

#include <string>
#include <unordered_map>
#include <vector>

enum class body_format_t
{
	json,
	form
};

enum class provider_kind_t
{
	simple,
	chat_completion
};

struct web_translator_config_t
{
	std::string identifier;
	std::string display_name;
	std::string endpoint;
	std::string method;
	body_format_t body_format = body_format_t::json;
	provider_kind_t kind = provider_kind_t::simple;
	std::unordered_map<std::string, std::string> headers;
	std::unordered_map<std::string, std::string> body_fields;
	std::string response_path;
	std::string system_prompt;
	int quota_limit = 0;
};

namespace web_translator_config
{

std::vector<web_translator_config_t> load_all(const std::string & providers_dir);
web_translator_config_t load_single(const std::string & json_path);

}
