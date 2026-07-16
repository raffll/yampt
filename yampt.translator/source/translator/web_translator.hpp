#pragma once

#include "translator.hpp"
#include "web_translator_config.hpp"
#include <functional>
#include <string>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

class web_translator_t : public QObject, public translator_t
{
	Q_OBJECT

public:
	explicit web_translator_t(const web_translator_config_t & config, QObject * parent = nullptr);

	std::string name() const override;
	bool is_available() const override;
	bool is_async() const override;
	bool has_quota() const override;
	int remaining_quota() const override;

	void translate(const std::string & text, const std::string & target_lang) override;

	void set_api_key(const std::string & key);
	std::string api_key() const;

	const web_translator_config_t & config() const;

	void set_glossary_fn(std::function<std::string(const std::string &)> glossary_fn);

signals:
	void translation_finished(translation_suggestion_t result);

private:
	void send_simple_request(const std::string & text, const std::string & target_lang);
	void send_chat_request(const std::string & text, const std::string & target_lang);
	void on_reply_finished(QNetworkReply * reply);
	std::string expand_template(const std::string & tmpl, const std::string & text, const std::string & target_lang) const;
	std::string extract_response(const QByteArray & data) const;

	web_translator_config_t m_config;
	QNetworkAccessManager * m_network = nullptr;
	std::string m_api_key;
	int m_chars_used = 0;
	std::function<std::string(const std::string &)> m_glossary_fn;
};
