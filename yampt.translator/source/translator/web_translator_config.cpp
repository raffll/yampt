#include "web_translator_config.hpp"
#include <filesystem>
#include <fstream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

static web_translator_config_t parse_config(const QJsonObject & root, const std::string & file_stem)
{
	web_translator_config_t config;
	config.identifier = file_stem;
	config.display_name = root.value("name").toString().toStdString();
	config.endpoint = root.value("endpoint").toString().toStdString();
	config.response_path = root.value("response_path").toString().toStdString();
	config.quota_limit = root.value("quota_limit").toInt(0);
	config.models_endpoint = root.value("models_endpoint").toString().toStdString();
	config.models_path = root.value("models_path").toString().toStdString();
	config.models_id_key = root.value("models_id_key").toString("id").toStdString();

	const auto format_str = root.value("body_format").toString("json").toStdString();
	if (format_str == "form")
		config.body_format = body_format_t::form;
	else if (format_str == "query")
		config.body_format = body_format_t::query;
	else
		config.body_format = body_format_t::json;

	const auto kind_str = root.value("kind").toString("simple").toStdString();
	config.kind = (kind_str == "chat_completion") ? provider_kind_t::chat_completion : provider_kind_t::simple;

	const auto style_str = root.value("message_style").toString("openai").toStdString();
	config.message_style = (style_str == "anthropic") ? message_style_t::anthropic : message_style_t::openai;

	const auto headers_obj = root.value("headers").toObject();
	for (auto it = headers_obj.begin(); it != headers_obj.end(); ++it)
		config.headers[it.key().toStdString()] = it.value().toString().toStdString();

	const auto body_obj = root.value("body").toObject();
	for (auto it = body_obj.begin(); it != body_obj.end(); ++it)
		config.body_fields[it.key().toStdString()] = it.value().toString().toStdString();

	const auto settings_arr = root.value("settings").toArray();
	for (const auto & item : settings_arr)
	{
		const auto obj = item.toObject();
		provider_setting_t setting;
		setting.key = obj.value("key").toString().toStdString();
		setting.label = obj.value("label").toString().toStdString();
		setting.default_value = obj.value("default").toString().toStdString();
		setting.required = obj.value("required").toBool(true);

		const auto type_str = obj.value("type").toString("text").toStdString();
		if (type_str == "password")
			setting.type = setting_type_t::password;
		else if (type_str == "choice")
			setting.type = setting_type_t::choice;
		else
			setting.type = setting_type_t::text;

		const auto choices_arr = obj.value("choices").toArray();
		for (const auto & choice : choices_arr)
			setting.choices.push_back(choice.toString().toStdString());

		config.settings.push_back(std::move(setting));
	}

	return config;
}

const std::string & web_translator_config::default_system_prompt()
{
	static const std::string prompt =
	    "You are a translator for the video game Morrowind. Translate the given text from "
	    "{{source_lang_upper}} to {{target_lang}}. Output only the translated text, nothing else. "
	    "Preserve all HTML tags, line breaks, and formatting exactly as they appear.";
	return prompt;
}

web_translator_config_t web_translator_config::parse_string(const std::string & json_content, const std::string & identifier)
{
	auto document = QJsonDocument::fromJson(QByteArray::fromStdString(json_content));
	if (!document.isObject())
		return { .identifier = identifier };

	return parse_config(document.object(), identifier);
}

web_translator_config_t web_translator_config::load_single(const std::string & json_path)
{
	namespace fs = std::filesystem;

	const auto stem = fs::path(json_path).stem().string();

	std::ifstream file(json_path);
	if (!file.is_open())
		return { .identifier = stem };

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	return parse_string(content, stem);
}

std::vector<web_translator_config_t> web_translator_config::load_all(const std::string & providers_dir)
{
	namespace fs = std::filesystem;

	std::vector<web_translator_config_t> configs;

	if (!fs::is_directory(providers_dir))
		return configs;

	for (const auto & entry : fs::directory_iterator(providers_dir))
	{
		if (!entry.is_regular_file())
			continue;

		if (entry.path().extension() != ".json")
			continue;

		auto config = load_single(entry.path().string());
		if (!config.display_name.empty())
			configs.push_back(std::move(config));
	}

	return configs;
}
