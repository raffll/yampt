#pragma once

#include <scanner/lua_scanner.hpp>
#include <string>
#include <vector>
#include <QThread>

Q_DECLARE_METATYPE(lua_scan_result_t)

class lua_scan_worker_t : public QThread
{
	Q_OBJECT

public:
	explicit lua_scan_worker_t(QObject * parent = nullptr);

	void start_scan(const std::vector<std::string> & data_paths, const std::vector<std::string> & mod_names);
	void cancel_scan();

signals:
	void scan_complete(lua_scan_result_t result);
	void scan_progress(int files_scanned, int total_files);

protected:
	void run() override;

private:
	lua_scanner_t m_scanner;
	std::vector<std::string> m_data_paths;
	std::vector<std::string> m_mod_names;
};
