#include <catch2/catch_all.hpp>
#include <scanner/auto_merge.hpp>
#include <scanner/dialogue_arranger.hpp>
#include <scanner/plugin_scan.hpp>
#include <utility/record_behavior.hpp>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

merge_record_t make_merge_record(const std::string & rec_type, const std::string & record_id)
{
	return { rec_type, record_id, rec_type + ":" + record_id };
}

std::vector<std::string> describe(const std::vector<merge_record_t> & records)
{
	std::vector<std::string> labels;
	for (const auto & record : records)
		labels.push_back(record.rec_type + " " + record.record_id);

	return labels;
}

dialogue_arranger::find_topic_dial_fn_t make_dial_lookup(const std::map<std::string, std::string> & dial_contents)
{
	return [dial_contents](const std::string & topic) -> std::optional<std::string>
	{
		const auto it_found = dial_contents.find(topic);
		if (it_found == dial_contents.end())
			return std::nullopt;

		return it_found->second;
	};
}

} // namespace

TEST_CASE("record_type_for_decode_mode, dialogue parent type comes from the behavior table", "[u]")
{
	REQUIRE(std::string(record_type_for_decode_mode(decode_mode_t::dial)) == "DIAL");
	REQUIRE(std::string(record_type_for_decode_mode(decode_mode_t::info)) == "INFO");
}

TEST_CASE("dialogue_arranger::topic_of_info, splits at the last separator", "[u]")
{
	REQUIRE(dialogue_arranger::topic_of_info("greeting 1|12345") == "greeting 1");
	REQUIRE(dialogue_arranger::topic_of_info("odd|topic|987") == "odd|topic");
}

TEST_CASE("dialogue_arranger::arrange, well-formed order is unchanged", "[u]")
{
	const std::vector<merge_record_t> records = {
		make_merge_record("CELL", "balmora"),    make_merge_record("DIAL", "alpha"), make_merge_record("INFO", "alpha|1"),
		make_merge_record("INFO", "alpha|2"),    make_merge_record("DIAL", "beta"),  make_merge_record("INFO", "beta|3"),
		make_merge_record("NPC_", "guard"),
	};

	const auto result = dialogue_arranger::arrange({ records, make_dial_lookup({}) });

	REQUIRE(describe(result.records) == describe(records));
	REQUIRE(result.records[2].content == records[2].content);
	REQUIRE(result.inserted_topics.empty());
	REQUIRE(result.missing_topics.empty());
}

TEST_CASE("dialogue_arranger::arrange, inserts the parent DIAL before an orphan INFO", "[u]")
{
	const std::vector<merge_record_t> records = {
		make_merge_record("CELL", "balmora"),
		make_merge_record("INFO", "alpha|1"),
	};

	const auto result = dialogue_arranger::arrange({ records, make_dial_lookup({ { "alpha", "winner dial" } }) });

	REQUIRE(describe(result.records) == std::vector<std::string> { "CELL balmora", "DIAL alpha", "INFO alpha|1" });
	REQUIRE(result.records[1].content == "winner dial");
	REQUIRE(result.inserted_topics == std::vector<std::string> { "alpha" });
}

TEST_CASE("dialogue_arranger::arrange, groups interleaved INFOs under their own topic", "[u]")
{
	const std::vector<merge_record_t> records = {
		make_merge_record("INFO", "alpha|1"),
		make_merge_record("CELL", "balmora"),
		make_merge_record("INFO", "beta|2"),
		make_merge_record("INFO", "alpha|3"),
	};
	const auto lookup = make_dial_lookup({ { "alpha", "dial a" }, { "beta", "dial b" } });

	const auto result = dialogue_arranger::arrange({ records, lookup });

	REQUIRE(
	    describe(result.records) ==
	    std::vector<std::string> { "DIAL alpha", "INFO alpha|1", "INFO alpha|3", "CELL balmora", "DIAL beta", "INFO beta|2" });
}

TEST_CASE("dialogue_arranger::arrange, an existing DIAL is moved before its INFOs, not duplicated", "[u]")
{
	const std::vector<merge_record_t> records = {
		make_merge_record("INFO", "alpha|1"),
		make_merge_record("DIAL", "alpha"),
	};

	const auto result = dialogue_arranger::arrange({ records, make_dial_lookup({ { "alpha", "winner dial" } }) });

	REQUIRE(describe(result.records) == std::vector<std::string> { "DIAL alpha", "INFO alpha|1" });
	REQUIRE(result.records[0].content == records[1].content);
	REQUIRE(result.inserted_topics.empty());
}

