#pragma once

#include <scanner/plugin_scan.hpp>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <QObject>

class patch_builder_t;

class plugin_session_t : public QObject
{
	Q_OBJECT

public:
	explicit plugin_session_t(QObject * parent = nullptr);
	~plugin_session_t() override;

	enum class load_source_t
	{
		none,
		folder,
		openmw_cfg,
		mo2_profile
	};

	void load_from_folder(const std::vector<std::string> & paths, const std::string & base_path);
	void load_from_mo2_profile(const QString & profile_dir);
	void load_from_openmw_cfg(const QString & cfg_path);
	void unload_all();

	plugin_scan_t & scan();
	const plugin_scan_t & scan() const;
	patch_builder_t & patch_builder();

	void save_session_state(const QString & ini_path);
	void restore_session_state(const QString & ini_path);

	const std::set<std::string> & excluded_plugins() const;
	void set_excluded_plugins(const std::set<std::string> & excluded);
	const std::set<std::string> & patch_plugins() const;
	void set_patch_plugins(const std::set<std::string> & patch);

	load_source_t load_source() const;
	const std::string & load_base_path() const;

signals:
	void plugins_loaded();
	void plugins_unloaded();
	void log_message(const std::string & message);

private:
	struct mo2_resolve_context_t
	{
		std::vector<std::string> enabled_mods;
		QString mods_path;
		QString game_data_path;
		QString overwrite_path;
	};

	std::vector<std::string> parse_mo2_profile(const QString & profile_dir);
	std::vector<std::string> parse_openmw_cfg(const QString & cfg_path);
	std::vector<std::string> resolve_mo2_plugins(
	    const std::vector<std::string> & plugin_names,
	    const mo2_resolve_context_t & context);
	std::string resolve_single_mo2_plugin(const std::string & plugin_name, const mo2_resolve_context_t & context);
	std::vector<std::string> resolve_openmw_content(
	    const std::vector<std::string> & content_names,
	    const std::vector<std::string> & data_dirs);
	std::string resolve_single_content(const std::string & content_name, const std::vector<std::string> & data_dirs);
	void load_plugins_internal(const std::vector<std::string> & paths);

	plugin_scan_t m_scan;
	std::unique_ptr<patch_builder_t> m_patch_builder;
	std::set<std::string> m_excluded_plugins;
	std::set<std::string> m_patch_plugins;
	load_source_t m_load_source = load_source_t::none;
	std::string m_load_base_path;
};
