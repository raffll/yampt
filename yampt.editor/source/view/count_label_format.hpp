#pragma once

#include <QString>

namespace count_label_format {

inline QString format(size_t plugin_count, size_t record_count, size_t conflict_count, size_t lua_conflict_count)
{
	auto label = QString("%1 plugins, %2 records, %3 conflicts")
	                 .arg(plugin_count)
	                 .arg(record_count)
	                 .arg(conflict_count);

	if (lua_conflict_count > 0)
		label += QString(", %1 Lua conflicts").arg(lua_conflict_count);

	return label;
}

} // namespace count_label_format
