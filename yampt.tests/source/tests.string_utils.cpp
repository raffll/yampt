#include <catch2/catch_all.hpp>
#include <filesystem>
#include <utility/string_utils.hpp>

TEST_CASE("string_utils::to_lower, ascii mixed case", "[u]")
{
	REQUIRE(string_utils::to_lower("Hello World") == "hello world");
	REQUIRE(string_utils::to_lower("ABCXYZ") == "abcxyz");
	REQUIRE(string_utils::to_lower("already") == "already");
	REQUIRE(string_utils::to_lower("") == "");
}

TEST_CASE("string_utils::join_path, single separator regardless of trailing or leading slash", "[u]")
{
	REQUIRE(string_utils::join_path("dir", "file.txt") == "dir/file.txt");
	REQUIRE(string_utils::join_path("dir/", "file.txt") == "dir/file.txt");
	REQUIRE(string_utils::join_path("dir", "/file.txt") == "dir/file.txt");
	REQUIRE(string_utils::join_path("dir/", "/file.txt") == "dir/file.txt");
	REQUIRE(string_utils::join_path("a/b/", "c/d") == "a/b/c/d");
	REQUIRE(string_utils::join_path("", "file.txt") == "file.txt");
	REQUIRE(string_utils::join_path("dir", "") == "dir");
}

TEST_CASE("string_utils::normalize_path, backslash to forward slash", "[u]")
{
	REQUIRE(string_utils::normalize_path("C:\\Users\\test\\file.txt") == "C:/Users/test/file.txt");
	REQUIRE(string_utils::normalize_path("no/change") == "no/change");
	REQUIRE(string_utils::normalize_path("") == "");
	REQUIRE(string_utils::normalize_path("\\\\server\\share") == "//server/share");
}

TEST_CASE("string_utils::normalize_path, trailing slash stripping", "[u]")
{
	REQUIRE(string_utils::normalize_path("C:/Users/workspace/") == "C:/Users/workspace");
	REQUIRE(string_utils::normalize_path("C:\\Users\\workspace\\") == "C:/Users/workspace");
	REQUIRE(string_utils::normalize_path("/home/user/") == "/home/user");
	REQUIRE(string_utils::normalize_path("/home/user///") == "/home/user");
	REQUIRE(string_utils::normalize_path("//server/share/") == "//server/share");
	REQUIRE(string_utils::normalize_path("//server/share/folder/") == "//server/share/folder");
	REQUIRE(string_utils::normalize_path("/") == "/");
	REQUIRE(string_utils::normalize_path("C:/") == "C:/");
	REQUIRE(string_utils::normalize_path("D:\\") == "D:/");
}

TEST_CASE("string_utils::extract_filename, various paths", "[u]")
{
	REQUIRE(string_utils::extract_filename("C:/Users/test/file.txt") == "file.txt");
	REQUIRE(string_utils::extract_filename("C:\\data\\dict.json") == "dict.json");
	REQUIRE(string_utils::extract_filename("file.txt") == "file.txt");
	REQUIRE(string_utils::extract_filename("") == "");
	REQUIRE(string_utils::extract_filename("/root/") == "");
}

TEST_CASE("string_utils::trim, whitespace removal", "[u]")
{
	REQUIRE(string_utils::trim("  hello  ") == "hello");
	REQUIRE(string_utils::trim("\t\r\nspaced\r\n") == "spaced");
	REQUIRE(string_utils::trim("nospace") == "nospace");
	REQUIRE(string_utils::trim("") == "");
	REQUIRE(string_utils::trim("   ") == "");
}

TEST_CASE("string_utils::case_insensitive_equal, various inputs", "[u]")
{
	REQUIRE(string_utils::case_insensitive_equal("Hello", "hello") == true);
	REQUIRE(string_utils::case_insensitive_equal("ABC", "abc") == true);
	REQUIRE(string_utils::case_insensitive_equal("", "") == true);
	REQUIRE(string_utils::case_insensitive_equal("Hello", "World") == false);
	REQUIRE(string_utils::case_insensitive_equal("short", "longer") == false);
}

