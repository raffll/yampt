#include "plugin_session.hpp"
#include "../patcher/patch_builder.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

plugin_session_t::plugin_session_t(QObject * parent)
    : QObject(parent)
    , m_patch_builder(std::make_unique<patch_builder_t>())
{}

plugin_session_t::~plugin_session_t() = default;

void plugin_session_t::load_from_folder(const std::vector<std::string> & paths, const std::string & base_path)
{
	m_load_base_path = base_path;
	m_load_source = load_source_t::folder;
	load_plugins_internal(paths);
}

void plugin_session_t::load_from_mo2_profile(const QString & profile_dir)
{
	auto paths = parse_mo2_profile(profile_dir);
	if (paths.empty())
		return;

	m_load_base_path = profile_dir.toStdString();
	m_load_source = load_source_t::mo2_profile;
	load_plugins_internal(paths);
}

void plugin_session_t::load_from_openmw_cfg(const QString & cfg_path)
{
	auto paths = parse_openmw_cfg(cfg_path);
	if (paths.empty())
		return;

	const auto cfg_dir = QFileInfo(cfg_path).absolutePath();
	m_load_base_path = cfg_dir.toStdString();
	m_load_source = load_source_t::openmw_cfg;
	load_plugins_internal(paths);
}

void plugin_session_t::unload_all()
{
	m_scan = plugin_scan_t();
	m_patch_builder->clear();
	m_load_source = load_source_t::none;
	m_load_base_path.clear();
	emit plugins_unloaded();
}

plugin_scan_t & plugin_session_t::scan()
{
	return m_scan;
}

const plugin_scan_t & plugin_session_t::scan() const
{
	return m_scan;
}

patch_builder_t & plugin_session_t::patch_builder()
{
	return *m_patch_builder;
}

const std::set<std::string> & plugin_session_t::excluded_plugins() const
{
	return m_excluded_plugins;
}

void plugin_session_t::set_excluded_plugins(const std::set<std::string> & excluded)
{
	m_excluded_plugins = excluded;
}

const std::set<std::string> & plugin_session_t::patch_plugins() const
{
	return m_patch_plugins;
}

void plugin_session_t::set_patch_plugins(const std::set<std::string> & patch)
{
	m_patch_plugins = patch;
}

plugin_session_t::load_source_t plugin_session_t::load_source() const
{
	return m_load_source;
}

const std::string & plugin_session_t::load_base_path() const
{
	return m_load_base_path;
}

void plugin_session_t::save_session_state(const QString & ini_path)
{
	QSettings settings(ini_path, QSettings::IniFormat);

	settings.setValue("session/load_source", static_cast<int>(m_load_source));
	settings.setValue("session/load_base_path", QString::fromStdString(m_load_base_path));

	QStringList excluded_list;
	for (const auto & name : m_excluded_plugins)
		excluded_list.append(QString::fromStdString(name));

	settings.setValue("merge/excluded_plugins", excluded_list);

	QStringList patch_list;
	for (const auto & name : m_patch_plugins)
		patch_list.append(QString::fromStdString(name));

	settings.setValue("merge/patch_plugins", patch_list);
}

void plugin_session_t::restore_session_state(const QString & ini_path)
{
	QSettings settings(ini_path, QSettings::IniFormat);

	m_load_source = static_cast<load_source_t>(settings.value("session/load_source", 0).toInt());
	m_load_base_path = settings.value("session/load_base_path").toString().toStdString();

	const auto excluded_list = settings.value("merge/excluded_plugins").toStringList();
	m_excluded_plugins.clear();
	for (const auto & name : excluded_list)
		m_excluded_plugins.insert(name.toStdString());

	const auto patch_list = settings.value("merge/patch_plugins").toStringList();
	m_patch_plugins.clear();
	for (const auto & name : patch_list)
		m_patch_plugins.insert(name.toStdString());

	if (m_load_base_path.empty())
		return;

	switch (m_load_source)
	{
	case load_source_t::folder:
	{
		QDir data_dir(QString::fromStdString(m_load_base_path));
		auto file_list = data_dir.entryInfoList({ "*.esm", "*.esp" }, QDir::Files, QDir::Time | QDir::Reversed);

		std::vector<std::string> esms;
		std::vector<std::string> esps;
		for (const auto & file_info : file_list)
		{
			if (file_info.suffix().toLower() == "esm")
				esms.push_back(file_info.absoluteFilePath().toStdString());
			else
				esps.push_back(file_info.absoluteFilePath().toStdString());
		}

		std::vector<std::string> paths;
		paths.insert(paths.end(), esms.begin(), esms.end());
		paths.insert(paths.end(), esps.begin(), esps.end());

		if (!paths.empty())
			load_plugins_internal(paths);

		break;
	}
	case load_source_t::mo2_profile:
	{
		auto paths = parse_mo2_profile(QString::fromStdString(m_load_base_path));
		if (!paths.empty())
			load_plugins_internal(paths);

		break;
	}
	case load_source_t::openmw_cfg:
	{
		const auto cfg_path = QString::fromStdString(m_load_base_path) + "/openmw.cfg";
		auto paths = parse_openmw_cfg(cfg_path);
		if (!paths.empty())
			load_plugins_internal(paths);

		break;
	}
	case load_source_t::none:
		break;
	}
}

