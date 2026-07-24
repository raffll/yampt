#include "creator/loc_generator.hpp"

#include "creator/inflection.hpp"
#include "io/loc_file_writer.hpp"
#include "io/loc_types.hpp"
#include "utility/app_logger.hpp"
#include "utility/string_utils.hpp"

#include <regex>
#include <unordered_map>

static bool has_control_characters(const std::string & text)
{
    for (const auto ch : text)
    {
        const auto uch = static_cast<unsigned char>(ch);
        if (uch < 0x20 && uch != '\t' && uch != '\r' && uch != '\n')
            return true;
    }

    return false;
}

static bool is_whitespace_only(const std::string & text)
{
    return text.empty() ||
           text.find_first_not_of(" \t\r\n") == std::string::npos;
}

static bool is_ascii_only(const std::string & text)
{
    for (const auto ch : text)
    {
        if (static_cast<unsigned char>(ch) > 127)
            return false;
    }

    return true;
}

static bool should_skip_entry(const record_entry_t & entry)
{
    if (entry.status != status_t::translated)
        return true;

    if (entry.old_text == entry.new_text)
        return true;

    if (is_whitespace_only(entry.new_text))
        return true;

    if (has_control_characters(entry.old_text))
        return true;

    if (has_control_characters(entry.new_text))
        return true;

    return false;
}

static bool try_encode_entry(loc_types::loc_entry_t & entry, codepage_t codepage)
{
    const auto encoded_key = encode_from_utf8(entry.key, codepage);
    const auto encoded_value = encode_from_utf8(entry.value, codepage);

    const auto roundtrip_key = decode_to_utf8(encoded_key, codepage);
    const auto roundtrip_value = decode_to_utf8(encoded_value, codepage);

    if (roundtrip_key != entry.key || roundtrip_value != entry.value)
        return false;

    entry.key = encoded_key;
    entry.value = encoded_value;
    return true;
}

struct build_context_t
{
    const dict_t & dict;
    codepage_t codepage;
    int skipped_count = 0;
    int collision_warnings = 0;
};

static std::vector<loc_types::loc_entry_t> build_cel_entries(
    build_context_t & context)
{
    std::vector<loc_types::loc_entry_t> entries;
    const auto it_cell = context.dict.find(rec_type_t::cell);
    if (it_cell == context.dict.end())
        return entries;

    for (const auto & record : it_cell->second.records)
    {
        if (should_skip_entry(record))
            continue;

        loc_types::loc_entry_t entry{record.old_text, record.new_text};

        if (!try_encode_entry(entry, context.codepage))
        {
            app_logger_t::add_log(
                "[warning] cel: cannot encode entry \"" +
                record.old_text + "\"\r\n");
            ++context.skipped_count;
            continue;
        }

        entries.push_back(std::move(entry));
    }

    return entries;
}

static bool should_skip_dial_entry(const record_entry_t & entry, codepage_t codepage)
{
    if (should_skip_entry(entry))
        return true;

    if (codepage == codepage_t::windows_1251 && is_ascii_only(entry.new_text))
        return true;

    return false;
}

static std::vector<loc_types::loc_entry_t> build_mrk_entries(
    build_context_t & context)
{
    std::vector<loc_types::loc_entry_t> entries;
    const auto it_dial = context.dict.find(rec_type_t::dial);
    if (it_dial == context.dict.end())
        return entries;

    for (const auto & record : it_dial->second.records)
    {
        if (should_skip_dial_entry(record, context.codepage))
            continue;

        loc_types::loc_entry_t entry{record.new_text, record.old_text};

        if (!try_encode_entry(entry, context.codepage))
        {
            app_logger_t::add_log(
                "[warning] mrk: cannot encode entry \"" +
                record.new_text + "\"\r\n");
            ++context.skipped_count;
            continue;
        }

        entries.push_back(std::move(entry));
    }

    return entries;
}

struct top_build_context_t
{
    std::unordered_map<std::string, std::string> dedup_map;
    std::vector<loc_types::loc_entry_t> entries;
    int collision_warnings = 0;
    int skipped_count = 0;
    codepage_t codepage;
    inflection_t * inflector = nullptr;
};

