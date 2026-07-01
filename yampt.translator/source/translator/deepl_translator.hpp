#pragma once

#include "translator.hpp"
#include <string>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

class deepl_translator_t : public QObject, public translator_t
{
	Q_OBJECT

public:
	explicit deepl_translator_t(QObject * parent = nullptr);

	std::string name() const override;
	bool is_available() const override;
	bool is_async() const override;
	bool has_quota() const override;
	int remaining_quota() const override;

	void translate(const std::string & text, const std::string & target_lang) override;

	void set_api_key(const std::string & key);
	void set_chars_used(int chars);
	int chars_used() const;

signals:
	void translation_finished(translation_suggestion_t result);

private:
	void on_reply_finished(QNetworkReply * reply);

	QNetworkAccessManager * m_network = nullptr;
	std::string m_api_key;
	int m_chars_used = 0;
	static constexpr int m_free_tier_limit = 500000;
};
