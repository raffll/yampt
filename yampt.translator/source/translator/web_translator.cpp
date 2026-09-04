#include "web_translator.hpp"
#include "model_list_utils.hpp"
#include "translation_example_ops.hpp"
#include "web_response_utils.hpp"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace {

std::string replace_placeholder(std::string text, const std::string & placeholder, const std::string & value)
{
	size_t position = 0;
	while ((position = text.find(placeholder, position)) != std::string::npos)
	{
		text.replace(position, placeholder.size(), value);
		position += value.size();
	}

	return text;
}

} // namespace

web_translator_t::web_translator_t(const web_translator_config_t & config, QObject * parent)
    : QObject(parent)
    , m_config(config)
    , m_network(new QNetworkAccessManager(this))
{
	if (m_config.system_prompt.empty())
		m_config.system_prompt = web_translator_config::default_system_prompt();
}

std::string web_translator_t::name() const
{
	return m_config.display_name;
}

bool web_translator_t::is_available() const
{
	if (m_config.settings.empty())
		return true;

	for (const auto & setting : m_config.settings)
	{
		if (!setting.required)
			continue;

		auto it = m_settings.find(setting.key);
		if (it == m_settings.end() || it->second.empty())
			return false;
	}

	return true;
}

bool web_translator_t::is_async() const
{
	return true;
}

bool web_translator_t::has_quota() const
{
	return m_config.quota_limit > 0;
}

int web_translator_t::remaining_quota() const
{
	if (m_config.quota_limit <= 0)
		return -1;

	return m_config.quota_limit - m_chars_used;
}

void web_translator_t::set_api_key(const std::string & key)
{
	m_settings["api_key"] = key;
}

std::string web_translator_t::api_key() const
{
	auto it = m_settings.find("api_key");
	return (it != m_settings.end()) ? it->second : std::string {};
}

void web_translator_t::set_provider_settings(const std::unordered_map<std::string, std::string> & settings)
{
	m_settings = settings;
}

void web_translator_t::set_setting(const std::string & key, const std::string & value)
{
	m_settings[key] = value;
}

void web_translator_t::set_source_language(const std::string & language)
{
	m_source_language = language;
}

std::string web_translator_t::source_language() const
{
	return m_source_language;
}

void web_translator_t::set_system_prompt(const std::string & prompt)
{
	m_config.system_prompt = prompt.empty() ? web_translator_config::default_system_prompt() : prompt;
}

const web_translator_config_t & web_translator_t::config() const
{
	return m_config;
}

void web_translator_t::set_glossary_fn(std::function<std::string(const std::string &)> glossary_fn)
{
	m_glossary_fn = std::move(glossary_fn);
}

void web_translator_t::set_examples(const std::vector<translation_example_t> & examples)
{
	m_examples = examples;
}

void web_translator_t::translate(const std::string & text, const std::string & target_lang)
{
	if (!is_available())
	{
		emit translation_finished(
		    { "", false, QCoreApplication::translate("yTranslator", "provider not configured").toStdString() });
		return;
	}

	if (m_config.kind == provider_kind_t::chat_completion)
		send_chat_request(text, target_lang);
	else
		send_simple_request(text, target_lang);
}

void web_translator_t::fetch_models()
{
	if (m_config.models_endpoint.empty())
	{
		emit models_fetch_failed(
		    QCoreApplication::translate("yTranslator", "Provider has no models endpoint").toStdString());
		return;
	}

	if (!is_available())
	{
		emit models_fetch_failed(
		    QCoreApplication::translate("yTranslator", "provider not configured").toStdString());
		return;
	}

	QNetworkRequest request(QUrl(QString::fromStdString(m_config.models_endpoint)));

	for (const auto & [header_name, header_value] : m_config.headers)
	{
		const auto expanded = expand_template(header_value, "", "");
		request.setRawHeader(QByteArray::fromStdString(header_name), QByteArray::fromStdString(expanded));
	}

	auto * reply = m_network->get(request);
	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply]()
	{
		reply->deleteLater();

		if (reply->error() != QNetworkReply::NoError)
		{
			emit models_fetch_failed(reply->errorString().toStdString());
			return;
		}

		const auto models = extract_model_list(reply->readAll());
		if (models.empty())
		{
			emit models_fetch_failed(
			    QCoreApplication::translate("yTranslator", "Empty or unparseable models response").toStdString());
			return;
		}

		emit models_fetched(models);
	});
}

std::string web_translator_t::expand_template(
    const std::string & tmpl,
    const std::string & text,
    const std::string & target_lang) const
{
	std::string result = tmpl;

	auto replace_all = [&](const std::string & placeholder, const std::string & value)
	{
		size_t position = 0;
		while ((position = result.find(placeholder, position)) != std::string::npos)
		{
			result.replace(position, placeholder.size(), value);
			position += value.size();
		}
	};

	replace_all("{{text}}", text);
	replace_all("{{target_lang}}", target_lang);
	replace_all("{{source_lang}}", m_source_language);

	for (const auto & [setting_key, setting_value] : m_settings)
		replace_all("{{" + setting_key + "}}", setting_value);

	return result;
}

