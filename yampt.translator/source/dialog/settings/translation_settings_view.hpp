#pragma once

#include "../../translator/web_translator_config.hpp"
#include <string>
#include <vector>
#include <QWidget>

class QLineEdit;
class QTableWidget;
class settings_store_t;

class translation_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit translation_settings_view_t(const std::string & providers_dir, QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;

private:
	void rebuild_table();

	std::string m_providers_dir;
	std::vector<web_translator_config_t> m_configs;
	QTableWidget * m_provider_table = nullptr;
};
