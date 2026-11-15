#include "editor_appearance.hpp"
#include "../view/plugin_icon.hpp"
#include <theme_system.hpp>

void editor_appearance_t::set_scan(plugin_scan_t * scan)
{
	m_scan = scan;
}

void editor_appearance_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_patch_plugins = patch;
}

QString editor_appearance_t::plugin_prefix(int plugin_idx) const
{
	if (m_scan == nullptr)
		return {};

	plugin_meta_t meta;
	meta.filename = m_scan->plugin_filename(plugin_idx);
	meta.path = m_scan->plugin_path(plugin_idx);
	meta.is_guard = m_patch_plugins != nullptr && m_patch_plugins->count(meta.filename) != 0;
	meta.is_active = m_scan->is_active_plugin(plugin_idx);

	return plugin_icon::prefix(tree_source_t::icon_flags(meta));
}

QBrush editor_appearance_t::foreground_brush(conflict_this_t cls) const
{
	return QBrush(theme_system_t::instance().conflict_this_foreground(cls));
}

QBrush editor_appearance_t::background_brush(conflict_all_t cls) const
{
	return QBrush(theme_system_t::instance().conflict_all_background(cls));
}

QBrush editor_appearance_t::locked_background() const
{
	return QBrush(theme_system_t::instance().get_color(color_name_t::locked_background));
}

QBrush editor_appearance_t::locked_text() const
{
	return QBrush(theme_system_t::instance().get_color(color_name_t::locked_text));
}