TEST_CASE("dialogue_arranger::arrange, unknown topic keeps the INFO and reports it", "[u]")
{
	const std::vector<merge_record_t> records = {
		make_merge_record("CELL", "balmora"),
		make_merge_record("INFO", "gamma|1"),
	};

	const auto result = dialogue_arranger::arrange({ records, make_dial_lookup({}) });

	REQUIRE(describe(result.records) == std::vector<std::string> { "CELL balmora", "INFO gamma|1" });
	REQUIRE(result.missing_topics == std::vector<std::string> { "gamma" });
	REQUIRE(result.inserted_topics.empty());
}

namespace {

std::string make_sub(const std::string & type, const std::string & payload)
{
	std::string result = type;
	const auto size_value = static_cast<uint32_t>(payload.size());
	result.append(reinterpret_cast<const char *>(&size_value), sizeof(size_value));
	return result + payload;
}

std::string make_record(const std::string & rec_type, const std::string & subs)
{
	std::string result = rec_type;
	const auto body_size = static_cast<uint32_t>(subs.size());
	const uint32_t header_unused = 0;
	const uint32_t record_flags = 0;
	result.append(reinterpret_cast<const char *>(&body_size), sizeof(body_size));
	result.append(reinterpret_cast<const char *>(&header_unused), sizeof(header_unused));
	result.append(reinterpret_cast<const char *>(&record_flags), sizeof(record_flags));
	return result + subs;
}

std::string make_zstring(const std::string & text)
{
	return text + std::string(1, '\0');
}

struct info_fields_t
{
	std::string previous_link;
	std::string speaker;
	std::string response;
};

std::string make_dialogue_plugin(const info_fields_t & fields)
{
	constexpr size_t tes3_header_size = 300;
	const auto tes3 = make_record("TES3", make_sub("HEDR", std::string(tes3_header_size, '\0')));
	const auto dial = make_record("DIAL", make_sub("NAME", make_zstring("topic")) + make_sub("DATA", std::string(1, '\0')));
	const auto info = make_record(
	    "INFO",
	    make_sub("INAM", make_zstring("100")) + make_sub("PNAM", make_zstring(fields.previous_link)) +
	        make_sub("NNAM", make_zstring("")) + make_sub("ONAM", make_zstring(fields.speaker)) +
	        make_sub("NAME", fields.response));
	return tes3 + dial + info;
}

std::string write_temp_plugin(const std::string & filename, const std::string & body)
{
	const auto path = (std::filesystem::temp_directory_path() / filename).string();
	std::ofstream file(path, std::ios::binary);
	file.write(body.data(), static_cast<std::streamsize>(body.size()));
	return path;
}

} // namespace

TEST_CASE("dialogue_arranger::arrange_active, merged INFO is written under its winner DIAL", "[i]")
{
	const auto master_path = write_temp_plugin("yampt_dial_master.esm", make_dialogue_plugin({ "", "speaker_a", "hello" }));
	const auto middle_path =
	    write_temp_plugin("yampt_dial_middle.esp", make_dialogue_plugin({ "middle_prev", "speaker_a", "hello mod" }));
	const auto winner_path =
	    write_temp_plugin("yampt_dial_winner.esp", make_dialogue_plugin({ "winner_prev", "speaker_b", "hello" }));

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(middle_path);
	scan.load_plugin(winner_path);
	scan.set_active_plugin("Merged Patch.esp");
	scan.rebuild_conflicts();

	auto_merge_t merge(scan);
	merge.execute();

	REQUIRE(scan.find_active_content("INFO", "topic|100") != nullptr);

	const auto result = dialogue_arranger::arrange_active(scan);

	REQUIRE(describe(result.records) == std::vector<std::string> { "DIAL topic", "INFO topic|100" });
	const auto * dial_entry = scan.find("DIAL", "topic");
	REQUIRE(dial_entry != nullptr);
	const auto & winner_dial = dial_entry->versions.back();
	REQUIRE(scan.plugin_filename(winner_dial.plugin_idx) == "yampt_dial_winner.esp");
	REQUIRE(result.records[0].content == scan.read_record_content(winner_dial.plugin_idx, winner_dial.record_index));

	const auto & merged_info = result.records[1].content;
	REQUIRE(merged_info.find(make_sub("PNAM", make_zstring("winner_prev"))) != std::string::npos);
	REQUIRE(merged_info.find(make_sub("ONAM", make_zstring("speaker_b"))) != std::string::npos);
	REQUIRE(merged_info.find(make_sub("NAME", "hello mod")) != std::string::npos);

	std::filesystem::remove(master_path);
	std::filesystem::remove(middle_path);
	std::filesystem::remove(winner_path);
}
