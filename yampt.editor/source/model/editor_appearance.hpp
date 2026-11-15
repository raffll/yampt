#pragma once

#include <scanner/plugin_scan.hpp>
#include <scanner/tree_source.hpp>
#include <conflict_types.hpp>
#include <set>
#include <string>
#include <QBrush>
#include <QString>

class editor_appearance_t
{
public:
	void set_scan(plugin_scan_t * scan);
	void set_patch_plugins(const std::set<std::string> * patch);

	QString plugin_prefix(int plugin_idx) const;

	QBrush foreground_brush(conflict_this_t cls) const;
	QBrush background_brush(conflict_all_t cls) const;
	QBrush locked_background() const;
	QBrush locked_text() const;

private:
	plugin_scan_t * m_scan = nullptr;
	const std::set<std::string> * m_patch_plugins = nullptr;
};
