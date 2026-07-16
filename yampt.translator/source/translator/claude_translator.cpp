#include "claude_translator.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

claude_translator_t::claude_translator_t(QObject * parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{}

std::string claude_translator_t::name() const
{
	return "Claude";
}

bool claude_translator_t::is_available() const
{
	return !m_api_key.empty();
}

bool claude_translator_t::is_async() const
{
	return true;
}

bool claude_translator_t::has_quota() const
{
	return false;
}

int claude_translator_t::remaining_quota() const
{
	return 0;
}

void claude_translator_t::set_api_key(const std::string & key)
{
	m_api_key = key;
}

void claude_translator_t::set_glossary_fn(std::function<std::string(const std::string &)> glossary_fn)
{
	m_glossary_fn = std::move(glossary_fn);
}

int claude_translator_t::input_tokens_used() const
{
	return m_input_tokens;
}

int claude_translator_t::output_tokens_used() const
{
	return m_output_tokens;
}

std::string claude_translator_t::build_system_prompt(const std::string & target_lang) const
{
	std::string prompt =
	    "You are a translator for the video game Morrowind. "
	    "Translate the given text from English to " + target_lang + ". "
	    "Output only the translated text, nothing else. "
	    "Preserve all HTML tags, line breaks, and formatting exactly as they appear.";

	if (m_glossary_fn && !m_last_source_text.empty())
	{
		const auto glossary_text = m_glossary_fn(m_last_source_text);
		if (!glossary_text.empty())
			prompt += "\n\nUse these established translations as reference:\n" + glossary_text;
	}

	return prompt;
}

std::string claude_translator_t::build_user_prompt(const std::string & text) const
{
	return text;
}

void claude_translator_t::translate(const std::string & text, const std::string & target_lang)
{
	if (m_api_key.empty())
	{
		emit translation_finished({ "", false, "No API key configured" });
		return;
	}

	m_last_source_text = text;

	QUrl url("https://api.anthropic.com/v1/messages");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("x-api-key", QByteArray::fromStdString(m_api_key));
	request.setRawHeader("anthropic-version", "2023-06-01");

	QJsonObject body;
	body["model"] = "claude-sonnet-4-20250514";
	body["max_tokens"] = 4096;
	body["system"] = QString::fromStdString(build_system_prompt(target_lang));

	QJsonArray messages;
	QJsonObject user_message;
	user_message["role"] = "user";
	user_message["content"] = QString::fromStdString(build_user_prompt(text));
	messages.append(user_message);
	body["messages"] = messages;

	auto * reply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
	connect(reply, &QNetworkReply::finished, this, [this, reply]() { on_reply_finished(reply); });
}

void claude_translator_t::on_reply_finished(QNetworkReply * reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit translation_finished({ "", false, reply->errorString().toStdString() });
		return;
	}

	auto data = reply->readAll();
	auto document = QJsonDocument::fromJson(data);
	if (!document.isObject())
	{
		emit translation_finished({ "", false, "Invalid JSON response" });
		return;
	}

	auto root = document.object();

	auto usage = root.value("usage").toObject();
	m_input_tokens += usage.value("input_tokens").toInt();
	m_output_tokens += usage.value("output_tokens").toInt();

	auto content = root.value("content").toArray();
	if (content.isEmpty())
	{
		emit translation_finished({ "", false, "No content in response" });
		return;
	}

	auto result_text = content[0].toObject().value("text").toString().toStdString();
	emit translation_finished({ result_text, true, "" });
}
