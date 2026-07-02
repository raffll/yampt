#pragma once

#include <string>
#include <vector>
#include <QSettings>
#include <QString>
#include <utility/theme_enums.hpp>

class app_settings_t
{
public:
	explicit app_settings_t(const QString & filename);

	int encoding_index() const;
	void set_encoding_index(int index);

	std::string native_language() const;
	void set_native_language(const std::string & value);

	std::string foreign_language() const;
	void set_foreign_language(const std::string & value);

	std::string spell_aff_path() const;
	void set_spell_aff_path(const std::string & value);

	std::string spell_dic_path() const;
	void set_spell_dic_path(const std::string & value);

	int spell_lang_index() const;
	void set_spell_lang_index(int index);

	std::string translation_target() const;
	void set_translation_target(const std::string & value);

	std::string partial_dict_aff_path() const;
	void set_partial_dict_aff_path(const std::string & value);

	std::string partial_dict_dic_path() const;
	void set_partial_dict_dic_path(const std::string & value);

	std::string native_tag() const;
	void set_native_tag(const std::string & value);

	std::string foreign_tag() const;
	void set_foreign_tag(const std::string & value);

	std::string deepl_api_key() const;
	void set_deepl_api_key(const std::string & value);

	std::string google_api_key() const;
	void set_google_api_key(const std::string & value);

	int translation_source_index() const;
	void set_translation_source_index(int index);

	int translation_language_index() const;
	void set_translation_language_index(int index);

	std::string shortcut(const std::string & action_name) const;
	void set_shortcut(const std::string & action_name, const std::string & key_sequence);

	std::vector<std::string> workspace_roots() const;
	void set_workspace_roots(const std::vector<std::string> & roots);

	std::vector<std::string> last_merge_order() const;
	void set_last_merge_order(const std::vector<std::string> & paths);

	std::string active_dict_path() const;
	void set_active_dict_path(const std::string & value);

	std::string last_directory() const;
	void set_last_directory(const std::string & value);

	std::string openmw_data_dir() const;
	void set_openmw_data_dir(const std::string & value);

	std::string mo2_profile_dir() const;
	void set_mo2_profile_dir(const std::string & value);

	std::string openmw_merge_path() const;
	void set_openmw_merge_path(const std::string & value);

	std::string mo2_merge_path() const;
	void set_mo2_merge_path(const std::string & value);

	std::string merge_output_path() const;
	void set_merge_output_path(const std::string & value);

	float split_ratio() const;
	void set_split_ratio(float value);

	int sidebar_width() const;
	void set_sidebar_width(int value);

	int bottom_height() const;
	void set_bottom_height(int value);

	int info_height() const;
	void set_info_height(int value);

	bool sidebar_visible() const;
	void set_sidebar_visible(bool value);

	bool bottom_visible() const;
	void set_bottom_visible(bool value);

	int column_width(int index) const;
	void set_column_width(int index, int value);

	int window_x() const;
	void set_window_x(int value);

	int window_y() const;
	void set_window_y(int value);

	int window_width() const;
	void set_window_width(int value);

	int window_height() const;
	void set_window_height(int value);

	bool window_maximized() const;
	void set_window_maximized(bool value);

	theme_t theme() const;
	void set_theme(theme_t value);

	void sync();

private:
	QSettings m_settings;
};
