#pragma once

#include "includes.hpp"
#include "status_types.hpp"

struct file_path_parts_t
{
	std::string full;
	std::string name;
	std::string ext;

	void set_name(const std::string & path)
	{
		full = path.substr(path.find_last_of("\\/") + 1);
		name = full.substr(0, full.find_last_of("."));
		ext = full.substr(full.rfind("."));
	}
};

class tools_t
{
public:
	enum class rec_type_t
	{
		cell,
		dial,
		indx,
		rnam,
		desc,
		gmst,
		fnam,
		info,
		text,
		bnam,
		sctx,

		yaml,

		pgrd,
		anam,
		scvr,
		dnam,
		cndt,
		gmdt,

		wild,
		regn,

		unknown,
	};

	enum class creator_mode_t
	{
		base
	};

	struct record_entry_t
	{
		std::string key_text;
		std::string old_text;
		std::string new_text;
		status_t status = status_t::untranslated;
		std::string speaker_name;
		std::string gender;
		std::string enchantment;
		std::string details;
	};

	struct chapter_t
	{
		std::vector<record_entry_t> records;
		std::unordered_map<std::string, size_t> index;
		std::unordered_map<std::string, size_t> old_text_index;

		bool insert(const record_entry_t & entry);
		record_entry_t * find(const std::string & id);
		const record_entry_t * find(const std::string & id) const;
		record_entry_t * find_by_old_text(const std::string & old_text);
		const record_entry_t * find_by_old_text(const std::string & old_text) const;

		size_t size() const
		{
			return records.size();
		}

		bool empty() const
		{
			return records.empty();
		}
	};

	using dict_t = std::map<rec_type_t, chapter_t>;

	struct entry_t
	{
		const std::string key_text;
		std::string old_text;
		const tools_t::rec_type_t type;
		const std::string optional = "";
	};

	struct record_t
	{
		const std::string id;
		std::string content;
		size_t size = 0;
		bool modified = false;
	};

	static const std::vector<std::string> keywords;

	static std::string read_file(const std::string & path);
	static void write_text(const std::string & text, const std::string & name);
	static void write_file(const std::vector<record_t> & records, const std::string & name);
	static void create_file(const std::vector<record_t> & records, const std::string & name);
	static size_t get_number_of_elements_in_dict(const dict_t & dict);
	static size_t convert_string_byte_array_to_uint(const std::string & str);
	static std::string convert_uint_to_string_byte_array(const size_t size);
	static bool case_insensitive_string_cmp(std::string lhs, std::string rhs);
	static std::string erase_null_chars(std::string str);
	static std::string trim_cr(std::string str);
	static std::string replace_non_readable_chars_with_dot(const std::string & str);
	static void add_log(const std::string & entry, const bool silent = false);

	static std::string get_log()
	{
		return m_log;
	}

	static bool has_error()
	{
		return m_error_flag;
	}

	static void set_debug(bool enabled)
	{
		m_debug_flag = enabled;
	}

	static bool is_debug()
	{
		return m_debug_flag;
	}

	static void set_quiet(bool enabled)
	{
		m_quiet_flag = enabled;
	}

	static void set_exe_dir(const std::string & dir)
	{
		m_exe_dir = dir;
	}

	static const std::string & get_exe_dir()
	{
		return m_exe_dir;
	}

	static void reset_log();
	static dict_t initialize_dict();
	static std::string type_to_str(tools_t::rec_type_t type);
	static rec_type_t str_to_type(const std::string & str);
	static std::string get_dialog_type(const std::string & content);
	static std::string get_indx(const std::string & content);
	static bool is_fnam(const std::string & rec_id);

private:
	static std::string m_log;
	static bool m_error_flag;
	static bool m_debug_flag;
	static bool m_quiet_flag;
	static std::string m_exe_dir;
};
