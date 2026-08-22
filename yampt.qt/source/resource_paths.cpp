#include "resource_paths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

static QString user_data_dir()
{
	return QDir::homePath() + "/.yampt";
}

static QString system_data_dir()
{
#ifdef YAMPT_DATA_DIR
	return QStringLiteral(YAMPT_DATA_DIR);
#else
	return QCoreApplication::applicationDirPath();
#endif
}

static std::string resolve_file(const QString & relative_path)
{
	const auto user_path = user_data_dir() + "/" + relative_path;
	if (QFileInfo::exists(user_path))
		return user_path.toStdString();

	const auto system_path = system_data_dir() + "/" + relative_path;
	if (QFileInfo::exists(system_path))
		return system_path.toStdString();

	return system_path.toStdString();
}

static std::string resolve_directory(const QString & relative_path)
{
	const auto user_path = user_data_dir() + "/" + relative_path;
	if (QFileInfo(user_path).isDir())
		return user_path.toStdString() + "/";

	const auto system_path = system_data_dir() + "/" + relative_path;
	return system_path.toStdString() + "/";
}

std::string resource_paths::data_dir()
{
	static const std::string cached = system_data_dir().toStdString();
	return cached;
}

std::string resource_paths::user_dir()
{
	static const std::string cached = user_data_dir().toStdString();
	return cached;
}

std::string resource_paths::languages_file()
{
	return resolve_file("languages.json");
}

std::string resource_paths::providers_dir()
{
	return resolve_directory("providers");
}

std::string resource_paths::dictionaries_dir()
{
	return resolve_directory("dictionaries");
}
