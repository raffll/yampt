#pragma once

#include "translator.hpp"
#include <functional>
#include <string>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

class claude_translator_t : public QObject, public translator_t
{
	Q_OBJECT

public:
	explicit claude_translator_t(QObject * parent = nullptr);

	std::string name() const override;
	bool is_available() const override;
	bool is_async() const override;
	bool has_quota() const override;
	int remaining_quota() const override;

	void translate(const std::string & text, const std::string & target_lang) override;

	void set_api_key(const std::string & key);
	void set_glossary_fn(std::function<std::string(const std::string &)> glossary_fn);

	int input_tokens_used() const;
	int output_tokens_used() const;

signals:
	void translation_finished(translation_suggestion_t result);

private:
	void on_reply_finished(QNetworkReply * reply);
	std::string build_system_prompt(const std::string & target_lang) const;
	std::string build_user_prompt(const std::string & text) const;

	QNetworkAccessManager * m_network = nullptr;
	std::string m_api_key;
	std::function<std::string(const std::string &)> m_glossary_fn;
	int m_input_tokens = 0;
	int m_output_tokens = 0;
	std::string m_last_source_text;
};
