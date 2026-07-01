#include "deepl_translator.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

deepl_translator_t::deepl_translator_t(QObject * parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{}

std::string deepl_translator_t::name() const
{
	return "DeepL Free";
}

bool deepl_translator_t::is_available() const
{
	return !m_api_key.empty();
}

bool deepl_translator_t::is_async() const
{
	return true;
}

bool deepl_translator_t::has_quota() const
{
	return true;
}

int deepl_translator_t::remaining_quota() const
{
	return m_free_tier_limit - m_chars_used;
}

void deepl_translator_t::set_api_key(const std::string & key)
{
	m_api_key = key;
}

void deepl_translator_t::set_chars_used(int chars)
{
	m_chars_used = chars;
}

int deepl_translator_t::chars_used() const
{
	return m_chars_used;
}

void deepl_translator_t::translate(const std::string & text, const std::string & target_lang)
{
	if (m_api_key.empty())
	{
		emit translation_finished({ "", false, "No API key configured" });
		return;
	}

	auto target = QString::fromStdString(target_lang).toUpper();

	QUrl url("https://api-free.deepl.com/v2/translate");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Authorization", QByteArray("DeepL-Auth-Key ") + QByteArray::fromStdString(m_api_key));

	QUrlQuery params;
	params.addQueryItem("text", QString::fromStdString(text));
	params.addQueryItem("target_lang", target);
	params.addQueryItem("source_lang", "EN");

	auto * reply = m_network->post(request, params.query(QUrl::FullyEncoded).toUtf8());
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

void deepl_translator_t::on_reply_finished(QNetworkReply * reply)
{
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit translation_finished({ "", false, reply->errorString().toStdString() });
		return;
	}

	auto data = reply->readAll();
	auto doc = QJsonDocument::fromJson(data);
	if (!doc.isObject())
	{
		emit translation_finished({ "", false, "Invalid JSON response" });
		return;
	}

	auto translations = doc.object().value("translations").toArray();
	if (translations.isEmpty())
	{
		emit translation_finished({ "", false, "No translation in response" });
		return;
	}

	auto result_text = translations[0].toObject().value("text").toString().toStdString();
	emit translation_finished({ result_text, true, "" });
}
