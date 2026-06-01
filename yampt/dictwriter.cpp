#include "dictwriter.hpp"

static std::string escapeJson(const std::string & s)
{
    std::string result;
    result.reserve(s.size() + 16);
    for (unsigned char c : s)
    {
        switch (c)
        {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            default:
                if (c < 0x20)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    result += buf;
                }
                else
                {
                    result += static_cast<char>(c);
                }
                break;
        }
    }
    return result;
}

void DictWriter::write(const tools_t::dict_t & dict, const std::string & path)
{
    tools_t::addLog("--> Writing \"" + path + "\"...\r\n");

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        tools_t::addLog("--> Error: cannot open \"" + path + "\" for writing!\r\n");
        return;
    }

    file << "{\n";
    bool first_chapter = true;

    for (const auto & [type, chapter] : dict)
    {
        if (chapter.empty())
            continue;

        if (!first_chapter)
            file << ",\n";
        first_chapter = false;

        file << "  \"" << tools_t::type2Str(type) << "\": [\n";

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & entry = chapter.records[i];
            file << "    {\n";
            file << "      \"id\": \"" << escapeJson(entry.key_text) << "\",\n";
            file << "      \"old\": \"" << escapeJson(entry.old_text) << "\",\n";
            file << "      \"new\": \"" << escapeJson(entry.new_text) << "\"";
            if (!entry.status.empty())
            {
                file << ",\n";
                file << "      \"status\": \"" << escapeJson(entry.status) << "\"\n";
            }
            else
            {
                file << "\n";
            }
            file << "    }";
            if (i + 1 < chapter.records.size())
                file << ",";
            file << "\n";
        }

        file << "  ]";
    }

    file << "\n}\n";
    file.close();

    tools_t::addLog("--> Done writing \"" + path + "\"\r\n");
}
