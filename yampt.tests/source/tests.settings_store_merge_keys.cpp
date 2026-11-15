#include <catch2/catch_all.hpp>
#include <settings_store.hpp>
#include <string>
#include <QDir>
#include <QFile>
#include <QSettings>

namespace {

const QString merge_keys_filename = "yampt_settings_merge_keys_test.ini";

QString merge_keys_path()
{
	return QDir(settings_store_t::settings_dir()).filePath(merge_keys_filename);
}

void rewrite_from_second_writer()
{
	QSettings second_writer(merge_keys_path(), QSettings::IniFormat);
	second_writer.setValue("Session/active_plugin_path", "C:/plugins/Merged Patch.esp");
	second_writer.sync();
}

} // namespace

TEST_CASE("settings_store_t::merge_excludes, survives another writer of the same file", "[i]")
{
	QFile::remove(merge_keys_path());
	const std::string rules = "id:^ahnassi$\ntype:SNDG\n";

	settings_store_t store(merge_keys_filename);
	store.set_merge_fog_fix_enabled(false);
	store.set_merge_summon_fix_enabled(false);
	store.set_merge_excludes(rules);
	store.sync();

	rewrite_from_second_writer();
	store.sync();

	REQUIRE(store.merge_excludes() == rules);
	REQUIRE_FALSE(store.merge_fog_fix_enabled());

	const settings_store_t reopened(merge_keys_filename);
	REQUIRE(reopened.merge_excludes() == rules);
	REQUIRE_FALSE(reopened.merge_summon_fix_enabled());

	QFile::remove(merge_keys_path());
}

TEST_CASE("settings_store_t::merge_excludes, survives a long-lived store saving a merge setting", "[i]")
{
	QFile::remove(merge_keys_path());
	const std::string rules = "id:^ahnassi$\n";

	settings_store_t long_lived_store(merge_keys_filename);
	REQUIRE(long_lived_store.merge_excludes().empty());

	settings_store_t context_menu_store(merge_keys_filename);
	context_menu_store.set_merge_excludes(rules);
	context_menu_store.sync();

	long_lived_store.set_merge_fog_fix_enabled(false);
	long_lived_store.sync();
	context_menu_store.sync();

	REQUIRE(context_menu_store.merge_excludes() == rules);
	REQUIRE(long_lived_store.merge_excludes() == rules);

	QFile::remove(merge_keys_path());
}

TEST_CASE("settings_store_t::merge_excludes, reads rules saved by earlier versions", "[i]")
{
	QFile::remove(merge_keys_path());
	{
		QFile legacy_file(merge_keys_path());
		REQUIRE(legacy_file.open(QIODevice::WriteOnly | QIODevice::Text));
		legacy_file.write("[merge]\nfog_fix=false\n\n[Merge]\nExcludes=id:^ahnassi$\\ntype:SNDG\\n\n");
	}

	settings_store_t store(merge_keys_filename);
	REQUIRE(store.merge_excludes() == "id:^ahnassi$\ntype:SNDG\n");
	REQUIRE_FALSE(store.merge_fog_fix_enabled());

	QFile::remove(merge_keys_path());
}

TEST_CASE("settings_store_t::merge_excludes, every merge setting lives in one ini section", "[i]")
{
	QFile::remove(merge_keys_path());
	{
		settings_store_t store(merge_keys_filename);
		store.set_merge_fog_fix_enabled(false);
		store.set_merge_excludes("type:SNDG\n");
		store.sync();
	}

	QFile file(merge_keys_path());
	REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
	const auto content = QString::fromUtf8(file.readAll());
	file.close();

	const auto merge_sections = content.count("[merge]", Qt::CaseInsensitive);
	REQUIRE(merge_sections == 1);

	QFile::remove(merge_keys_path());
}
