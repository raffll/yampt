#include "settings_store.hpp"
#include "resource_paths.hpp"
#include <QCoreApplication>
#include <QDir>

QString settings_store_t::settings_dir()
{
	return QString::fromStdString(resource_paths::config_dir());
}

settings_store_t::settings_store_t(const QString & filename)
    : m_settings(settings_dir() + filename, QSettings::IniFormat)
{}

int settings_store_t::encoding_index() const
{
	return m_settings.value("Language/EncodingIndex", 0).toInt();
}

void settings_store_t::set_encoding_index(int index)
{
	m_settings.setValue("Language/EncodingIndex", index);
}

std::string settings_store_t::native_language() const
{
	return m_settings.value("Language/NativeLanguage", "").toString().toStdString();
}

void settings_store_t::set_native_language(const std::string & value)
{
	m_settings.setValue("Language/NativeLanguage", QString::fromStdString(value));
}

std::string settings_store_t::foreign_language() const
{
	return m_settings.value("Language/ForeignLanguage", "").toString().toStdString();
}

void settings_store_t::set_foreign_language(const std::string & value)
{
	m_settings.setValue("Language/ForeignLanguage", QString::fromStdString(value));
}

std::string settings_store_t::spell_aff_path() const
{
	return m_settings.value("Language/SpellAffPath", "").toString().toStdString();
}

void settings_store_t::set_spell_aff_path(const std::string & value)
{
	m_settings.setValue("Language/SpellAffPath", QString::fromStdString(value));
}

std::string settings_store_t::spell_dic_path() const
{
	return m_settings.value("Language/SpellDicPath", "").toString().toStdString();
}

void settings_store_t::set_spell_dic_path(const std::string & value)
{
	m_settings.setValue("Language/SpellDicPath", QString::fromStdString(value));
}

int settings_store_t::spell_lang_index() const
{
	return m_settings.value("Language/SpellLangIndex", 0).toInt();
}

void settings_store_t::set_spell_lang_index(int index)
{
	m_settings.setValue("Language/SpellLangIndex", index);
}

std::string settings_store_t::translation_target() const
{
	return m_settings.value("Language/TranslationTarget", "").toString().toStdString();
}

void settings_store_t::set_translation_target(const std::string & value)
{
	m_settings.setValue("Language/TranslationTarget", QString::fromStdString(value));
}

std::string settings_store_t::partial_dict_aff_path() const
{
	return m_settings.value("Language/PartialDictAffPath", "").toString().toStdString();
}

void settings_store_t::set_partial_dict_aff_path(const std::string & value)
{
	m_settings.setValue("Language/PartialDictAffPath", QString::fromStdString(value));
}

std::string settings_store_t::partial_dict_dic_path() const
{
	return m_settings.value("Language/PartialDictDicPath", "").toString().toStdString();
}

void settings_store_t::set_partial_dict_dic_path(const std::string & value)
{
	m_settings.setValue("Language/PartialDictDicPath", QString::fromStdString(value));
}

std::string settings_store_t::native_tag() const
{
	return m_settings.value("Language/NativeTag", "").toString().toStdString();
}

void settings_store_t::set_native_tag(const std::string & value)
{
	m_settings.setValue("Language/NativeTag", QString::fromStdString(value));
}

std::string settings_store_t::foreign_tag() const
{
	return m_settings.value("Language/ForeignTag", "").toString().toStdString();
}

void settings_store_t::set_foreign_tag(const std::string & value)
{
	m_settings.setValue("Language/ForeignTag", QString::fromStdString(value));
}

std::string settings_store_t::web_api_key(const std::string & provider_id) const
{
	const auto key = QString("WebTranslators/") + QString::fromStdString(provider_id);
	return m_settings.value(key, "").toString().toStdString();
}

void settings_store_t::set_web_api_key(const std::string & provider_id, const std::string & value)
{
	const auto key = QString("WebTranslators/") + QString::fromStdString(provider_id);
	m_settings.setValue(key, QString::fromStdString(value));
}

