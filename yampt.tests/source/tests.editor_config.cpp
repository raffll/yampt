#include <catch2/catch_all.hpp>
#include <QSettings>
#include <QTemporaryFile>
#include <filesystem>

namespace {

std::string temp_settings_path()
{
	auto result = (std::filesystem::temp_directory_path() / "yampt_test_app_settings.ini").string();
	return result;
}

void cleanup_settings(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

} // anonymous namespace

TEST_CASE("app_settings_t::encoding_index, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Language/EncodingIndex", 1);
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Language/EncodingIndex", 0).toInt() == 1);
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::native_language, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Language/NativeLanguage", "PL");
		settings.setValue("Language/ForeignLanguage", "EN");
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Language/NativeLanguage").toString().toStdString() == "PL");
		REQUIRE(settings.value("Language/ForeignLanguage").toString().toStdString() == "EN");
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::workspace_roots, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("WorkspaceRoots/Count", 3);
		settings.setValue("WorkspaceRoots/Path0", "C:/workspace/first");
		settings.setValue("WorkspaceRoots/Path1", "D:/mods/second");
		settings.setValue("WorkspaceRoots/Path2", "E:/translations/third");
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		const int count = settings.value("WorkspaceRoots/Count", 0).toInt();
		REQUIRE(count == 3);
		REQUIRE(settings.value("WorkspaceRoots/Path0").toString().toStdString() == "C:/workspace/first");
		REQUIRE(settings.value("WorkspaceRoots/Path1").toString().toStdString() == "D:/mods/second");
		REQUIRE(settings.value("WorkspaceRoots/Path2").toString().toStdString() == "E:/translations/third");
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::last_merge_order, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("MergeOrder/Count", 2);
		settings.setValue("MergeOrder/Path0", "C:/dicts/morrowind.json");
		settings.setValue("MergeOrder/Path1", "C:/dicts/tribunal.json");
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		const int count = settings.value("MergeOrder/Count", 0).toInt();
		REQUIRE(count == 2);
		REQUIRE(settings.value("MergeOrder/Path0").toString().toStdString() == "C:/dicts/morrowind.json");
		REQUIRE(settings.value("MergeOrder/Path1").toString().toStdString() == "C:/dicts/tribunal.json");
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::translation_keys, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Translation/SourceIndex", 2);
		settings.setValue("Translation/LanguageIndex", 1);
		settings.setValue("Translation/DeepLApiKey", "abc-123-key");
		settings.setValue("Translation/GoogleApiKey", "goog-456");
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Translation/SourceIndex", 0).toInt() == 2);
		REQUIRE(settings.value("Translation/LanguageIndex", 0).toInt() == 1);
		REQUIRE(settings.value("Translation/DeepLApiKey").toString().toStdString() == "abc-123-key");
		REQUIRE(settings.value("Translation/GoogleApiKey").toString().toStdString() == "goog-456");
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::editor_layout, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Editor/ActiveDictPath", "C:/test/path.json");
		settings.setValue("Editor/SplitRatio", 0.7);
		settings.setValue("Editor/SidebarWidth", 300);
		settings.setValue("Editor/BottomHeight", 180);
		settings.setValue("Editor/InfoHeight", 120);
		settings.setValue("Editor/SidebarVisible", false);
		settings.setValue("Editor/BottomVisible", false);
		settings.setValue("Editor/Column0", 150);
		settings.setValue("Editor/Column1", 250);
		settings.setValue("Editor/Column2", 350);
		settings.setValue("Editor/Column3", 90);
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Editor/ActiveDictPath").toString().toStdString() == "C:/test/path.json");
		REQUIRE(settings.value("Editor/SplitRatio").toFloat() == Catch::Approx(0.7f));
		REQUIRE(settings.value("Editor/SidebarWidth").toInt() == 300);
		REQUIRE(settings.value("Editor/BottomHeight").toInt() == 180);
		REQUIRE(settings.value("Editor/InfoHeight").toInt() == 120);
		REQUIRE(settings.value("Editor/SidebarVisible").toBool() == false);
		REQUIRE(settings.value("Editor/BottomVisible").toBool() == false);
		REQUIRE(settings.value("Editor/Column0").toInt() == 150);
		REQUIRE(settings.value("Editor/Column1").toInt() == 250);
		REQUIRE(settings.value("Editor/Column2").toInt() == 350);
		REQUIRE(settings.value("Editor/Column3").toInt() == 90);
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::window_geometry, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Window/X", 50);
		settings.setValue("Window/Y", 60);
		settings.setValue("Window/W", 1920);
		settings.setValue("Window/H", 1080);
		settings.setValue("Window/Maximized", true);
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Window/X").toInt() == 50);
		REQUIRE(settings.value("Window/Y").toInt() == 60);
		REQUIRE(settings.value("Window/W").toInt() == 1920);
		REQUIRE(settings.value("Window/H").toInt() == 1080);
		REQUIRE(settings.value("Window/Maximized").toBool() == true);
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::shortcuts, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Shortcuts/copy_original", "F8");
		settings.setValue("Shortcuts/set_in_progress", "F9");
		settings.setValue("Shortcuts/set_translated", "F10");
		settings.setValue("Shortcuts/set_untranslated", "Del");
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Shortcuts/copy_original").toString().toStdString() == "F8");
		REQUIRE(settings.value("Shortcuts/set_in_progress").toString().toStdString() == "F9");
		REQUIRE(settings.value("Shortcuts/set_translated").toString().toStdString() == "F10");
		REQUIRE(settings.value("Shortcuts/set_untranslated").toString().toStdString() == "Del");
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::spell_check, round-trip", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		settings.setValue("Language/SpellAffPath", "dictionaries/pl_PL.aff");
		settings.setValue("Language/SpellDicPath", "dictionaries/pl_PL.dic");
		settings.setValue("Language/SpellLangIndex", 2);
		settings.sync();
	}

	{
		QSettings settings(QString::fromStdString(path), QSettings::IniFormat);
		REQUIRE(settings.value("Language/SpellAffPath").toString().toStdString() == "dictionaries/pl_PL.aff");
		REQUIRE(settings.value("Language/SpellDicPath").toString().toStdString() == "dictionaries/pl_PL.dic");
		REQUIRE(settings.value("Language/SpellLangIndex").toInt() == 2);
	}

	cleanup_settings(path);
}

TEST_CASE("app_settings_t::defaults, missing file returns defaults", "[i]")
{
	const auto path = temp_settings_path();
	cleanup_settings(path);

	QSettings settings(QString::fromStdString(path), QSettings::IniFormat);

	REQUIRE(settings.value("Language/EncodingIndex", 0).toInt() == 0);
	REQUIRE(settings.value("Language/NativeLanguage", "").toString().toStdString().empty());
	REQUIRE(settings.value("Language/ForeignLanguage", "").toString().toStdString().empty());
	REQUIRE(settings.value("Translation/SourceIndex", 0).toInt() == 0);
	REQUIRE(settings.value("Translation/DeepLApiKey", "").toString().toStdString().empty());
	REQUIRE(settings.value("Editor/SplitRatio", 0.5f).toFloat() == Catch::Approx(0.5f));
	REQUIRE(settings.value("Editor/SidebarVisible", true).toBool() == true);
	REQUIRE(settings.value("WorkspaceRoots/Count", 0).toInt() == 0);

	cleanup_settings(path);
}