void plugin_session_t::load_plugins_internal(const std::vector<std::string> & paths)
{
	m_scan = plugin_scan_t();
	m_patch_builder->clear();

	for (const auto & path : paths)
	{
		if (!QFile::exists(QString::fromStdString(path)))
		{
			emit log_message("[warning] skipping missing plugin: " + path);
			continue;
		}

		auto filename = path;
		auto pos = filename.find_last_of("/\\");
		if (pos != std::string::npos)
			filename = filename.substr(pos + 1);

		try
		{
			m_scan.load_plugin(path);
			const int loaded_idx = static_cast<int>(m_scan.plugin_count()) - 1;

			if (filename == "Merged Patch.esp")
			{
				m_scan.set_merge_plugin_from_loaded(loaded_idx);
				emit log_message("Loaded merge plugin: " + filename);
			}
			else
			{
				const auto & idx = m_scan.index(loaded_idx);
				emit log_message(
				    "Loaded " + m_scan.plugin_filename(loaded_idx) + " (" +
				    std::to_string(idx.entries().size()) + " records indexed)");
			}
		}
		catch (const std::exception & exception)
		{
			emit log_message("[error] loading " + filename + ": " + exception.what());
		}
	}

	if (m_scan.plugin_count() == 0)
		return;

	m_scan.rebuild_conflicts();
	emit plugins_loaded();
}