std::string settings_store_t::web_provider_setting(const std::string & provider_id, const std::string & key) const
{
	const auto ini_key =
	    QString("WebTranslators/") + QString::fromStdString(provider_id) + "/" + QString::fromStdString(key);
	auto result = m_settings.value(ini_key, "").toString().toStdString();

	if (result.empty() && key == "api_key")
		result = web_api_key(provider_id);

	return result;
}

void settings_store_t::set_web_provider_setting(
    const std::string & provider_id,
    const std::string & key,
    const std::string & value)
{
	const auto ini_key =
	    QString("WebTranslators/") + QString::fromStdString(provider_id) + "/" + QString::fromStdString(key);
	m_settings.setValue(ini_key, QString::fromStdString(value));

	if (key == "api_key")
		set_web_api_key(provider_id, value);
}

int settings_store_t::translation_source_index() const
{
	return m_settings.value("Translation/SourceIndex", 0).toInt();
}

void settings_store_t::set_translation_source_index(int index)
{
	m_settings.setValue("Translation/SourceIndex", index);
}

int settings_store_t::translation_language_index() const
{
	return m_settings.value("Translation/LanguageIndex", 0).toInt();
}

void settings_store_t::set_translation_language_index(int index)
{
	m_settings.setValue("Translation/LanguageIndex", index);
}

std::string settings_store_t::shortcut(const std::string & action_name) const
{
	const auto key = QString("Shortcuts/") + QString::fromStdString(action_name);
	return m_settings.value(key, "").toString().toStdString();
}

void settings_store_t::set_shortcut(const std::string & action_name, const std::string & key_sequence)
{
	const auto key = QString("Shortcuts/") + QString::fromStdString(action_name);
	m_settings.setValue(key, QString::fromStdString(key_sequence));
}

std::vector<std::string> settings_store_t::workspace_roots() const
{
	const int count = m_settings.value("WorkspaceRoots/Count", 0).toInt();
	std::vector<std::string> roots;
	roots.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		const auto key = QString("WorkspaceRoots/Path%1").arg(i);
		roots.push_back(m_settings.value(key, "").toString().toStdString());
	}
	return roots;
}

void settings_store_t::set_workspace_roots(const std::vector<std::string> & roots)
{
	m_settings.setValue("WorkspaceRoots/Count", static_cast<int>(roots.size()));
	for (int i = 0; i < static_cast<int>(roots.size()); ++i)
	{
		const auto key = QString("WorkspaceRoots/Path%1").arg(i);
		m_settings.setValue(key, QString::fromStdString(roots[i]));
	}
}

std::vector<std::string> settings_store_t::last_merge_order() const
{
	const int count = m_settings.value("MergeOrder/Count", 0).toInt();
	std::vector<std::string> paths;
	paths.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		const auto key = QString("MergeOrder/Path%1").arg(i);
		paths.push_back(m_settings.value(key, "").toString().toStdString());
	}
	return paths;
}

void settings_store_t::set_last_merge_order(const std::vector<std::string> & paths)
{
	m_settings.setValue("MergeOrder/Count", static_cast<int>(paths.size()));
	for (int i = 0; i < static_cast<int>(paths.size()); ++i)
	{
		const auto key = QString("MergeOrder/Path%1").arg(i);
		m_settings.setValue(key, QString::fromStdString(paths[i]));
	}
}

std::string settings_store_t::active_dict_path() const
{
	return m_settings.value("Editor/ActiveDictPath", "").toString().toStdString();
}

void settings_store_t::set_active_dict_path(const std::string & value)
{
	m_settings.setValue("Editor/ActiveDictPath", QString::fromStdString(value));
}

std::string settings_store_t::last_directory() const
{
	return m_settings.value("Paths/LastDirectory", "").toString().toStdString();
}

void settings_store_t::set_last_directory(const std::string & value)
{
	m_settings.setValue("Paths/LastDirectory", QString::fromStdString(value));
}

