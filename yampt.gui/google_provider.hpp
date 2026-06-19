#pragma once

#include "translation_provider.hpp"

#include <QObject>
#include <string>

class QNetworkAccessManager;
class QNetworkReply;

class google_provider_t : public QObject, public translation_provider_t
{
	Q_OBJECT

public:
	explicit google_provider_t(QObject * parent = nullptr);

	std::string name() const override;
	bool is_available() const override;
	bool is_async() const override;
	bool has_quota() const override;
	int remaining_quota() const override;

	void translate(const std::string & text, const std::string & target_lang) override;

signals:
	void translation_finished(translation_suggestion_t result);

private:
	void on_reply_finished(QNetworkReply * reply);

	QNetworkAccessManager * network_ = nullptr;
};
