#pragma once

#include <string>
#include <QWidget>

class QComboBox;
class QLineEdit;
class settings_store_t;

class language_settings_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit language_settings_view_t(const std::string & dictionaries_dir, QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;

private:
	void on_native_language_changed(int index);
	void on_foreign_language_changed(int index);
	void scan_dictionaries(const std::string & directory);
	void update_spell_combo(QComboBox * combo, const char * prefix);

	QComboBox * m_foreign_language_combo = nullptr;
	QComboBox * m_native_language_combo = nullptr;
	QComboBox * m_foreign_spell_combo = nullptr;
	QComboBox * m_native_spell_combo = nullptr;
	QLineEdit * m_foreign_tag_edit = nullptr;
	QLineEdit * m_native_tag_edit = nullptr;
	std::string m_dictionaries_dir;
};
