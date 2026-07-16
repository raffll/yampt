#include "web_translator.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

web_translator_t::web_translator_t(const web_translator_config_t & config, QObject * parent)
    : QObject(parent)
    , m_config(config)
    , m_network(new QNetworkAccessManager(this))
{}

std::string web_translator_t::name() const
{
	return m_config.display_name;
}

bool web_translator_t::is_available() const
{
	return !m_api_key.empty();
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
	m_api_key = key;
}

std::string web_translator_t::api_key() const
{
	return m_api_key;
}

const web_translator_config_t & web_translator_t::config() const
{
	return m_config;
}

void web_translator_t::set_glossary_fn(std::function<std::string(const std::string &)> glossary_fn)
{
	m_glossary_fn = std::move(glossary_fn);
}

void web_translator_t::translate(const std::string & text, const std::string & target_lang)
{
	if (m_api_key.empty())
	{
		emit translation_finished({ "", false, "No API key configured" });
		return;
	}

	if (m_config.kind == provider_kind_t::chat_completion)
		send_chat_request(text, target_lang);
	else
		send_simple_request(text, target_lang);
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

	replace_all("{{api_key}}", m_api_key);
	replace_all("{{text}}", text);
	replace_all("{{target_lang}}", target_lang);

	auto upper_lang = QString::fromStdString(target_lang).toUpper().toStdString();
	replace_all("{{target_lang_upper}}", upper_lang);

	return result;
}

void web_translator_t::send_simple_request(const std::string & text, const std::string & target_lang)
{
	QUrl url(QString::fromStdString(m_config.endpoint));
	QNetworkRequest request(url);

	for (const auto & [header_name, header_value] : m_config.headers)
	{
		auto expanded = expand_template(header_value, text, target_lang);
		request.setRawHeader(
		    QByteArray::fromStdString(header_name),
		    QByteArray::fromStdString(expanded));
	}

	QByteArray body_data;

	if (m_config.body_format == body_format_t::form)
	{
		QUrlQuery params;
		for (const auto & [field_name, field_template] : m_config.body_fields)
		{
			auto value = expand_template(field_template, text, target_lang);
			params.addQueryItem(
			    QString::fromStdString(field_name),
			    QString::fromStdString(value));
		}
		body_data = params.query(QUrl::FullyEncoded).toUtf8();
	}
	else
	{
		QJsonObject body_obj;
		for (const auto & [field_name, field_template] : m_config.body_fields)
		{
			auto value = expand_template(field_template, text, target_lang);
			body_obj[QString::fromStdString(field_name)] = QString::fromStdString(value);
		}
		body_data = QJsonDocument(body_obj).toJson(QJsonDocument::Compact);
	}

	auto * reply = m_network->post(request, body_data);
	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply, text_len = static_cast<int>(text.size())]()
	{
		on_reply_finished(reply);
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
		request.setRawHeader(
		    QByteArray::fromStdString(header_name),
		    QByteArray::fromStdString(expanded));
	}

	auto system_prompt = expand_template(m_config.system_prompt, text, target_lang);

	if (m_glossary_fn)
	{
		const auto glossary_text = m_glossary_fn(text);
		if (!glossary_text.empty())
			system_prompt += "\n\nUse these established translations as reference:\n" + glossary_text;
	}

	QJsonObject body_obj;
	for (const auto & [field_name, field_template] : m_config.body_fields)
	{
		auto value = expand_template(field_template, text, target_lang);
		body_obj[QString::fromStdString(field_name)] = QString::fromStdString(value);
	}

	body_obj["system"] = QString::fromStdString(system_prompt);

	QJsonArray messages;
	QJsonObject user_message;
	user_message["role"] = "user";
	user_message["content"] = QString::fromStdString(text);
	messages.append(user_message);
	body_obj["messages"] = messages;

	auto body_data = QJsonDocument(body_obj).toJson(QJsonDocument::Compact);

	auto * reply = m_network->post(request, body_data);
	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply, text_len = static_cast<int>(text.size())]()
	{
		on_reply_finished(reply);
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
		emit translation_finished({ "", false, "Empty or unparseable response" });
		return;
	}

	emit translation_finished({ result_text, true, "" });
}

std::string web_translator_t::extract_response(const QByteArray & data) const
{
	auto document = QJsonDocument::fromJson(data);
	if (!document.isObject() && !document.isArray())
		return {};

	auto path = QString::fromStdString(m_config.response_path);
	auto segments = path.split('.');

	QJsonValue current;
	if (document.isObject())
		current = QJsonValue(document.object());
	else
		current = QJsonValue(document.array());

	for (const auto & segment : segments)
	{
		if (segment.isEmpty())
			continue;

		auto bracket_pos = segment.indexOf('[');
		if (bracket_pos >= 0)
		{
			auto field_name = segment.left(bracket_pos);
			auto index_str = segment.mid(bracket_pos + 1, segment.indexOf(']') - bracket_pos - 1);
			auto index = index_str.toInt();

			if (!field_name.isEmpty())
			{
				if (!current.isObject())
					return {};

				current = current.toObject().value(field_name);
			}

			if (!current.isArray())
				return {};

			current = current.toArray().at(index);
		}
		else
		{
			if (!current.isObject())
				return {};

			current = current.toObject().value(segment);
		}
	}

	if (current.isString())
		return current.toString().toStdString();

	return {};
}
