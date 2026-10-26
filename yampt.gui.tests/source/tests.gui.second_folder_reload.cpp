#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.editor/source/model/view_tree_model.hpp"
#include <scanner/plugin_scan.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

std::string make_tes3_header_record()
{
	return make_record("TES3", make_sub("HEDR", std::string(300, '\0')));
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

std::string make_activator_plugin_body(const std::string & record_id)
{
	const auto subs = make_sub("NAME", make_string_field(record_id)) + make_sub("FNAM", make_string_field("Signpost"));
	return make_tes3_header_record() + make_record("ACTI", subs);
}

} // namespace

TEST_CASE("view_tree_model_t::clear, drops stale plugin columns after scan reload", "[gui]")
{
	const std::string record_id = "yampt_reload_acti";
	const auto path_a1 = temp_plugin_path("yampt_reload_a1.esm");
	const auto path_a2 = temp_plugin_path("yampt_reload_a2.esp");
	const auto path_b1 = temp_plugin_path("yampt_reload_b1.esm");

	write_plugin_file(path_a1, make_activator_plugin_body(record_id));
	write_plugin_file(path_a2, make_activator_plugin_body(record_id));
	write_plugin_file(path_b1, make_activator_plugin_body(record_id));

	plugin_scan_t scan;
	REQUIRE(scan.load_plugin(path_a1));
	REQUIRE(scan.load_plugin(path_a2));
	scan.rebuild_conflicts();

	const auto * entry_a = scan.find("ACTI", record_id);
	REQUIRE(entry_a != nullptr);

	view_tree_model_t model;
	model.set_record(scan, *entry_a);

	const auto & indices_a = model.column_plugin_indices();
	const bool has_second_plugin_column =
	    std::any_of(indices_a.begin(), indices_a.end(), [](int plugin_idx) { return plugin_idx >= 1; });
	REQUIRE(has_second_plugin_column);

	scan = plugin_scan_t();
	REQUIRE(scan.load_plugin(path_b1));
	scan.rebuild_conflicts();

	model.clear();

	REQUIRE(model.column_plugin_indices().empty());
	REQUIRE(model.record_type().empty());

	const int section_count = static_cast<int>(indices_a.size()) + 1;
	for (int section = 0; section < section_count; ++section)
	{
		const auto header = model.headerData(section, Qt::Horizontal, Qt::DisplayRole);
		REQUIRE(header.toString().isEmpty());
	}

	std::filesystem::remove(path_a1);
	std::filesystem::remove(path_a2);
	std::filesystem::remove(path_b1);
}

TEST_CASE("plugin_scan_t reload, second folder replaces the loaded plugin set", "[gui]")
{
	const std::string record_id = "yampt_replace_acti";
	const auto path_a1 = temp_plugin_path("yampt_replace_a1.esm");
	const auto path_a2 = temp_plugin_path("yampt_replace_a2.esp");
	const auto path_b1 = temp_plugin_path("yampt_replace_b1.esm");

	write_plugin_file(path_a1, make_activator_plugin_body(record_id));
	write_plugin_file(path_a2, make_activator_plugin_body(record_id));
	write_plugin_file(path_b1, make_activator_plugin_body(record_id));

	plugin_scan_t scan;
	REQUIRE(scan.load_plugin(path_a1));
	REQUIRE(scan.load_plugin(path_a2));
	scan.rebuild_conflicts();
	REQUIRE(scan.plugin_count() == 2);

	scan = plugin_scan_t();
	REQUIRE(scan.load_plugin(path_b1));
	scan.rebuild_conflicts();

	REQUIRE(scan.plugin_count() == 1);
	REQUIRE(scan.plugin_filename(0) == "yampt_replace_b1.esm");

	std::filesystem::remove(path_a1);
	std::filesystem::remove(path_a2);
	std::filesystem::remove(path_b1);
}
