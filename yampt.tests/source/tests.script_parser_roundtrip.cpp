#include <catch2/catch_all.hpp>
#include <converter/script_parser.hpp>
#include <merger/dict_merger.hpp>
#include <utility/tools.hpp>
#include <random>

namespace {

static std::string size_byte(size_t value)
{
	return tools_t::convert_uint_to_string_byte_array(value).substr(0, 1);
}

struct generated_script_t
{
	std::string source;
	std::string scdt;
	std::vector<std::string> cell_names;
};

std::string random_cell_name(std::mt19937 & rng)
{
	static const std::vector<std::string> prefixes = { "Ald",   "Bal",     "Dren",    "Tel",       "Sadr",
		                                               "Maar",  "Gnar",    "Hla",     "Ebonheart", "Vivec",
		                                               "Seyda", "Caldera", "Pelagiad" };

	static const std::vector<std::string> suffixes = { "Velothi", "mora",     "ith Gor", " Plantation", " Mine",
		                                               "'Ruhn",   " Gandosa", " Neen",   " Fyr",        " Tower" };

	std::uniform_int_distribution<size_t> prefix_dist(0, prefixes.size() - 1);
	std::uniform_int_distribution<size_t> suffix_dist(0, suffixes.size() - 1);

	return prefixes[prefix_dist(rng)] + suffixes[suffix_dist(rng)];
}

std::string random_keyword(std::mt19937 & rng)
{
	static const std::vector<std::string> keywords = { "GetPCCell", "ShowMap", "PositionCell" };

	std::uniform_int_distribution<size_t> dist(0, keywords.size() - 1);
	return keywords[dist(rng)];
}

std::string build_source_line(const std::string & keyword, const std::string & cell_name)
{
	if (keyword == "GetPCCell")
		return "if ( GetPCCell \"" + cell_name + "\" == 1 )";

	if (keyword == "ShowMap")
		return "ShowMap \"" + cell_name + "\"";

	if (keyword == "PositionCell")
		return "PositionCell, 0.0, 0, 0, 0, \"" + cell_name + "\"";

	return "";
}

std::string build_scdt_entry(const std::string & keyword, const std::string & cell_name)
{
	if (keyword == "GetPCCell")
	{
		std::string comparison = " == 1";
		size_t text_size = cell_name.size();
		size_t expr_size = 2 + 1 + 1 + text_size + comparison.size();

		std::string entry;
		entry += size_byte(expr_size);
		entry += "X";
		entry += std::string(1, '\x00');
		entry += size_byte(text_size);
		entry += cell_name;
		entry += comparison;
		return entry;
	}

	std::string entry;
	entry += size_byte(cell_name.size());
	entry += cell_name;
	return entry;
}

generated_script_t generate_script(std::mt19937 & rng)
{
	std::uniform_int_distribution<int> line_count_dist(1, 5);
	const int line_count = line_count_dist(rng);

	generated_script_t result;
	std::string scdt_body;
	scdt_body += std::string(5, '\x00');

	for (int index = 0; index < line_count; ++index)
	{
		const auto kw = random_keyword(rng);
		const auto cell = random_cell_name(rng);
		result.cell_names.push_back(cell);

		if (index > 0)
			result.source += "\r\n";

		result.source += build_source_line(kw, cell);

		scdt_body += build_scdt_entry(kw, cell);
		scdt_body += std::string(3, '\x00');
	}

	result.scdt = scdt_body;
	return result;
}

} // namespace

TEST_CASE("script_parser_t, round-trip identity for untranslated cells", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed));

	const auto script_data = generate_script(rng);

	dict_merger_t merger;
	for (const auto & cell_name : script_data.cell_names)
	{
		merger.add_record(tools_t::rec_type_t::cell, cell_name, cell_name);
	}

	script_parser_t parser(
	    tools_t::rec_type_t::sctx, merger, "TestScript", "test.esm", script_data.source, script_data.scdt);

	REQUIRE(parser.get_new_script() == script_data.source);
	REQUIRE(parser.get_new_scdt() == script_data.scdt);
}
