#include <catch2/catch_all.hpp>
#include <io/editor_config.hpp>
#include <utility/string_utils.hpp>

#include <filesystem>
#include <fstream>

namespace {

std::string temp_config_path()
{
	auto result = (std::filesystem::temp_directory_path() / "yampt_test_editor_config.ini").string();
	return string_utils::normalize_path(result);
}

void write_config_file(const std::string & path, const std::string & content)
{
	std::ofstream output(path);
	output << content;
}

void cleanup_config(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

} // anonymous namespace

TEST_CASE("editor_config_t::load, missing file leaves defaults", "[i]")
{
	editor_config_t config;
	config.load("nonexistent_path_that_does_not_exist.ini");

	REQUIRE(config.split_ratio == Catch::Approx(0.5f));
	REQUIRE(config.sidebar_width == Catch::Approx(250.0f));
	REQUIRE(config.bottom_height == Catch::Approx(200.0f));
	REQUIRE(config.info_height == Catch::Approx(150.0f));
	REQUIRE(config.sidebar_visible == true);
	REQUIRE(config.bottom_visible == true);
	REQUIRE(config.encoding_index == 2);
	REQUIRE(config.active_dict_index == -1);
	REQUIRE(config.active_dict_path.empty());
	REQUIRE(config.window_x == -1);
	REQUIRE(config.window_y == -1);
	REQUIRE(config.window_w == 1280);
	REQUIRE(config.window_h == 720);
	REQUIRE(config.window_maximized == false);
	REQUIRE(config.workspace_roots.empty());
	REQUIRE(config.last_merge_order.empty());
	REQUIRE(config.spell_check_aff.empty());
	REQUIRE(config.spell_check_dic.empty());
	REQUIRE(config.spell_lang_index == -1);
	REQUIRE(config.deepl_api_key.empty());
	REQUIRE(config.translation_source_index == 0);
	REQUIRE(config.translation_language_index == 0);
}

TEST_CASE("editor_config_t::load, editor section parses all fields", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "ActiveDictIndex=3\n"
	    "ActiveDictPath=C:/dicts/test.json\n"
	    "DeepLApiKey=abc-123-key\n"
	    "TranslationSourceIndex=2\n"
	    "TranslationLanguageIndex=1\n"
	    "SplitRatio=0.7\n"
	    "SidebarWidth=300.0\n"
	    "BottomHeight=180.0\n"
	    "InfoHeight=120.5\n"
	    "SidebarVisible=0\n"
	    "BottomVisible=0\n"
	    "EncodingIndex=1\n"
	    "WindowX=100\n"
	    "WindowY=200\n"
	    "WindowW=1920\n"
	    "WindowH=1080\n"
	    "WindowMaximized=1\n"
	    "Column0=120.5\n"
	    "Column1=250.0\n"
	    "Column2=350.0\n"
	    "Column3=90.0\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.active_dict_index == 3);
	REQUIRE(config.active_dict_path == "C:/dicts/test.json");
	REQUIRE(config.deepl_api_key == "abc-123-key");
	REQUIRE(config.translation_source_index == 2);
	REQUIRE(config.translation_language_index == 1);
	REQUIRE(config.split_ratio == Catch::Approx(0.7f));
	REQUIRE(config.sidebar_width == Catch::Approx(300.0f));
	REQUIRE(config.bottom_height == Catch::Approx(180.0f));
	REQUIRE(config.info_height == Catch::Approx(120.5f));
	REQUIRE(config.sidebar_visible == false);
	REQUIRE(config.bottom_visible == false);
	REQUIRE(config.encoding_index == 1);
	REQUIRE(config.window_x == 100);
	REQUIRE(config.window_y == 200);
	REQUIRE(config.window_w == 1920);
	REQUIRE(config.window_h == 1080);
	REQUIRE(config.window_maximized == true);
	REQUIRE(config.column_widths[0] == Catch::Approx(120.5f));
	REQUIRE(config.column_widths[1] == Catch::Approx(250.0f));
	REQUIRE(config.column_widths[2] == Catch::Approx(350.0f));
	REQUIRE(config.column_widths[3] == Catch::Approx(90.0f));

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, workspace roots section", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[WorkspaceRoots]\n"
	    "Count=3\n"
	    "Path0=C:/workspace/first\n"
	    "Path1=D:/mods/second\n"
	    "Path2=E:/translations/third\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.workspace_roots.size() == 3);
	REQUIRE(config.workspace_roots[0] == "C:/workspace/first");
	REQUIRE(config.workspace_roots[1] == "D:/mods/second");
	REQUIRE(config.workspace_roots[2] == "E:/translations/third");

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, spell check section", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[SpellCheck]\n"
	    "AffPath=dictionaries/pl_PL.aff\n"
	    "DicPath=dictionaries/pl_PL.dic\n"
	    "LangIndex=2\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.spell_check_aff == "dictionaries/pl_PL.aff");
	REQUIRE(config.spell_check_dic == "dictionaries/pl_PL.dic");
	REQUIRE(config.spell_lang_index == 2);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, merge order section", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[MergeOrder]\n"
	    "Count=2\n"
	    "Path0=C:/dicts/morrowind.json\n"
	    "Path1=C:/dicts/tribunal.json\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.last_merge_order.size() == 2);
	REQUIRE(config.last_merge_order[0] == "C:/dicts/morrowind.json");
	REQUIRE(config.last_merge_order[1] == "C:/dicts/tribunal.json");

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, encoding index clamps to valid range", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "EncodingIndex=5\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.encoding_index == 2);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, encoding index rejects negative", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "EncodingIndex=-1\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.encoding_index == 2);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, invalid integer keeps default", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "ActiveDictIndex=not_a_number\n"
	    "WindowX=garbage\n"
	    "SplitRatio=invalid\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.active_dict_index == -1);
	REQUIRE(config.window_x == -1);
	REQUIRE(config.split_ratio == Catch::Approx(0.5f));

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, column index out of range ignored", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "Column99=500.0\n"
	    "Column0=200.0\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.column_widths[0] == Catch::Approx(200.0f));
	REQUIRE(config.column_widths[1] == Catch::Approx(300.f));
	REQUIRE(config.column_widths[2] == Catch::Approx(300.f));
	REQUIRE(config.column_widths[3] == Catch::Approx(80.f));

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, whitespace around keys and values trimmed", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "  ActiveDictIndex  =  7  \n"
	    "  ActiveDictPath  =  C:/path/to/dict.json  \n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.active_dict_index == 7);
	REQUIRE(config.active_dict_path == "C:/path/to/dict.json");

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, empty lines and unknown keys ignored", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "\n"
	    "UnknownKey=some_value\n"
	    "\n"
	    "ActiveDictIndex=5\n"
	    "\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.active_dict_index == 5);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, boolean fields treat non-zero as true", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[Editor]\n"
	    "SidebarVisible=1\n"
	    "BottomVisible=1\n"
	    "WindowMaximized=1\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.sidebar_visible == true);
	REQUIRE(config.bottom_visible == true);
	REQUIRE(config.window_maximized == true);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::save, round-trip preserves all fields", "[i]")
{
	const auto path = temp_config_path();

	editor_config_t original;
	original.split_ratio = 0.65f;
	original.column_widths = { 100.f, 200.f, 300.f, 400.f };
	original.workspace_roots = { "C:/root1", "D:/root2" };
	original.spell_check_aff = "dictionaries/en_US.aff";
	original.spell_check_dic = "dictionaries/en_US.dic";
	original.spell_lang_index = 1;
	original.sidebar_width = 280.0f;
	original.bottom_height = 220.0f;
	original.info_height = 160.0f;
	original.sidebar_visible = false;
	original.bottom_visible = false;
	original.encoding_index = 0;
	original.active_dict_index = 2;
	original.active_dict_path = "C:/test/path.json";
	original.last_merge_order = { "A.json", "B.json", "C.json" };
	original.deepl_api_key = "test-key-value";
	original.translation_source_index = 1;
	original.translation_language_index = 2;
	original.window_x = 50;
	original.window_y = 60;
	original.window_w = 1600;
	original.window_h = 900;
	original.window_maximized = true;

	original.save(path);

	editor_config_t loaded;
	loaded.load(path);

	REQUIRE(loaded.split_ratio == Catch::Approx(original.split_ratio));
	REQUIRE(loaded.column_widths[0] == Catch::Approx(original.column_widths[0]));
	REQUIRE(loaded.column_widths[1] == Catch::Approx(original.column_widths[1]));
	REQUIRE(loaded.column_widths[2] == Catch::Approx(original.column_widths[2]));
	REQUIRE(loaded.column_widths[3] == Catch::Approx(original.column_widths[3]));
	REQUIRE(loaded.workspace_roots == original.workspace_roots);
	REQUIRE(loaded.spell_check_aff == original.spell_check_aff);
	REQUIRE(loaded.spell_check_dic == original.spell_check_dic);
	REQUIRE(loaded.spell_lang_index == original.spell_lang_index);
	REQUIRE(loaded.sidebar_width == Catch::Approx(original.sidebar_width));
	REQUIRE(loaded.bottom_height == Catch::Approx(original.bottom_height));
	REQUIRE(loaded.info_height == Catch::Approx(original.info_height));
	REQUIRE(loaded.sidebar_visible == original.sidebar_visible);
	REQUIRE(loaded.bottom_visible == original.bottom_visible);
	REQUIRE(loaded.encoding_index == original.encoding_index);
	REQUIRE(loaded.active_dict_index == original.active_dict_index);
	REQUIRE(loaded.active_dict_path == original.active_dict_path);
	REQUIRE(loaded.last_merge_order == original.last_merge_order);
	REQUIRE(loaded.deepl_api_key == original.deepl_api_key);
	REQUIRE(loaded.translation_source_index == original.translation_source_index);
	REQUIRE(loaded.translation_language_index == original.translation_language_index);
	REQUIRE(loaded.window_x == original.window_x);
	REQUIRE(loaded.window_y == original.window_y);
	REQUIRE(loaded.window_w == original.window_w);
	REQUIRE(loaded.window_h == original.window_h);
	REQUIRE(loaded.window_maximized == original.window_maximized);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::save, empty deepl key not written", "[i]")
{
	const auto path = temp_config_path();

	editor_config_t config;
	config.deepl_api_key = "";
	config.save(path);

	std::ifstream input(path);
	std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

	REQUIRE(content.find("DeepLApiKey") == std::string::npos);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::save, spell check section omitted when empty", "[i]")
{
	const auto path = temp_config_path();

	editor_config_t config;
	config.spell_check_aff = "";
	config.spell_check_dic = "";
	config.spell_lang_index = -1;
	config.save(path);

	std::ifstream input(path);
	std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

	REQUIRE(content.find("[SpellCheck]") == std::string::npos);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::save, merge order section omitted when empty", "[i]")
{
	const auto path = temp_config_path();

	editor_config_t config;
	config.last_merge_order.clear();
	config.save(path);

	std::ifstream input(path);
	std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

	REQUIRE(content.find("[MergeOrder]") == std::string::npos);

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, multiple sections in single file", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[WorkspaceRoots]\n"
	    "Count=1\n"
	    "Path0=C:/workspace\n"
	    "\n"
	    "[Editor]\n"
	    "ActiveDictIndex=4\n"
	    "SplitRatio=0.8\n"
	    "\n"
	    "[SpellCheck]\n"
	    "AffPath=test.aff\n"
	    "DicPath=test.dic\n"
	    "LangIndex=0\n"
	    "\n"
	    "[MergeOrder]\n"
	    "Count=1\n"
	    "Path0=merge.json\n");

	editor_config_t config;
	config.load(path);

	REQUIRE(config.workspace_roots.size() == 1);
	REQUIRE(config.workspace_roots[0] == "C:/workspace");
	REQUIRE(config.active_dict_index == 4);
	REQUIRE(config.split_ratio == Catch::Approx(0.8f));
	REQUIRE(config.spell_check_aff == "test.aff");
	REQUIRE(config.spell_check_dic == "test.dic");
	REQUIRE(config.spell_lang_index == 0);
	REQUIRE(config.last_merge_order.size() == 1);
	REQUIRE(config.last_merge_order[0] == "merge.json");

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, workspace roots count resets vector", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[WorkspaceRoots]\n"
	    "Count=2\n"
	    "Path0=first\n"
	    "Path1=second\n");

	editor_config_t config;
	config.workspace_roots = { "stale_entry" };
	config.load(path);

	REQUIRE(config.workspace_roots.size() == 2);
	REQUIRE(config.workspace_roots[0] == "first");
	REQUIRE(config.workspace_roots[1] == "second");

	cleanup_config(path);
}

TEST_CASE("editor_config_t::load, merge order count resets vector", "[i]")
{
	const auto path = temp_config_path();
	write_config_file(
	    path,
	    "[MergeOrder]\n"
	    "Count=1\n"
	    "Path0=only.json\n");

	editor_config_t config;
	config.last_merge_order = { "old1.json", "old2.json" };
	config.load(path);

	REQUIRE(config.last_merge_order.size() == 1);
	REQUIRE(config.last_merge_order[0] == "only.json");

	cleanup_config(path);
}
