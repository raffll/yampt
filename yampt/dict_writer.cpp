#include "dict_writer.hpp"

static std::string escape_json(const std::string & s)
{
	std::string result;
	result.reserve(s.size() + 16);
	for (unsigned char c : s)
	{
		switch (c)
		{
		case '"':
			result += "\\\"";
			break;
		case '\\':
			result += "\\\\";
			break;
		case '\n':
			result += "\\n";
			break;
		case '\r':
			result += "\\r";
			break;
		case '\t':
			result += "\\t";
			break;
		case '\b':
			result += "\\b";
			break;
		case '\f':
			result += "\\f";
			break;
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

void dict_writer_t::write(const tools_t::dict_t & dict, const std::string & path)
{
	tools_t::add_log("[info] writing \"" + path + "\"\r\n");

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		tools_t::add_log("[error] cannot open \"" + path + "\" for writing\r\n");
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

		file << "  \"" << tools_t::type_to_str(type) << "\": [\n";

		bool is_info = (type == tools_t::rec_type_t::info);
		bool is_fnam = (type == tools_t::rec_type_t::fnam);

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			const auto & entry = chapter.records[i];
			file << "    {\n";
			file << "      \"key\": \"" << escape_json(entry.key_text) << "\",\n";
			file << "      \"old\": \"" << escape_json(entry.old_text) << "\",\n";
			file << "      \"new\": \"" << escape_json(entry.new_text) << "\"";

			file << ",\n";
			file << "      \"status\": \"" << escape_json(entry.status) << "\"";

			if (is_info)
			{
				if (!entry.speaker_name.empty())
				{
					file << ",\n";
					file << "      \"speaker_name\": \"" << escape_json(entry.speaker_name) << "\"";
				}
				if (!entry.gender.empty())
				{
					file << ",\n";
					file << "      \"gender\": \"" << escape_json(entry.gender) << "\"";
				}
			}

			if (is_fnam && !entry.enchantment.empty())
			{
				file << ",\n";
				file << "      \"enchantment\": \"" << escape_json(entry.enchantment) << "\"";
			}

			if ((entry.status == "adapted" || entry.status == "changed" || entry.status == "ambiguous" ||
			     entry.status == "missing" || entry.status == "duplicate") &&
			    !entry.adapted_from.empty())
			{
				file << ",\n";
				file << "      \"adapted_from\": \"" << escape_json(entry.adapted_from) << "\"";
			}

			file << "\n";
			file << "    }";
			if (i + 1 < chapter.records.size())
				file << ",";
			file << "\n";
		}

		file << "  ]";
	}

	file << "\n}\n";
	file.close();

	tools_t::add_log(
	    "[info] done writing \"" + path + "\" (" + std::to_string(tools_t::get_number_of_elements_in_dict(dict)) +
	    " entries)\r\n");
}
