#include <catch2/catch_all.hpp>
#include <filesystem>
#include <path_resolver.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace {

std::string make_temp_dir(const std::string & suffix)
{
	namespace fs = std::filesystem;
	const auto base = fs::temp_directory_path() / ("yampt_test_" + suffix);
	fs::create_directories(base);
	return base.string();
}

void cleanup_temp_dir(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove_all(path, error_code);
}

} // namespace

TEST_CASE("path_resolver_t::resolve_workspace_path, finds file in first root", "[u][qt]")
{
	const auto root = make_temp_dir("ws_first");
	QDir(QString::fromStdString(root)).mkpath("sub");
	QFile file(QString::fromStdString(root + "/sub/test.txt"));
	file.open(QIODevice::WriteOnly);
	file.close();

	path_resolver_t::search_config_t config;
	config.workspace_roots = { root };
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_workspace_path("sub/test.txt");
	REQUIRE_FALSE(result.empty());
	REQUIRE(QFileInfo(QString::fromStdString(result)).exists());

	cleanup_temp_dir(root);
}

TEST_CASE("path_resolver_t::resolve_workspace_path, returns empty for missing file", "[u][qt]")
{
	const auto root = make_temp_dir("ws_missing");

	path_resolver_t::search_config_t config;
	config.workspace_roots = { root };
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_workspace_path("does_not_exist.txt");
	REQUIRE(result.empty());

	cleanup_temp_dir(root);
}

TEST_CASE("path_resolver_t::resolve_workspace_path, searches multiple roots in order", "[u][qt]")
{
	const auto root_first = make_temp_dir("ws_order_a");
	const auto root_second = make_temp_dir("ws_order_b");

	QFile file_b(QString::fromStdString(root_second + "/shared.txt"));
	file_b.open(QIODevice::WriteOnly);
	file_b.write("second");
	file_b.close();

	QFile file_a(QString::fromStdString(root_first + "/shared.txt"));
	file_a.open(QIODevice::WriteOnly);
	file_a.write("first");
	file_a.close();

	path_resolver_t::search_config_t config;
	config.workspace_roots = { root_first, root_second };
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_workspace_path("shared.txt");
	REQUIRE(result.find("ws_order_a") != std::string::npos);

	cleanup_temp_dir(root_first);
	cleanup_temp_dir(root_second);
}

TEST_CASE("path_resolver_t::resolve_mo2_mods_dir, navigates up from profile", "[u][qt]")
{
	const auto base = make_temp_dir("mo2_mods");
	const auto profile_dir = base + "/profiles/Default";
	const auto mods_dir = base + "/mods";
	std::filesystem::create_directories(profile_dir);
	std::filesystem::create_directories(mods_dir);

	path_resolver_t::search_config_t config;
	config.mo2_profile_dir = profile_dir;
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_mo2_mods_dir();
	REQUIRE_FALSE(result.empty());
	REQUIRE(QFileInfo(QString::fromStdString(result)).isDir());

	cleanup_temp_dir(base);
}

TEST_CASE("path_resolver_t::resolve_mo2_mods_dir, empty when no profile dir", "[u][qt]")
{
	path_resolver_t::search_config_t config;
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_mo2_mods_dir();
	REQUIRE(result.empty());
}

TEST_CASE("path_resolver_t::resolve_mo2_overwrite_dir, navigates up from profile", "[u][qt]")
{
	const auto base = make_temp_dir("mo2_overwrite");
	const auto profile_dir = base + "/profiles/Default";
	const auto overwrite_dir = base + "/overwrite";
	std::filesystem::create_directories(profile_dir);
	std::filesystem::create_directories(overwrite_dir);

	path_resolver_t::search_config_t config;
	config.mo2_profile_dir = profile_dir;
	path_resolver_t resolver(config);

	const auto result = resolver.resolve_mo2_overwrite_dir();
	REQUIRE_FALSE(result.empty());
	REQUIRE(QFileInfo(QString::fromStdString(result)).isDir());

	cleanup_temp_dir(base);
}

TEST_CASE("path_resolver_t::resolve_openmw_data_dir, returns config value", "[u][qt]")
{
	path_resolver_t::search_config_t config;
	config.openmw_data_dir = "/some/openmw/data";
	path_resolver_t resolver(config);

	REQUIRE(resolver.resolve_openmw_data_dir() == "/some/openmw/data");
}

TEST_CASE("path_resolver_t::workspace_roots, returns configured roots", "[u][qt]")
{
	path_resolver_t::search_config_t config;
	config.workspace_roots = { "root_a", "root_b", "root_c" };
	path_resolver_t resolver(config);

	const auto & roots = resolver.workspace_roots();
	REQUIRE(roots.size() == 3);
	REQUIRE(roots[0] == "root_a");
	REQUIRE(roots[1] == "root_b");
	REQUIRE(roots[2] == "root_c");
}