TEST_CASE("string_utils::to_lower, ascii behavior preserved as regression guard", "[u]")
{
	REQUIRE(string_utils::to_lower("BALMORA") == "balmora");
	REQUIRE(string_utils::to_lower("Vivec City") == "vivec city");
	REQUIRE(string_utils::to_lower("Mix3d C4se!") == "mix3d c4se!");
	REQUIRE(string_utils::to_lower("digits 123 stay") == "digits 123 stay");
}

TEST_CASE("string_utils::to_lower, preserves byte length on accented input", "[u]")
{
	const std::string upper = "B\xc4\x84lmora";
	const auto lowered = string_utils::to_lower(upper);
	REQUIRE(lowered.size() == upper.size());
}

TEST_CASE("string_utils::paths_equal, same directory different spelling", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:\\OMEN\\workspace", "C:/OMEN/workspace") == true);
	REQUIRE(string_utils::paths_equal("C:/OMEN/workspace", "c:/omen/workspace") == true);
	REQUIRE(string_utils::paths_equal("C:\\OMEN\\workspace\\", "C:/OMEN/workspace") == true);
	REQUIRE(string_utils::paths_equal("C:\\OMEN\\Workspace\\", "c:/omen/workspace") == true);
	REQUIRE(string_utils::paths_equal("", "") == true);
}

TEST_CASE("string_utils::paths_equal, different directories", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/OMEN/workspace", "C:/OMEN/other") == false);
	REQUIRE(string_utils::paths_equal("C:/OMEN/workspace", "C:/OMEN/workspace/sub") == false);
	REQUIRE(string_utils::paths_equal("D:/workspace", "C:/workspace") == false);
}

TEST_CASE("string_utils::erase_null_chars, removes from first null", "[u]")
{
	REQUIRE(string_utils::erase_null_chars("hello") == "hello");
	REQUIRE(string_utils::erase_null_chars(std::string("he\0llo", 6)) == "he");
	REQUIRE(string_utils::erase_null_chars(std::string("\0abc", 4)) == "");
	REQUIRE(string_utils::erase_null_chars("") == "");
}

TEST_CASE("string_utils::trim_cr, trailing carriage return", "[u]")
{
	REQUIRE(string_utils::trim_cr("hello\r") == "hello");
	REQUIRE(string_utils::trim_cr("hello") == "hello");
	REQUIRE(string_utils::trim_cr("\r") == "");
	REQUIRE(string_utils::trim_cr("") == "");
	REQUIRE(string_utils::trim_cr("multi\r\r") == "multi\r");
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, ascii only", "[u]")
{
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 0) == 0);
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 3) == 3);
	REQUIRE(string_utils::utf8_byte_to_char_offset("hello", 5) == 5);
}

TEST_CASE("string_utils::utf8_byte_to_char_offset, multi-byte chars", "[u]")
{
	const std::string polish = "\xC4\x85\xC4\x87\xC4\x99";
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 0) == 0);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 2) == 1);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 4) == 2);
	REQUIRE(string_utils::utf8_byte_to_char_offset(polish, 6) == 3);
}

TEST_CASE("string_utils::replace_non_printable_with_dot, mixed content", "[u]")
{
	REQUIRE(string_utils::replace_non_printable_with_dot("hello") == "hello");
	REQUIRE(
	    string_utils::replace_non_printable_with_dot(
	        "a\x01"
	        "b") == "a.b");
	REQUIRE(string_utils::replace_non_printable_with_dot(std::string("\x00\x1F", 2)) == "..");
	REQUIRE(string_utils::replace_non_printable_with_dot("") == "");
}

TEST_CASE("string_utils::path_to_utf8, ascii path round-trips", "[u]")
{
	const std::filesystem::path ascii_path = "C:/mods/scripts/handler.lua";
	REQUIRE(string_utils::path_to_utf8(ascii_path) == "C:/mods/scripts/handler.lua");
}