static void insert_top_entry(
    top_build_context_t & context,
    const loc_types::loc_entry_t & candidate)
{
    if (candidate.key.size() < 3)
        return;

    const auto it_existing = context.dedup_map.find(candidate.key);
    if (it_existing != context.dedup_map.end())
    {
        if (it_existing->second != candidate.value)
        {
            app_logger_t::add_log(
                "[warning] top: collision \"" + candidate.key +
                "\" maps to \"" + it_existing->second +
                "\" and \"" + candidate.value + "\"\r\n");
            ++context.collision_warnings;
        }

        return;
    }

    loc_types::loc_entry_t entry = candidate;
    if (!try_encode_entry(entry, context.codepage))
    {
        app_logger_t::add_log(
            "[warning] top: cannot encode form \"" +
            candidate.key + "\"\r\n");
        ++context.skipped_count;
        return;
    }

    context.dedup_map[candidate.key] = candidate.value;
    context.entries.push_back(std::move(entry));
}

static void process_top_record(
    top_build_context_t & context,
    const record_entry_t & record)
{
    const auto & new_text = record.new_text;
    const auto lowercase_new = string_utils::to_lower(new_text);
    insert_top_entry(context, {lowercase_new, new_text});

    if (context.inflector == nullptr)
        return;

    const auto forms = context.inflector->phrase_forms(new_text);
    for (const auto & form : forms)
        insert_top_entry(context, {form, new_text});
}

struct top_input_t
{
    const std::string & aff_path;
    const std::string & dic_path;
};

static std::vector<loc_types::loc_entry_t> build_top_entries(
    build_context_t & context,
    const top_input_t & top_input)
{
    const auto it_dial = context.dict.find(rec_type_t::dial);
    if (it_dial == context.dict.end())
        return {};

    inflection_t inflector;
    bool inflection_available = false;

    if (!top_input.aff_path.empty() && !top_input.dic_path.empty())
    {
        inflection_available = inflector.load(
            top_input.aff_path, top_input.dic_path);
        if (!inflection_available)
        {
            app_logger_t::add_log(
                "[warning] hunspell dictionary failed to load, "
                "generating identity mappings only\r\n");
        }
    }
    else
    {
        app_logger_t::add_log(
            "[warning] no hunspell dictionary configured, "
            "generating identity mappings only\r\n");
    }

    top_build_context_t top_context;
    top_context.codepage = context.codepage;
    top_context.inflector = inflection_available ? &inflector : nullptr;

    for (const auto & record : it_dial->second.records)
    {
        if (should_skip_dial_entry(record, context.codepage))
            continue;

        process_top_record(top_context, record);
    }

    context.skipped_count += top_context.skipped_count;
    context.collision_warnings = top_context.collision_warnings;
    return std::move(top_context.entries);
}

std::string loc_generator::derive_esm_name(const std::string & filename)
{
    auto stem = std::string(string_utils::extract_filename(filename));
    const auto dot_pos = stem.find_last_of('.');
    if (dot_pos != std::string::npos)
        stem = stem.substr(0, dot_pos);

    stem = string_utils::to_lower(stem);

    const std::regex pattern(R"((.+)_[a-z]{2}_[a-z]{2}$)");
    std::smatch match;
    if (std::regex_match(stem, match, pattern))
        return match[1].str();

    return stem;
}

static std::string build_output_path(
    const std::string & output_directory,
    const std::string & filename)
{
    auto path = output_directory;
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
        path += '/';

    return path + filename;
}

loc_generator::generation_result_t loc_generator::generate(
    const generation_input_t & input)
{
    generation_result_t result;
    result.cel_path = build_output_path(
        input.output_directory, input.esm_name + ".cel");
    result.mrk_path = build_output_path(
        input.output_directory, input.esm_name + ".mrk");
    result.top_path = build_output_path(
        input.output_directory, input.esm_name + ".top");

    build_context_t context{input.dict, input.codepage, 0};

    auto cel_entries = build_cel_entries(context);
    auto mrk_entries = build_mrk_entries(context);

    top_input_t top_input{input.hunspell_aff_path, input.hunspell_dic_path};
    auto top_entries = build_top_entries(context, top_input);

    result.cel_entries = static_cast<int>(cel_entries.size());
    result.mrk_entries = static_cast<int>(mrk_entries.size());
    result.top_entries = static_cast<int>(top_entries.size());
    result.skipped_entries = context.skipped_count;
    result.collision_warnings = context.collision_warnings;

    loc_file_writer::write(result.cel_path, cel_entries);
    loc_file_writer::write(result.mrk_path, mrk_entries);
    loc_file_writer::write(result.top_path, top_entries);

    app_logger_t::add_log(
        "[info] loc generation complete: cel=" +
        std::to_string(result.cel_entries) +
        " mrk=" + std::to_string(result.mrk_entries) +
        " top=" + std::to_string(result.top_entries) +
        " skipped=" + std::to_string(result.skipped_entries) +
        " collisions=" + std::to_string(result.collision_warnings) +
        "\r\n");

    return result;
}
