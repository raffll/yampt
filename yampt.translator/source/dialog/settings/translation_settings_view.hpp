#pragma once

#include "../../translator/web_translator_config.hpp"
#include <translation_example.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QVBoxLayout;
class settings_store_t;

class translation_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_settings_view_t(
	    const std::string & providers_dir,
	    const std::string & models_dir,
	    QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;

private:
	struct setting_widget_t
	{
		std::string provider_id;
		std::string setting_key;
		setting_type_t type;
		QWidget * widget = nullptr;
	};

	struct provider_card_t
	{
		std::string identifier;
		QLabel * status_label = nullptr;
	};

	void build_local_models_section(QVBoxLayout * parent, const std::string & models_dir);
	void build_preview_tab(QVBoxLayout * parent);
	void update_prompt_preview();
	void build_provider_card(QVBoxLayout * parent, const web_translator_config_t & config);
	void build_prompt_tab(QVBoxLayout * parent);
	void build_examples_tab(QVBoxLayout * parent);
	void rebuild_examples_table();
	void update_status(const provider_card_t & card);
	std::string read_widget_value(const setting_widget_t & entry) const;

	std::string m_providers_dir;
	std::vector<web_translator_config_t> m_configs;
	std::vector<setting_widget_t> m_setting_widgets;
	std::vector<provider_card_t> m_provider_cards;

	QPlainTextEdit * m_prompt_edit = nullptr;
	QPlainTextEdit * m_prompt_preview = nullptr;
	std::string m_preview_source_lang;
	std::string m_preview_target_lang;

	std::vector<translation_example_t> m_examples;
	QTableWidget * m_examples_table = nullptr;
	QLabel * m_examples_empty_label = nullptr;
	QPushButton * m_examples_remove_button = nullptr;
};
