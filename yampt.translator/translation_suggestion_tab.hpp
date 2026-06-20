#pragma once

#include "translation_provider.hpp"

#include <QWidget>
#include <memory>
#include <string>
#include <vector>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

class ctranslate2_provider_t;
class deepl_provider_t;
class google_provider_t;

class translation_suggestion_tab_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_suggestion_tab_t(QWidget * parent = nullptr);

	void set_source_text(const std::string & text);
	void set_models_dir(const std::string & dir);
	void set_glossary_fn(std::function<std::string(const std::string &)> fn);

	int language_index() const;
	void set_language_index(int index);

	ctranslate2_provider_t * ct2_provider() const;
	void append_log(const std::string & msg);
	void set_translate_all_enabled(bool enabled);

signals:
	void translate_all_requested();

private slots:
	void on_language_changed(int index);

private:
	void update_counter_label();
	void load_model_for_language(int index);
	void rebuild_language_combo();

	QComboBox * source_combo_ = nullptr;
	QComboBox * language_combo_ = nullptr;
	QPushButton * translate_all_btn_ = nullptr;
	QPlainTextEdit * result_text_ = nullptr;
	QLabel * counter_label_ = nullptr;

	std::string source_text_;
	std::string models_dir_;
	std::function<std::string(const std::string &)> glossary_fn_;

	ctranslate2_provider_t * ct2_provider_ = nullptr;
	deepl_provider_t * deepl_provider_ = nullptr;
	google_provider_t * google_provider_ = nullptr;
	std::vector<translation_provider_t *> providers_;

	struct lang_entry_t
	{
		std::string code;
		std::string display;
		std::string model_path;
	};

	std::vector<lang_entry_t> languages_;
};