std::vector<std::string> plugin_session_t::parse_mo2_profile(const QString & profile_dir)
{
	QString loadorder_path = profile_dir + "/loadorder.txt";
	QFile loadorder_file(loadorder_path);
	if (!loadorder_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		emit log_message("Cannot open loadorder.txt in " + profile_dir.toStdString());
		return {};
	}

	std::vector<std::string> plugin_names;
	QTextStream stream(&loadorder_file);
	while (!stream.atEnd())
	{
		auto line = stream.readLine().trimmed();
		if (line.isEmpty() || line.startsWith('#'))
			continue;

		if (line.startsWith('*'))
			line = line.mid(1);

		plugin_names.push_back(line.toStdString());
	}
	loadorder_file.close();

	QDir profile(profile_dir);
	QDir mo2_root = profile;
	mo2_root.cdUp();
	mo2_root.cdUp();

	const auto mods_path = mo2_root.absolutePath() + "/mods";

	QString game_data_path;
	QSettings mo2_ini(mo2_root.absolutePath() + "/ModOrganizer.ini", QSettings::IniFormat);
	game_data_path = mo2_ini.value("General/gamePath").toString();
	if (game_data_path.isEmpty())
		game_data_path = mo2_ini.value("Settings/game_path").toString();

	if (game_data_path.isEmpty())
	{
		QFile ini_file(mo2_root.absolutePath() + "/ModOrganizer.ini");
		if (ini_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream ini_stream(&ini_file);
			while (!ini_stream.atEnd())
			{
				auto ini_line = ini_stream.readLine().trimmed();
				if (ini_line.startsWith("gamePath=") || ini_line.startsWith("gamePath ="))
				{
					game_data_path = ini_line.mid(ini_line.indexOf('=') + 1).trimmed();
					if (game_data_path.startsWith("@ByteArray(") && game_data_path.endsWith(")"))
						game_data_path = game_data_path.mid(11, game_data_path.size() - 12);

					break;
				}
			}
			ini_file.close();
		}
	}

	if (!game_data_path.isEmpty())
	{
		game_data_path.replace('\\', '/');
		if (!game_data_path.endsWith("/Data Files"))
			game_data_path += "/Data Files";
	}

	mo2_resolve_context_t context;

	QString modlist_path = profile_dir + "/modlist.txt";
	QFile modlist_file(modlist_path);
	if (modlist_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream modlist_stream(&modlist_file);
		while (!modlist_stream.atEnd())
		{
			auto mod_line = modlist_stream.readLine().trimmed();
			if (mod_line.startsWith('+'))
				context.enabled_mods.push_back(mod_line.mid(1).toStdString());
		}
		modlist_file.close();
		std::reverse(context.enabled_mods.begin(), context.enabled_mods.end());
	}

	context.mods_path = mods_path;
	context.game_data_path = game_data_path;
	context.overwrite_path = mo2_root.absolutePath() + "/overwrite";

	auto paths = resolve_mo2_plugins(plugin_names, context);

	static const std::vector<std::string> master_files = { "Morrowind.esm", "Tribunal.esm", "Bloodmoon.esm" };

	for (const auto & master : master_files)
	{
		bool found = false;
		for (const auto & resolved : paths)
		{
			auto pos = resolved.find_last_of("/\\");
			auto filename = (pos != std::string::npos) ? resolved.substr(pos + 1) : resolved;
			if (QString::fromStdString(filename).compare(QString::fromStdString(master), Qt::CaseInsensitive) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
			emit log_message(
			    "[warning] master file not found in load order: " + master +
			    " (searched in " + context.game_data_path.toStdString() + ")");
	}

	const auto merge_full_path = context.overwrite_path + "/Merged Patch.esp";
	if (QFile::exists(merge_full_path))
	{
		const auto merge_std = merge_full_path.toStdString();
		bool already_included = false;
		for (const auto & resolved : paths)
		{
			if (resolved == merge_std)
			{
				already_included = true;
				break;
			}
		}

		if (!already_included)
			paths.push_back(merge_std);
	}

	return paths;
}

std::vector<std::string> plugin_session_t::resolve_mo2_plugins(
    const std::vector<std::string> & plugin_names,
    const mo2_resolve_context_t & context)
{
	std::vector<std::string> paths;
	for (const auto & name : plugin_names)
	{
		const auto & resolved = resolve_single_mo2_plugin(name, context);
		if (!resolved.empty())
			paths.push_back(resolved);
		else
			emit log_message("Cannot find: " + name);
	}

	if (paths.empty())
		emit log_message("No plugins resolved from MO2 profile");

	return paths;
}

std::string plugin_session_t::resolve_single_mo2_plugin(
    const std::string & plugin_name,
    const mo2_resolve_context_t & context)
{
	if (!context.overwrite_path.isEmpty())
	{
		const auto overwrite_candidate = context.overwrite_path + "/" + QString::fromStdString(plugin_name);
		if (QFile::exists(overwrite_candidate))
			return overwrite_candidate.toStdString();
	}

	for (const auto & mod_name : context.enabled_mods)
	{
		const auto candidate =
		    context.mods_path + "/" + QString::fromStdString(mod_name) + "/" + QString::fromStdString(plugin_name);
		if (QFile::exists(candidate))
			return candidate.toStdString();
	}

	const auto game_file = context.game_data_path + "/" + QString::fromStdString(plugin_name);
	if (QFile::exists(game_file))
		return game_file.toStdString();

	return {};
}

std::vector<std::string> plugin_session_t::parse_openmw_cfg(const QString & cfg_path)
{
	QFile cfg_file(cfg_path);
	if (!cfg_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		emit log_message("Cannot open " + cfg_path.toStdString());
		return {};
	}

	std::vector<std::string> data_dirs;
	std::vector<std::string> content_names;

	QTextStream stream(&cfg_file);
	while (!stream.atEnd())
	{
		auto line = stream.readLine().trimmed();

		if (line.startsWith("data=") || line.startsWith("data ="))
		{
			auto value = line.mid(line.indexOf('=') + 1).trimmed();
			if (value.startsWith('"') && value.endsWith('"'))
				value = value.mid(1, value.size() - 2);

			data_dirs.push_back(value.toStdString());
		}
		else if (line.startsWith("content=") || line.startsWith("content ="))
		{
			auto value = line.mid(line.indexOf('=') + 1).trimmed();
			content_names.push_back(value.toStdString());
		}
	}
	cfg_file.close();

	return resolve_openmw_content(content_names, data_dirs);
}

std::vector<std::string> plugin_session_t::resolve_openmw_content(
    const std::vector<std::string> & content_names,
    const std::vector<std::string> & data_dirs)
{
	std::vector<std::string> paths;
	for (const auto & name : content_names)
	{
		const auto & resolved = resolve_single_content(name, data_dirs);
		if (!resolved.empty())
			paths.push_back(resolved);
		else
			emit log_message("Cannot find: " + name);
	}

	if (paths.empty())
		emit log_message("No plugins resolved from openmw.cfg");

	return paths;
}

std::string plugin_session_t::resolve_single_content(
    const std::string & content_name,
    const std::vector<std::string> & data_dirs)
{
	for (auto it_dir = data_dirs.rbegin(); it_dir != data_dirs.rend(); ++it_dir)
	{
		const auto candidate = QString::fromStdString(*it_dir) + "/" + QString::fromStdString(content_name);
		if (QFile::exists(candidate))
			return candidate.toStdString();
	}

	return {};
}
