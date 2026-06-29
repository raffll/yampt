#include <catch2/catch_all.hpp>
#include <utility/tools.hpp>
#include <io/dict_reader.hpp>

TEST_CASE("dict_reader_t, not loaded for nonexistent file", "[u]")
{
	dict_reader_t reader("nonexistent_file_12345.json");
	REQUIRE(reader.is_loaded() == false);
}
