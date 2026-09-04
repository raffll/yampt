#include "merge_patch_store.hpp"
#include <algorithm>

void merge_patch_store_t::clear()
{
	m_records.clear();
}

void merge_patch_store_t::add(const std::string & rec_type, const std::string & record_id, const std::string & content)
{
	m_records.push_back({ rec_type, record_id, content, false });
}

void merge_patch_store_t::add_pinned(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	m_records.push_back({ rec_type, record_id, content, true });
}

void merge_patch_store_t::remove(const std::string & rec_type, const std::string & record_id)
{
	auto it = std::remove_if(
	    m_records.begin(),
	    m_records.end(),
	    [&](const merge_record_t & record) { return record.rec_type == rec_type && record.record_id == record_id; });
	m_records.erase(it, m_records.end());
}

void merge_patch_store_t::update_or_add(
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

void merge_patch_store_t::update_or_add_pinned(
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

bool merge_patch_store_t::is_pinned(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return record.pinned;
	}
	return false;
}

const std::string * merge_patch_store_t::find_content(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return &record.content;
	}
	return nullptr;
}

std::vector<merge_record_t> merge_patch_store_t::collect_pinned() const
{
	std::vector<merge_record_t> pinned;
	for (const auto & record : m_records)
	{
		if (record.pinned)
			pinned.push_back(record);
	}
	return pinned;
}

void merge_patch_store_t::restore_pinned(const std::vector<merge_record_t> & pinned)
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

void merge_patch_store_t::add_lock(const merge_lock_t & lock)
{
	for (auto & existing : m_locks)
	{
		if (existing.same_target(lock))
		{
			existing = lock;
			return;
		}
	}

	m_locks.push_back(lock);
}

void merge_patch_store_t::remove_lock(const merge_lock_t & lock)
{
	auto it = std::remove_if(
	    m_locks.begin(), m_locks.end(), [&](const merge_lock_t & existing) { return existing.same_target(lock); });
	m_locks.erase(it, m_locks.end());
}

bool merge_patch_store_t::has_lock(const merge_lock_t & lock) const
{
	for (const auto & existing : m_locks)
	{
		if (existing.same_target(lock))
			return true;
	}

	return false;
}

std::vector<merge_lock_t> merge_patch_store_t::locks_for(const std::string & rec_type, const std::string & record_id)
    const
{
	std::vector<merge_lock_t> result;
	for (const auto & lock : m_locks)
	{
		if (lock.rec_type == rec_type && lock.record_id == record_id)
			result.push_back(lock);
	}

	return result;
}

void merge_patch_store_t::set_locks(const std::vector<merge_lock_t> & locks)
{
	m_locks = locks;
}
