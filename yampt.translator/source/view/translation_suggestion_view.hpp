#pragma once

#include "../translator/translator.hpp"
#include <memory>
#include <string>
#include <vector>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;

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

	ctranslate2_translator_t * ct2_provider() const;
	void append_log(const std::string & msg);
	void set_translate_all_enabled(bool enabled);

signals:
	void translate_all_requested();

private:
	void setup_controls();
	void update_counter_label();
	void load_model_for_language(int index);
	void rebuild_language_list();

	QPushButton * translate_all_btn_ = nullptr;
	QPlainTextEdit * result_text_ = nullptr;
	QLabel * counter_label_ = nullptr;

	std::string source_text_;
	std::string models_dir_;
	std::function<std::string(const std::string &)> glossary_fn_;

	ctranslate2_translator_t * ct2_provider_ = nullptr;
	deepl_translator_t * deepl_translator_ = nullptr;
	google_translator_t * google_translator_ = nullptr;
	std::vector<translator_t *> providers_;

	struct lang_entry_t
	{
		std::string code;
		std::string display;
		std::string model_path;
	};

	std::vector<lang_entry_t> languages_;
};