std::string settings_store_t::openmw_data_dir() const
{
	return m_settings.value("Paths/OpenMwDataDir", "").toString().toStdString();
}

void settings_store_t::set_openmw_data_dir(const std::string & value)
{
	m_settings.setValue("Paths/OpenMwDataDir", QString::fromStdString(value));
}

std::string settings_store_t::mo2_profile_dir() const
{
	return m_settings.value("Paths/Mo2ProfileDir", "").toString().toStdString();
}

void settings_store_t::set_mo2_profile_dir(const std::string & value)
{
	m_settings.setValue("Paths/Mo2ProfileDir", QString::fromStdString(value));
}

std::string settings_store_t::output_dir_folder() const
{
	return m_settings.value("Paths/OutputDirFolder", "").toString().toStdString();
}

void settings_store_t::set_output_dir_folder(const std::string & value)
{
	m_settings.setValue("Paths/OutputDirFolder", QString::fromStdString(value));
}

std::string settings_store_t::output_dir_mo2() const
{
	return m_settings.value("Paths/OutputDirMo2", "../../overwrite").toString().toStdString();
}

void settings_store_t::set_output_dir_mo2(const std::string & value)
{
	m_settings.setValue("Paths/OutputDirMo2", QString::fromStdString(value));
}

std::string settings_store_t::output_dir_openmw() const
{
	return m_settings.value("Paths/OutputDirOpenmw", "data").toString().toStdString();
}

void settings_store_t::set_output_dir_openmw(const std::string & value)
{
	m_settings.setValue("Paths/OutputDirOpenmw", QString::fromStdString(value));
}

float settings_store_t::split_ratio() const
{
	return m_settings.value("Editor/SplitRatio", 0.5f).toFloat();
}

void settings_store_t::set_split_ratio(float value)
{
	m_settings.setValue("Editor/SplitRatio", static_cast<double>(value));
}

int settings_store_t::sidebar_width() const
{
	return m_settings.value("Editor/SidebarWidth", 250).toInt();
}

void settings_store_t::set_sidebar_width(int value)
{
	m_settings.setValue("Editor/SidebarWidth", value);
}

int settings_store_t::bottom_height() const
{
	return m_settings.value("Editor/BottomHeight", 0).toInt();
}

void settings_store_t::set_bottom_height(int value)
{
	m_settings.setValue("Editor/BottomHeight", value);
}

int settings_store_t::info_height() const
{
	return m_settings.value("Editor/InfoHeight", 0).toInt();
}

void settings_store_t::set_info_height(int value)
{
	m_settings.setValue("Editor/InfoHeight", value);
}

bool settings_store_t::sidebar_visible() const
{
	return m_settings.value("Editor/SidebarVisible", true).toBool();
}

void settings_store_t::set_sidebar_visible(bool value)
{
	m_settings.setValue("Editor/SidebarVisible", value);
}

bool settings_store_t::bottom_visible() const
{
	return m_settings.value("Editor/BottomVisible", true).toBool();
}

void settings_store_t::set_bottom_visible(bool value)
{
	m_settings.setValue("Editor/BottomVisible", value);
}

int settings_store_t::column_width(int index) const
{
	const auto key = QString("Editor/Column%1").arg(index);
	return m_settings.value(key, 100).toInt();
}

void settings_store_t::set_column_width(int index, int value)
{
	const auto key = QString("Editor/Column%1").arg(index);
	m_settings.setValue(key, value);
}

int settings_store_t::window_x() const
{
	return m_settings.value("Window/X", 100).toInt();
}

void settings_store_t::set_window_x(int value)
{
	m_settings.setValue("Window/X", value);
}

int settings_store_t::window_y() const
{
	return m_settings.value("Window/Y", 100).toInt();
}

void settings_store_t::set_window_y(int value)
{
	m_settings.setValue("Window/Y", value);
}

int settings_store_t::window_width() const
{
	return m_settings.value("Window/W", 1200).toInt();
}

