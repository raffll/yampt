#include "google_provider.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

google_provider_t::google_provider_t(QObject * parent)
    : QObject(parent)
    , network_(new QNetworkAccessManager(this))
{
}

std::string google_provider_t::name() const
{
    return "Google Translate";
}

bool google_provider_t::is_available() const
{
    return true;
}

bool google_provider_t::is_async() const
{
    return true;
}

bool google_provider_t::has_quota() const
{
    return false;
}

int google_provider_t::remaining_quota() const
{
    return -1;
}

void google_provider_t::translate(const std::string & text, const std::string & target_lang)
{
    auto tl = QString::fromStdString(target_lang).toLower();

    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", "en");
    query.addQueryItem("tl", tl);
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", QString::fromStdString(text));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    auto * reply = network_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        on_reply_finished(reply);
    });
}

void google_provider_t::on_reply_finished(QNetworkReply * reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit translation_finished({"", false, reply->errorString().toStdString()});
        return;
    }

    auto data = reply->readAll();
    auto doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
    {
        emit translation_finished({"", false, "Invalid response format"});
        return;
    }

    auto outer = doc.array();
    if (outer.isEmpty() || !outer[0].isArray())
    {
        emit translation_finished({"", false, "No translation in response"});
        return;
    }

    auto sentences = outer[0].toArray();
    std::string result_text;
    for (const auto & sentence : sentences)
    {
        if (!sentence.isArray())
            continue;

        auto parts = sentence.toArray();
        if (parts.isEmpty())
            continue;

        result_text += parts[0].toString().toStdString();
    }

    if (result_text.empty())
    {
        emit translation_finished({"", false, "Empty translation"});
        return;
    }

    emit translation_finished({result_text, true, ""});
}
