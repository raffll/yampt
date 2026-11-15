#pragma once

#include <settings_store.hpp>
#include <QDir>
#include <QFile>
#include <QString>

namespace gui_test_support {

inline void remove_isolated_store(const QString & unique_name)
{
	const auto full_path = QDir(settings_store_t::settings_dir()).filePath(unique_name);
	QFile::remove(full_path);
}

} // namespace gui_test_support
