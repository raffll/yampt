#pragma once

#include "includes.hpp"

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

		default_val,
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
		std::string status;
		std::string speaker_name;
		std::string gender;
		std::string enchantment;
		std::string adapted_from;
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

	struct status_t
	{
		// base mode
		static constexpr const char * matched = "matched";
		static constexpr const char * fingerprint = "fingerprint";
		static constexpr const char * coords = "coords";
		static constexpr const char * heuristic = "heuristic";
		static constexpr const char * exact = "exact";
		static constexpr const char * info = "info";
		static constexpr const char * wilderness = "wilderness";
		static constexpr const char * region = "region";
		static constexpr const char * missing = "missing";
		static constexpr const char * duplicate = "duplicate";
		static constexpr const char * mismatch = "mismatch";
		static constexpr const char * error = "error";

		// single_with_base mode
		static constexpr const char * translated = "translated";
		static constexpr const char * identical = "identical";
		static constexpr const char * adapted = "adapted";
		static constexpr const char * changed = "changed";
		static constexpr const char * reused = "reused";
		static constexpr const char * untranslated = "untranslated";
		static constexpr const char * ambiguous = "ambiguous";
		static constexpr const char * in_progress = "in_progress";
	};

	struct entry_t
	{
		const std::string key_text;
		std::string val_text;
		const tools_t::rec_type_t type;
		const std::string optional = "";
	};

	struct name_t
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
		return log1;
	}

	static bool has_error()
	{
		return error_flag;
	}

	static void set_debug(bool enabled)
	{
		debug_flag = enabled;
	}

	static bool is_debug()
	{
		return debug_flag;
	}

	static void reset_log();
	static dict_t initialize_dict();
	static std::string type_to_str(tools_t::rec_type_t type);
	static rec_type_t str_to_type(const std::string & str);
	static std::string get_dialog_type(const std::string & content);
	static std::string get_indx(const std::string & content);
	static bool is_fnam(const std::string & rec_id);

private:
	static std::string log1;
	static bool error_flag;
	static bool debug_flag;
};
