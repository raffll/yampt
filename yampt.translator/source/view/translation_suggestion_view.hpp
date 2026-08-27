#pragma once

#include "../translator/translator.hpp"
#include "../translator/web_translator_config.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <translation_example.hpp>
#include <QWidget>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

class settings_store_t;
class ctranslate2_translator_t;
class web_translator_t;

class translation_suggestion_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_suggestion_view_t(QWidget * parent = nullptr);

	void set_source_text(const std::string & text);
	void set_models_dir(const std::string & dir);
	void set_providers_dir(const std::string & dir);
	void set_target_language(const std::string & language);
	void set_glossary_fn(std::function<std::string(const std::string &)> fn);
	void set_examples(const std::vector<translation_example_t> & examples);

	void set_settings_store(settings_store_t & settings);
	void apply_provider_settings(const settings_store_t & settings);
	void update_provider_status();

	void select_provider(int index);
	translator_t * active_provider() const;

	void append_log(const std::string & msg);
	void display_translation_result(const translation_suggestion_t & result);

	void request_translation(const std::string & text);
	void request_translation_lines(const std::vector<std::string> & lines);
	bool is_translating() const;

signals:
	void translate_all_requested();
	void translation_committed(const std::string & result_text);
	void translation_lines_committed(const std::vector<std::string> & result_lines);
	void translation_failed(const std::string & error_message);

private:
	void setup_controls();
	void load_model_for_language(int index);
	void rebuild_language_list();
	void rebuild_web_providers();
	void on_provider_result(const translation_suggestion_t & result);
	void advance_line_queue();
	void update_model_controls();
	web_translator_t * active_web_provider() const;
	void populate_model_combo(const std::vector<std::string> & models, const std::string & selected);

	QComboBox * m_provider_combo = nullptr;
	QComboBox * m_model_combo = nullptr;
	QPushButton * m_refresh_models = nullptr;
	QPushButton * m_translate_all_btn = nullptr;
	QPlainTextEdit * m_result_text = nullptr;
	QLabel * m_status_label = nullptr;

	settings_store_t * m_settings = nullptr;

	std::string m_source_text;
	std::string m_models_dir;
	std::string m_providers_dir;
	std::string m_target_language;
	std::function<std::string(const std::string &)> m_glossary_fn;
	std::vector<translation_example_t> m_examples;

	ctranslate2_translator_t * m_ct2_provider = nullptr;
	std::vector<web_translator_t *> m_web_providers;
	std::vector<translator_t *> m_providers;
	int m_active_provider_index = 0;
	bool m_translating = false;

	std::vector<std::string> m_line_queue;
	std::vector<std::string> m_line_results;

	struct lang_entry_t
	{
		std::string code;
		std::string display;
		std::string model_path;
	};

	std::vector<lang_entry_t> m_languages;
};
