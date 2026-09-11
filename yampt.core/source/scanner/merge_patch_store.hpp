#pragma once

#include <string>
#include <vector>

struct merge_record_t
{
	std::string rec_type;
	std::string record_id;
	std::string content;
};

enum class lock_scope_t
{
	whole_record,
	sub_record,
	field,
	bit,
	group
};

namespace merge_lock_scope {

inline constexpr int min_value = static_cast<int>(lock_scope_t::whole_record);
inline constexpr int max_value = static_cast<int>(lock_scope_t::group);

inline bool is_valid_value(int value)
{
	return value >= min_value && value <= max_value;
}

inline bool scope_from_value(int value, lock_scope_t & out_scope)
{
	if (!is_valid_value(value))
		return false;

	out_scope = static_cast<lock_scope_t>(value);
	return true;
}

} // namespace merge_lock_scope

struct merge_lock_t
{
	std::string rec_type;
	std::string record_id;
	lock_scope_t scope = lock_scope_t::whole_record;
	std::string sub_type;
	int occurrence = 0;
	int field_index = -1;
	int bit_index = -1;
	size_t sub_size = 0;
	int group_start = -1;
	int group_end = -1;
	std::vector<std::pair<std::string, int>> group_members;
	std::string frozen_content;

	bool same_target(const merge_lock_t & other) const
	{
		return rec_type == other.rec_type && record_id == other.record_id && scope == other.scope &&
		       sub_type == other.sub_type && occurrence == other.occurrence && field_index == other.field_index &&
		       bit_index == other.bit_index && group_start == other.group_start && group_end == other.group_end;
	}
};

class merge_patch_store_t
{
public:
	void clear();
	void add(const std::string & rec_type, const std::string & record_id, const std::string & content);
	void remove(const std::string & rec_type, const std::string & record_id);

	void update_or_add(const std::string & rec_type, const std::string & record_id, const std::string & content);

	const std::string * find_content(const std::string & rec_type, const std::string & record_id) const;

	void add_lock(const merge_lock_t & lock);
	void remove_lock(const merge_lock_t & lock);
	bool has_lock(const merge_lock_t & lock) const;

	const std::vector<merge_lock_t> & locks() const
	{
		return m_locks;
	}

	std::vector<merge_lock_t> locks_for(const std::string & rec_type, const std::string & record_id) const;
	void set_locks(const std::vector<merge_lock_t> & locks);

	bool empty() const
	{
		return m_records.empty();
	}

	size_t count() const
	{
		return m_records.size();
	}

	const std::string & record_content(size_t index) const
	{
		return m_records[index].content;
	}

	const std::string & record_type(size_t index) const
	{
		return m_records[index].rec_type;
	}

	const std::string & record_id(size_t index) const
	{
		return m_records[index].record_id;
	}

	const std::vector<merge_record_t> & records() const
	{
		return m_records;
	}

private:
	std::vector<merge_record_t> m_records;
	std::vector<merge_lock_t> m_locks;
};
