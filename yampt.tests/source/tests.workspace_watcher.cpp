#include <catch2/catch_all.hpp>
#include <session/workspace_watcher.hpp>
#include <filesystem>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTest>

namespace {

std::string make_temp_dir(const std::string & suffix)
{
	namespace fs = std::filesystem;
	const auto base = fs::temp_directory_path() / ("yampt_watcher_" + suffix);
	fs::create_directories(base);
	return base.string();
}

void cleanup_temp_dir(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove_all(path, error_code);
}

} // namespace

TEST_CASE("workspace_watcher_t::set_watch_roots, does not crash on empty list", "[u][qt]")
{
	workspace_watcher_t watcher;
	watcher.set_watch_roots({});
	watcher.clear();
}

TEST_CASE("workspace_watcher_t::set_watch_roots, accepts valid directory", "[u][qt]")
{
	const auto dir = make_temp_dir("watcher_valid");

	workspace_watcher_t watcher;
	watcher.set_watch_roots({ QString::fromStdString(dir) });
	watcher.clear();

	cleanup_temp_dir(dir);
}

TEST_CASE("workspace_watcher_t::clear, safe to call multiple times", "[u][qt]")
{
	workspace_watcher_t watcher;
	watcher.clear();
	watcher.clear();
	watcher.clear();
}

TEST_CASE("workspace_watcher_t, emits workspace_changed on file creation", "[i][qt]")
{
	const auto dir = make_temp_dir("watcher_signal");

	workspace_watcher_t watcher;
	watcher.set_watch_roots({ QString::fromStdString(dir) });

	QSignalSpy spy(&watcher, &workspace_watcher_t::workspace_changed);
	REQUIRE(spy.isValid());

	QFile file(QString::fromStdString(dir + "/new_file.txt"));
	file.open(QIODevice::WriteOnly);
	file.write("test");
	file.close();

	QTest::qWait(600);
	QCoreApplication::processEvents();

	REQUIRE(spy.count() >= 1);

	watcher.clear();
	cleanup_temp_dir(dir);
}

TEST_CASE("workspace_watcher_t, does not emit after clear", "[i][qt]")
{
	const auto dir = make_temp_dir("watcher_no_emit");

	workspace_watcher_t watcher;
	watcher.set_watch_roots({ QString::fromStdString(dir) });
	watcher.clear();

	QSignalSpy spy(&watcher, &workspace_watcher_t::workspace_changed);

	QFile file(QString::fromStdString(dir + "/ignored.txt"));
	file.open(QIODevice::WriteOnly);
	file.write("ignored");
	file.close();

	QTest::qWait(600);
	QCoreApplication::processEvents();

	REQUIRE(spy.count() == 0);

	cleanup_temp_dir(dir);
}
