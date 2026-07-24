#include "io/loc_file_writer.hpp"

#include <fstream>
#include <stdexcept>

void loc_file_writer::write(const std::string & path, const std::vector<loc_types::loc_entry_t> & entries)
{
    std::ofstream stream(path, std::ios::binary);

    if (!stream.is_open())
        throw std::runtime_error("cannot open file for writing: " + path);

    for (const auto & entry : entries)
        stream << entry.key << '\t' << entry.value << "\r\n";
}
