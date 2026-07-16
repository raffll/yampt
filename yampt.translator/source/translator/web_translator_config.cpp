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
	config.method = root.value("method").toString("POST").toStdString();
	config.response_path = root.value("response_path").toString().toStdString();
	config.quota_limit = root.value("quota_limit").toInt(0);
	config.system_prompt = root.value("system_prompt").toString().toStdString();

	const auto format_str = root.value("body_format").toString("json").toStdString();
	config.body_format = (format_str == "form") ? body_format_t::form : body_format_t::json;

	const auto kind_str = root.value("kind").toString("simple").toStdString();
	config.kind = (kind_str == "chat_completion") ? provider_kind_t::chat_completion : provider_kind_t::simple;

	const auto headers_obj = root.value("headers").toObject();
	for (auto it = headers_obj.begin(); it != headers_obj.end(); ++it)
		config.headers[it.key().toStdString()] = it.value().toString().toStdString();

	const auto body_obj = root.value("body").toObject();
	for (auto it = body_obj.begin(); it != body_obj.end(); ++it)
		config.body_fields[it.key().toStdString()] = it.value().toString().toStdString();

	return config;
}

web_translator_config_t web_translator_config::load_single(const std::string & json_path)
{
	namespace fs = std::filesystem;

	std::ifstream file(json_path);
	if (!file.is_open())
		return {};

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	auto document = QJsonDocument::fromJson(QByteArray::fromStdString(content));
	if (!document.isObject())
		return {};

	const auto stem = fs::path(json_path).stem().string();
	return parse_config(document.object(), stem);
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
