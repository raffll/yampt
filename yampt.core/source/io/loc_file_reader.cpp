#include "io/loc_file_reader.hpp"

#include "utility/string_utils.hpp"

#include <fstream>
#include <stdexcept>

static std::string read_file_bytes(const std::string & path)
{
    std::ifstream stream(path, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error("cannot open file for reading: " + path);

    return std::string(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>());
}

static std::vector<std::string> split_lines(const std::string & text)
{
    std::vector<std::string> lines;
    std::string::size_type start = 0;

    while (start < text.size())
    {
        const auto pos = text.find("\r\n", start);

        if (pos == std::string::npos)
        {
            lines.push_back(text.substr(start));
            break;
        }

        lines.push_back(text.substr(start, pos - start));
        start = pos + 2;
    }

    return lines;
}

static loc_types::loc_entry_t parse_line(const std::string & line)
{
    const auto tab_pos = line.find('\t');

    if (tab_pos == std::string::npos)
        return {line, ""};

    return {line.substr(0, tab_pos), line.substr(tab_pos + 1)};
}

static std::string extract_extension(const std::string & path)
{
    const auto dot_pos = path.find_last_of('.');

    if (dot_pos == std::string::npos)
        return "";

    return string_utils::to_lower(path.substr(dot_pos));
}

loc_file_reader::loc_file_t loc_file_reader::read(const std::string & path, codepage_t codepage)
{
    const auto raw_bytes = read_file_bytes(path);
    const auto utf8_text = decode_to_utf8(raw_bytes, codepage);
    const auto lines = split_lines(utf8_text);

    loc_file_t result;
    result.file_kind = classify_extension(path);

    for (const auto & line : lines)
    {
        if (line.empty())
            continue;

        result.entries.push_back(parse_line(line));
    }

    return result;
}

loc_types::loc_file_kind_t loc_file_reader::classify_extension(const std::string & path)
{
    const auto extension = extract_extension(path);

    if (extension == ".cel")
        return loc_types::loc_file_kind_t::cel;

    if (extension == ".top")
        return loc_types::loc_file_kind_t::top;

    if (extension == ".mrk")
        return loc_types::loc_file_kind_t::mrk;

    return loc_types::loc_file_kind_t::cel;
}