TEST_CASE("string_utils::path_to_utf8, non-ansi filename does not throw", "[u]")
{
	const std::u8string unicode_name = u8"\u6708\u660e\u304b\u308a.lua";
	const std::filesystem::path unicode_path = std::filesystem::path(unicode_name);

	std::string converted;
	REQUIRE_NOTHROW(converted = string_utils::path_to_utf8(unicode_path));

	const std::string expected(reinterpret_cast<const char *>(unicode_name.data()), unicode_name.size());
	REQUIRE(converted == expected);
}

TEST_CASE("string_utils::path_to_utf8, cyrillic filename converts to utf8", "[u]")
{
	const std::u8string unicode_name = u8"\u041c\u043e\u0434.omwscripts";
	const std::filesystem::path unicode_path = std::filesystem::path(unicode_name);

	const auto converted = string_utils::path_to_utf8(unicode_path);
	const std::string expected(reinterpret_cast<const char *>(unicode_name.data()), unicode_name.size());
	REQUIRE(converted == expected);
}

TEST_CASE("string_utils::utf8_to_path, round-trips through path_to_utf8", "[u]")
{
	const std::u8string unicode_name = u8"\u041c\u043e\u0434\u6708.omwscripts";
	const std::string utf8_text(reinterpret_cast<const char *>(unicode_name.data()), unicode_name.size());

	const auto path = string_utils::utf8_to_path(utf8_text);
	REQUIRE(string_utils::path_to_utf8(path) == utf8_text);
}
TEST_CASE("string_utils::canonicalize_path, empty input", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("") == "");
}

TEST_CASE("string_utils::canonicalize_path, unix root", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("/") == "/");
}

TEST_CASE("string_utils::canonicalize_path, drive root uppercased", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("c:/") == "C:/");
}

TEST_CASE("string_utils::canonicalize_path, drive path with backslashes", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("d:\\a\\b") == "D:/a/b");
}

TEST_CASE("string_utils::canonicalize_path, backslashes double-dot and trailing slash", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("C:\\Users\\..\\Users\\workspace\\") == "C:/Users/workspace");
}

TEST_CASE("string_utils::canonicalize_path, unc double-dot and trailing slash", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("//server/share/../share/folder/") == "//server/share/folder");
}

TEST_CASE("string_utils::canonicalize_path, dot and double-dot segments", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("/home/./user/../user/docs/") == "/home/user/docs");
}

TEST_CASE("string_utils::canonicalize_path, unix trailing slash", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("/a/b/") == "/a/b");
}

TEST_CASE("string_utils::canonicalize_path, redundant separators collapsed", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("C://Users///file") == "C:/Users/file");
}

TEST_CASE("string_utils::canonicalize_path, relative with double-dot", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("relative/../other") == "other");
}

TEST_CASE("string_utils::canonicalize_path, leading double-dot preserved in relative path", "[u]")
{
	REQUIRE(string_utils::canonicalize_path("../../up") == "../../up");
}

TEST_CASE("string_utils::paths_equal, identical paths equal", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/Users/x", "C:/Users/x") == true);
}

TEST_CASE("string_utils::paths_equal, separator differences equal", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:\\Users\\x", "C:/Users/x") == true);
}

TEST_CASE("string_utils::paths_equal, trailing slash differences equal", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/Users/x/", "C:/Users/x") == true);
}

TEST_CASE("string_utils::paths_equal, case differences equal on windows", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/Users/Workspace", "c:/users/workspace") == true);
}

TEST_CASE("string_utils::paths_equal, dot and double-dot equivalence", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/Users/../Users/x", "C:/Users/x") == true);
}

TEST_CASE("string_utils::paths_equal, redundant separators equal", "[u]")
{
	REQUIRE(string_utils::paths_equal("C://Users///x", "C:/Users/x") == true);
}

TEST_CASE("string_utils::paths_equal, genuinely different paths not equal", "[u]")
{
	REQUIRE(string_utils::paths_equal("C:/Users/a", "C:/Users/b") == false);
}
