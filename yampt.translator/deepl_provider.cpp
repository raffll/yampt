#include "deepl_provider.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

deepl_provider_t::deepl_provider_t(QObject * parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this))
{}

std::string deepl_provider_t::name() const
{
	return "DeepL Free";
}

bool deepl_provider_t::is_available() const
{
	return !api_key_.empty();
}

bool deepl_provider_t::is_async() const
{
	return true;
}

bool deepl_provider_t::has_quota() const
{
	return true;
}

int deepl_provider_t::remaining_quota() const
{
	return free_tier_limit_ - chars_used_;
}

void deepl_provider_t::set_api_key(const std::string & key)
{
	api_key_ = key;
}

void deepl_provider_t::set_chars_used(int chars)
{
	chars_used_ = chars;
}

int deepl_provider_t::chars_used() const
{
	return chars_used_;
}

void deepl_provider_t::translate(const std::string & text, const std::string & target_lang)
{
	if (api_key_.empty())
	{
		emit translation_finished({ "", false, "No API key configured" });
		return;
	}

	auto target = QString::fromStdString(target_lang).toUpper();

	QUrl url("https://api-free.deepl.com/v2/translate");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	request.setRawHeader("Authorization", QByteArray("DeepL-Auth-Key ") + QByteArray::fromStdString(api_key_));

	QUrlQuery params;
	params.addQueryItem("text", QString::fromStdString(text));
	params.addQueryItem("target_lang", target);
	params.addQueryItem("source_lang", "EN");

	auto * reply = network_->post(request, params.query(QUrl::FullyEncoded).toUtf8());
	connect(
	    reply,
	    &QNetworkReply::finished,
	    this,
	    [this, reply, text_len = static_cast<int>(text.size())]()
	{
		on_reply_finished(reply);
		chars_used_ += text_len;
	});
}

void deepl_provider_t::on_reply_finished(QNetworkReply * reply)
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
