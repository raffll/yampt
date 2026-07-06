#pragma once

#include <string>
#include <vector>

struct merge_record_t
{
	std::string rec_type;
	std::string record_id;
	std::string content;
	bool pinned = false;
};

class merge_patch_store_t
{
public:
	void clear();
	void add(const std::string & rec_type, const std::string & record_id, const std::string & content);
	void add_pinned(const std::string & rec_type, const std::string & record_id, const std::string & content);
	void remove(const std::string & rec_type, const std::string & record_id);

	void update_or_add(const std::string & rec_type, const std::string & record_id, const std::string & content);
	void update_or_add_pinned(const std::string & rec_type, const std::string & record_id, const std::string & content);

	bool is_pinned(const std::string & rec_type, const std::string & record_id) const;
	const std::string * find_content(const std::string & rec_type, const std::string & record_id) const;

	std::vector<merge_record_t> collect_pinned() const;
	void restore_pinned(const std::vector<merge_record_t> & pinned);

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
};
