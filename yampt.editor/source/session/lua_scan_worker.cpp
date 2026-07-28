#include "lua_scan_worker.hpp"

lua_scan_worker_t::lua_scan_worker_t(QObject * parent)
    : QThread(parent)
{
	qRegisterMetaType<lua_scan_result_t>();
}

void lua_scan_worker_t::start_scan(const std::vector<std::string> & data_paths,
                                   const std::vector<std::string> & mod_names)
{
	m_data_paths = data_paths;
	m_mod_names = mod_names;
	start();
}

void lua_scan_worker_t::cancel_scan()
{
	m_scanner.cancel();
}

void lua_scan_worker_t::run()
{
	const auto result = m_scanner.scan(m_data_paths, m_mod_names);
	emit scan_complete(result);
}
