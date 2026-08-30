#include "resource_paths.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

static QString exe_dir()
{
	return QCoreApplication::applicationDirPath();
}

static QString user_dir()
{
#ifdef _WIN32
	return exe_dir();
#else
	return QDir::homePath() + "/.yampt";
#endif
}

static QString system_dir()
{
#ifdef YAMPT_DATA_DIR
	return QStringLiteral(YAMPT_DATA_DIR);
#else
	return exe_dir();
#endif
}

static std::string resolve_shared_file(const QString & relative_path)
{
	const auto from_user = user_dir() + "/" + relative_path;
	if (QFileInfo::exists(from_user))
		return from_user.toStdString();

	const auto from_system = system_dir() + "/" + relative_path;
	if (QFileInfo::exists(from_system))
		return from_system.toStdString();

	const auto from_exe = exe_dir() + "/" + relative_path;
	if (QFileInfo::exists(from_exe))
		return from_exe.toStdString();

	return from_system.toStdString();
}

static std::string resolve_shared_directory(const QString & relative_path)
{
	const auto from_user = user_dir() + "/" + relative_path;
	if (QFileInfo(from_user).isDir())
		return from_user.toStdString() + "/";

	const auto from_system = system_dir() + "/" + relative_path;
	if (QFileInfo(from_system).isDir())
		return from_system.toStdString() + "/";

	const auto from_exe = exe_dir() + "/" + relative_path;
	if (QFileInfo(from_exe).isDir())
		return from_exe.toStdString() + "/";

	return from_system.toStdString() + "/";
}

static std::string resolve_user_directory(const QString & relative_path)
{
	const auto path = user_dir() + "/" + relative_path;
	QDir().mkpath(path);
	return path.toStdString() + "/";
}

std::string resource_paths::languages_file()
{
	return resolve_shared_file("languages.json");
}

std::string resource_paths::providers_dir()
{
	return resolve_shared_directory("providers");
}

std::string resource_paths::dictionaries_dir()
{
	return resolve_shared_directory("dictionaries");
}

std::string resource_paths::translations_dir()
{
	return resolve_shared_directory("translations");
}

std::string resource_paths::config_dir()
{
	return resolve_user_directory("");
}

std::string resource_paths::workspace_dir()
{
	return resolve_user_directory("workspace");
}

std::string resource_paths::models_dir()
{
	return resolve_user_directory("models");
}
