#include "patch_builder.hpp"
#include <scanner/sub_record_merge.hpp>
#include <utility/tools.hpp>
#include <algorithm>
#include <cstring>
#include <filesystem>

static constexpr float tes3_header_version = 1.3f;
static constexpr size_t tes3_hedr_size = 300;
static constexpr size_t tes3_author_offset = 8;
static constexpr size_t tes3_author_max_len = 32;
static constexpr size_t tes3_desc_offset = 40;
static constexpr size_t tes3_desc_max_len = 256;
static constexpr size_t tes3_numrec_offset = 296;
static constexpr size_t tes3_flags_offset = 4;
static constexpr size_t data_sub_record_size = 8;

void patch_builder_t::clear()
{
	m_records.clear();
}

std::vector<patch_builder_t::merge_record_t> patch_builder_t::collect_pinned_records() const
{
	std::vector<merge_record_t> pinned;
	for (const auto & record : m_records)
	{
		if (record.pinned)
			pinned.push_back(record);
	}
	return pinned;
}

void patch_builder_t::restore_pinned_records(const std::vector<merge_record_t> & pinned)
{
	for (const auto & pinned_record : pinned)
	{
		bool replaced = false;
		for (auto & existing : m_records)
		{
			if (existing.rec_type == pinned_record.rec_type && existing.record_id == pinned_record.record_id)
			{
				existing.content = pinned_record.content;
				existing.pinned = true;
				replaced = true;
				break;
			}
		}

		if (!replaced)
			m_records.push_back(pinned_record);
	}
}

void patch_builder_t::add_record(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	for (auto & existing : m_records)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			if (!existing.pinned)
				existing.content = content;

			return;
		}
	}

	m_records.push_back({ rec_type, record_id, content, false });
}

void patch_builder_t::add_record_raw(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	for (auto & existing : m_records)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			existing.content = content;
			return;
		}
	}

	m_records.push_back({ rec_type, record_id, content, false });
}

void patch_builder_t::pin_record(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	for (auto & existing : m_records)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			existing.content = content;
			existing.pinned = true;
			return;
		}
	}

	m_records.push_back({ rec_type, record_id, content, true });
}

bool patch_builder_t::is_pinned(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return record.pinned;
	}
	return false;
}

const std::string * patch_builder_t::find_content(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return &record.content;
	}
	return nullptr;
}

void patch_builder_t::remove_record(const std::string & type, const std::string & id)
{
	auto it = std::remove_if(
	    m_records.begin(),
	    m_records.end(),
	    [&](const merge_record_t & record) { return record.rec_type == type && record.record_id == id; });
	m_records.erase(it, m_records.end());
}

bool patch_builder_t::has_records() const
{
	return !m_records.empty();
}

size_t patch_builder_t::record_count() const
{
	return m_records.size();
}

const std::string & patch_builder_t::record_content(size_t index) const
{
	return m_records[index].content;
}

const std::string & patch_builder_t::record_type(size_t index) const
{
	return m_records[index].rec_type;
}

const std::string & patch_builder_t::record_id(size_t index) const
{
	return m_records[index].record_id;
}

void patch_builder_t::merge_leveled_list(const leveled_list_input_t & input)
{
	if (input.version_contents.size() < 2)
		return;

	merge_input_t merge_input;
	merge_input.rec_type = input.rec_type;
	merge_input.record_id = input.record_id;
	merge_input.version_contents = input.version_contents;

	const auto result = leveled_list_merge_t::merge(merge_input);
	if (!result.content.empty())
		add_record_raw(input.rec_type, input.record_id, result.content);
}

void patch_builder_t::merge_dialogue(const leveled_list_input_t & input)
{
	if (input.version_contents.empty())
		return;

	const auto & winning_dial = input.version_contents.back();
	add_record_raw("DIAL", input.record_id, winning_dial);
}

bool patch_builder_t::save(
    const std::string & output_path,
    const std::string & author,
    const std::string & description,
    const std::vector<master_entry_t> & masters)
{
	if (m_records.empty())
		return false;

	const auto header_content = build_tes3_header(author, description, m_records.size(), masters);

	std::vector<tools_t::record_t> records;
	records.push_back({ "TES3", header_content, header_content.size(), false });

	for (const auto & record : m_records)
		records.push_back({ record.rec_type, record.content, record.content.size(), false });

	const auto temp_path = output_path + ".tmp";
	tools_t::write_file(records, temp_path);

	if (!std::filesystem::exists(temp_path))
		return false;

	std::error_code error_code;
	std::filesystem::rename(temp_path, output_path, error_code);

	if (!error_code)
		return true;

	std::filesystem::copy_file(temp_path, output_path, std::filesystem::copy_options::overwrite_existing, error_code);

	std::filesystem::remove(temp_path, error_code);
	return std::filesystem::exists(output_path);
}

std::string patch_builder_t::build_tes3_header(
    const std::string & author,
    const std::string & description,
    size_t record_count,
    const std::vector<master_entry_t> & masters)
{
	std::string hedr_data(tes3_hedr_size, '\0');

	float version = tes3_header_version;
	std::memcpy(&hedr_data[0], &version, 4);

	uint32_t flags = 0;
	std::memcpy(&hedr_data[tes3_flags_offset], &flags, 4);

	for (size_t i = 0; i < author.size() && i < tes3_author_max_len; ++i)
		hedr_data[tes3_author_offset + i] = author[i];

	for (size_t i = 0; i < description.size() && i < tes3_desc_max_len; ++i)
		hedr_data[tes3_desc_offset + i] = description[i];

	uint32_t num_records = static_cast<uint32_t>(record_count);
	std::memcpy(&hedr_data[tes3_numrec_offset], &num_records, 4);

	std::string body;
	body += "HEDR";
	body += tools_t::convert_uint_to_string_byte_array(tes3_hedr_size);
	body += hedr_data;

	for (const auto & master : masters)
	{
		std::string filename = master.filename;
		filename.push_back('\0');

		body += "MAST";
		body += tools_t::convert_uint_to_string_byte_array(filename.size());
		body += filename;

		std::string size_data(data_sub_record_size, '\0');
		std::memcpy(&size_data[0], &master.file_size, data_sub_record_size);

		body += "DATA";
		body += tools_t::convert_uint_to_string_byte_array(data_sub_record_size);
		body += size_data;
	}

	std::string header;
	header += "TES3";
	header += tools_t::convert_uint_to_string_byte_array(body.size());

	std::string unknown(4, '\0');
	header += unknown;

	std::string rec_flags(4, '\0');
	header += rec_flags;

	header += body;

	return header;
}
