#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.editor/source/model/view_tree_model.hpp"
#include <scanner/plugin_scan.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string make_sub(const std::string & type, const std::string & data)
{
	std::string result;
	result += type;
	const auto size_val = static_cast<uint32_t>(data.size());
	result.append(reinterpret_cast<const char *>(&size_val), 4);
	result += data;
	return result;
}

std::string make_record(const std::string & rec_type, const std::string & subs)
{
	std::string header;
	header += rec_type;
	const auto body_size = static_cast<uint32_t>(subs.size());
	header.append(reinterpret_cast<const char *>(&body_size), 4);
	header.append(8, '\0');
	return header + subs;
}

std::string make_string_field(const std::string & text)
{
	return text + std::string(1, '\0');
}

std::string make_uint16(uint16_t value)
{
	std::string result(2, '\0');
	std::memcpy(result.data(), &value, 2);
	return result;
}

std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

std::string levi_entry(const std::string & item_id, uint16_t level)
{
	return make_sub("INAM", make_string_field(item_id)) + make_sub("INTV", make_uint16(level));
}

std::string make_tes3_header_record()
{
	return make_record("TES3", make_sub("HEDR", std::string(300, '\0')));
}

std::string npco_item(const std::string & item_id)
{
	std::string data(36, '\0');
	const uint32_t count = 1;
	std::memcpy(data.data(), &count, 4);
	const auto id = make_string_field(item_id);
	std::memcpy(data.data() + 4, id.data(), std::min<size_t>(32, id.size()));
	return data;
}

std::string temp_plugin_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

void write_plugin_file(const std::string & path, const std::string & body)
{
	std::ofstream file(path, std::ios::binary);
	file.write(body.data(), body.size());
}

size_t count_item_group_rows(const view_tree_model_t & model, const std::string & label_prefix)
{
	size_t count = 0;
	for (const auto & row : model.rows())
	{
		if (row.label.rfind(label_prefix, 0) == 0)
			++count;
	}

	return count;
}

} // namespace

TEST_CASE("view_tree_model_t::set_record, leveled list shows every duplicate item", "[gui]")
{
	const auto subs = make_sub("NAME", make_string_field("l_b_amulets")) + make_sub("DATA", make_uint32(1)) +
	                  make_sub("NNAM", std::string(1, '\0')) + make_sub("INDX", make_uint32(3)) +
	                  levi_entry("l_n_amulet", 1) + levi_entry("l_n_amulet", 1) + levi_entry("l_m_amulet", 1);

	const auto body = make_tes3_header_record() + make_record("LEVI", subs);
	const auto path = temp_plugin_path("yampt_gui_levi_dup.esm");
	write_plugin_file(path, body);

	plugin_scan_t scan;
	REQUIRE(scan.load_plugin(path));
	scan.rebuild_conflicts();

	const auto * entry = scan.find("LEVI", "l_b_amulets");
	REQUIRE(entry != nullptr);

	view_tree_model_t model;
	model.set_record(scan, *entry);

	REQUIRE(count_item_group_rows(model, "Item #") == 3);

	std::filesystem::remove(path);
}

TEST_CASE("view_tree_model_t::set_record, container shows every duplicate item", "[gui]")
{
	const auto subs = make_sub("NAME", make_string_field("barrel_dup")) + make_sub("FNAM", make_string_field("Barrel")) +
	                  make_sub("NPCO", npco_item("misc_com_bottle_01")) +
	                  make_sub("NPCO", npco_item("misc_com_bottle_01")) +
	                  make_sub("NPCO", npco_item("misc_com_bottle_02"));

	const auto body = make_tes3_header_record() + make_record("CONT", subs);
	const auto path = temp_plugin_path("yampt_gui_cont_dup.esm");
	write_plugin_file(path, body);

	plugin_scan_t scan;
	REQUIRE(scan.load_plugin(path));
	scan.rebuild_conflicts();

	const auto * entry = scan.find("CONT", "barrel_dup");
	REQUIRE(entry != nullptr);

	view_tree_model_t model;
	model.set_record(scan, *entry);

	size_t npco_rows = 0;
	for (const auto & row : model.rows())
	{
		if (row.type == "NPCO")
			++npco_rows;
	}

	REQUIRE(npco_rows == 3);

	std::filesystem::remove(path);
}