void web_translator_t::send_simple_request(const std::string & text, const std::string & target_lang)
{
	QUrl url(QString::fromStdString(expand_template(m_config.endpoint, text, target_lang)));
	QNetworkRequest request;

	for (const auto & [header_name, header_value] : m_config.headers)
	{
		auto expanded = expand_template(header_value, text, target_lang);
		request.setRawHeader(QByteArray::fromStdString(header_name), QByteArray::fromStdString(expanded));
	}

	QNetworkReply * reply = nullptr;

	if (m_config.body_format == body_format_t::query)
	{
		QUrlQuery params;
		for (const auto & [field_name, field_template] : m_config.body_fields)
		{
			auto value = expand_template(field_template, text, target_lang);
			params.addQueryItem(QString::fromStdString(field_name), QString::fromStdString(value));
		}
		url.setQuery(params);
		request.setUrl(url);
		reply = m_network->get(request);
	}
	else if (m_config.body_format == body_format_t::form)
	{
		request.setUrl(url);
		if (!request.hasRawHeader("Content-Type"))
			request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

		QUrlQuery params;
		for (const auto & [field_name, field_template] : m_config.body_fields)
		{
			auto value = expand_template(field_template, text, target_lang);
			params.addQueryItem(QString::fromStdString(field_name), QString::fromStdString(value));
		}
		reply = m_network->post(request, params.query(QUrl::FullyEncoded).toUtf8());
	}
	else
	{
		request.setUrl(url);
		QJsonObject body_obj;
		for (const auto & [field_name, field_template] : m_config.body_fields)
		{
			auto value = expand_template(field_template, text, target_lang);
			body_obj[QString::fromStdString(field_name)] = web_response_utils::json_value_from_string(value);
		}
		reply = m_network->post(request, QJsonDocument(body_obj).toJson(QJsonDocument::Compact));
	}

	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply, text_len = static_cast<int>(text.size())]()
	{
		const bool success = (reply->error() == QNetworkReply::NoError);
		on_reply_finished(reply);
		if (success)
			m_chars_used += text_len;
	});
}

void web_translator_t::send_chat_request(const std::string & text, const std::string & target_lang)
{
	QUrl url(QString::fromStdString(m_config.endpoint));
	QNetworkRequest request(url);

	for (const auto & [header_name, header_value] : m_config.headers)
	{
		auto expanded = expand_template(header_value, text, target_lang);
		request.setRawHeader(QByteArray::fromStdString(header_name), QByteArray::fromStdString(expanded));
	}

	auto system_prompt = expand_template(m_config.system_prompt, text, target_lang);

	const auto examples_text = translation_example_ops::format_examples_lines(m_examples);
	system_prompt = replace_placeholder(system_prompt, "{{examples}}", examples_text);

	const auto glossary_text = m_glossary_fn ? m_glossary_fn(text) : std::string {};
	system_prompt = replace_placeholder(system_prompt, "{{hyperlinks}}", glossary_text);

	QJsonObject body_obj;
	for (const auto & [field_name, field_template] : m_config.body_fields)
	{
		auto value = expand_template(field_template, text, target_lang);
		body_obj[QString::fromStdString(field_name)] = QString::fromStdString(value);
	}

	if (m_config.message_style == message_style_t::anthropic)
	{
		body_obj["system"] = QString::fromStdString(system_prompt);

		QJsonArray messages;
		QJsonObject user_message;
		user_message["role"] = "user";
		user_message["content"] = QString::fromStdString(text);
		messages.append(user_message);
		body_obj["messages"] = messages;
	}
	else
	{
		QJsonArray messages;
		QJsonObject system_message;
		system_message["role"] = "system";
		system_message["content"] = QString::fromStdString(system_prompt);
		messages.append(system_message);

		QJsonObject user_message;
		user_message["role"] = "user";
		user_message["content"] = QString::fromStdString(text);
		messages.append(user_message);
		body_obj["messages"] = messages;
	}

	auto body_data = QJsonDocument(body_obj).toJson(QJsonDocument::Compact);

	auto * reply = m_network->post(request, body_data);
	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply, text_len = static_cast<int>(text.size())]()
	{
		const bool success = (reply->error() == QNetworkReply::NoError);
		on_reply_finished(reply);
		if (success)
			m_chars_used += text_len;
	});
}

void web_translator_t::on_reply_finished(QNetworkReply * reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit translation_finished({ "", false, reply->errorString().toStdString() });
		return;
	}

	auto data = reply->readAll();
	auto result_text = extract_response(data);

	if (result_text.empty())
	{
		emit translation_finished(
		    { "", false, QCoreApplication::translate("yTranslator", "Empty or unparseable response").toStdString() });
		return;
	}

	emit translation_finished({ result_text, true, "" });
}

std::string web_translator_t::extract_response(const QByteArray & data) const
{
	const auto document = QJsonDocument::fromJson(data);
	return web_response_utils::extract_by_path(document, m_config.response_path);
}

std::vector<std::string> web_translator_t::extract_model_list(const QByteArray & data) const
{
	const auto document = QJsonDocument::fromJson(data);
	return model_list_utils::extract_model_list(
	    document, model_list_utils::model_list_path_t { m_config.models_path, m_config.models_id_key });
}