void settings_store_t::set_window_width(int value)
{
	m_settings.setValue("Window/W", value);
}

int settings_store_t::window_height() const
{
	return m_settings.value("Window/H", 800).toInt();
}

void settings_store_t::set_window_height(int value)
{
	m_settings.setValue("Window/H", value);
}

bool settings_store_t::window_maximized() const
{
	return m_settings.value("Window/Maximized", false).toBool();
}

void settings_store_t::set_window_maximized(bool value)
{
	m_settings.setValue("Window/Maximized", value);
}

theme_t settings_store_t::theme() const
{
	const auto text = m_settings.value("Appearance/Theme", "light").toString().toStdString();
	if (text == "dark")
		return theme_t::dark;

	return theme_t::light;
}

void settings_store_t::set_theme(theme_t value)
{
	const auto text = (value == theme_t::dark) ? "dark" : "light";
	m_settings.setValue("Appearance/Theme", text);
}

std::string settings_store_t::merge_exclusion_pattern() const
{
	return m_settings.value("merge/exclusion_pattern", "").toString().toStdString();
}

void settings_store_t::set_merge_exclusion_pattern(const std::string & pattern)
{
	m_settings.setValue("merge/exclusion_pattern", QString::fromStdString(pattern));
}

bool settings_store_t::merge_fog_fix_enabled() const
{
	return m_settings.value("merge/fog_fix", true).toBool();
}

void settings_store_t::set_merge_fog_fix_enabled(bool value)
{
	m_settings.setValue("merge/fog_fix", value);
}

bool settings_store_t::merge_summon_fix_enabled() const
{
	return m_settings.value("merge/summon_fix", true).toBool();
}

void settings_store_t::set_merge_summon_fix_enabled(bool value)
{
	m_settings.setValue("merge/summon_fix", value);
}

bool settings_store_t::merge_cell_name_fix_enabled() const
{
	return m_settings.value("merge/cell_name_fix", true).toBool();
}

void settings_store_t::set_merge_cell_name_fix_enabled(bool value)
{
	m_settings.setValue("merge/cell_name_fix", value);
}

std::string settings_store_t::sub_record_ignore_conflict() const
{
	return m_settings.value("SubRecordRules/IgnoreConflict", "CELL:NAM0").toString().toStdString();
}

void settings_store_t::set_sub_record_ignore_conflict(const std::string & value)
{
	m_settings.setValue("SubRecordRules/IgnoreConflict", QString::fromStdString(value));
}

int settings_store_t::display_codepage() const
{
	return m_settings.value("Editor/DisplayCodepage", 1252).toInt();
}

void settings_store_t::set_display_codepage(int value)
{
	m_settings.setValue("Editor/DisplayCodepage", value);
}

bool settings_store_t::clean_evil_gmst_enabled() const
{
	return m_settings.value("Cleaning/EvilGmst", true).toBool();
}

void settings_store_t::set_clean_evil_gmst_enabled(bool value)
{
	m_settings.setValue("Cleaning/EvilGmst", value);
}

bool settings_store_t::clean_junk_cell_enabled() const
{
	return m_settings.value("Cleaning/JunkCell", true).toBool();
}

void settings_store_t::set_clean_junk_cell_enabled(bool value)
{
	m_settings.setValue("Cleaning/JunkCell", value);
}

bool settings_store_t::clean_update_master_sizes() const
{
	return m_settings.value("Cleaning/UpdateMasterSizes", false).toBool();
}

void settings_store_t::set_clean_update_master_sizes(bool value)
{
	m_settings.setValue("Cleaning/UpdateMasterSizes", value);
}

bool settings_store_t::clean_update_version() const
{
	return m_settings.value("Cleaning/UpdateVersion", false).toBool();
}

void settings_store_t::set_clean_update_version(bool value)
{
	m_settings.setValue("Cleaning/UpdateVersion", value);
}

void settings_store_t::sync()
{
	m_settings.sync();
}
