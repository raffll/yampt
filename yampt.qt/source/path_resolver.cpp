#include "path_resolver.hpp"

#include <QDir>
#include <QFileInfo>

path_resolver_t::path_resolver_t(const search_config_t & config)
	: m_config(config)
{
}

std::string path_resolver_t::resolve_workspace_path(const std::string & relative_path) const
{
	for (const auto & root : m_config.workspace_roots)
	{
		const auto full_path = QDir(QString::fromStdString(root)).filePath(QString::fromStdString(relative_path));
		if (QFileInfo::exists(full_path))
			return full_path.toStdString();
	}

	return {};
}

std::string path_resolver_t::resolve_mo2_mods_dir() const
{
	if (m_config.mo2_profile_dir.empty())
		return {};

	QDir profile_dir(QString::fromStdString(m_config.mo2_profile_dir));
	if (!profile_dir.cdUp() || !profile_dir.cdUp())
		return {};

	const auto mods_path = profile_dir.filePath("mods");
	if (QFileInfo(mods_path).isDir())
		return mods_path.toStdString();

	return {};
}

std::string path_resolver_t::resolve_mo2_overwrite_dir() const
{
	if (m_config.mo2_profile_dir.empty())
		return {};

	QDir profile_dir(QString::fromStdString(m_config.mo2_profile_dir));
	if (!profile_dir.cdUp() || !profile_dir.cdUp())
		return {};

	const auto overwrite_path = profile_dir.filePath("overwrite");
	if (QFileInfo(overwrite_path).isDir())
		return overwrite_path.toStdString();

	return {};
}

std::string path_resolver_t::resolve_openmw_data_dir() const
{
	return m_config.openmw_data_dir;
}

std::string path_resolver_t::resolve_game_data_path() const
{
	if (!m_config.last_directory.empty())
	{
		const auto data_files = QDir(QString::fromStdString(m_config.last_directory)).filePath("Data Files");
		if (QFileInfo(data_files).isDir())
			return data_files.toStdString();
	}

	const QStringList standard_paths = {
		"C:/Program Files (x86)/Steam/steamapps/common/Morrowind/Data Files",
		"C:/Program Files/Steam/steamapps/common/Morrowind/Data Files",
		"C:/GOG Games/Morrowind/Data Files"
	};

	for (const auto & path : standard_paths)
	{
		if (QFileInfo(path).isDir())
			return path.toStdString();
	}

	return {};
}

const std::vector<std::string> & path_resolver_t::workspace_roots() const
{
	return m_config.workspace_roots;
}
