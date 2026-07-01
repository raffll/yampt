#pragma once

#include "../translator/translator.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <QWidget>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;

class app_settings_t;
class ctranslate2_translator_t;
class deepl_translator_t;
class google_translator_t;

class translation_suggestion_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_suggestion_view_t(QWidget * parent = nullptr);

	void set_source_text(const std::string & text);
	void set_models_dir(const std::string & dir);
	void set_glossary_fn(std::function<std::string(const std::string &)> fn);

	void apply_provider_settings(const app_settings_t & settings);
	void update_provider_status();

	void select_provider(int index);
	void validate_api_key();
	translator_t * active_provider() const;

	ctranslate2_translator_t * ct2_provider() const;
	void append_log(const std::string & msg);
	void set_translate_all_enabled(bool enabled);
	void set_batch_in_progress(bool in_progress);

signals:
	void translate_all_requested();

private:
	void setup_controls();
	void setup_status_bar(QVBoxLayout * layout);
	void update_counter_label();
	void load_model_for_language(int index);
	void rebuild_language_list();
	void display_translation_result(const translation_suggestion_t & result);

	QComboBox * m_provider_combo = nullptr;
	QPushButton * m_translate_all_btn = nullptr;
	QPlainTextEdit * m_result_text = nullptr;
	QLabel * m_counter_label = nullptr;

	QLabel * m_ct2_status_label = nullptr;
	QLabel * m_deepl_status_label = nullptr;
	QLabel * m_google_status_label = nullptr;
	QLabel * m_api_key_error_label = nullptr;

	std::string m_source_text;
	std::string m_models_dir;
	std::function<std::string(const std::string &)> m_glossary_fn;

	ctranslate2_translator_t * m_ct2_provider = nullptr;
	deepl_translator_t * m_deepl_translator = nullptr;
	google_translator_t * m_google_translator = nullptr;
	std::vector<translator_t *> m_providers;
	int m_active_provider_index = 0;
	bool m_batch_in_progress = false;

	struct lang_entry_t
	{
		std::string code;
		std::string display;
		std::string model_path;
	};

	std::vector<lang_entry_t> m_languages;
};
