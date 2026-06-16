#include "dict_workspace.hpp"
#include "../yampt/dict_reader.hpp"
#include "../yampt/dict_writer.hpp"

int dict_workspace_t::load_dict(const std::string & path, dict_kind_t kind)
{
	int existing = find_by_path(path);
	if (existing >= 0)
	{
		set_active(existing);
		return existing;
	}

	dict_reader_t reader(path);
	if (!reader.is_loaded())
		return -1;

	dict_slot_t slot;
	slot.data = reader.get_dict();
	slot.path = path;

	slots_.push_back(std::move(slot));
	kinds_.push_back(kind);

	int new_index = static_cast<int>(slots_.size()) - 1;
	set_active(new_index);
	return new_index;
}

int dict_workspace_t::add_slot(dict_slot_t slot, dict_kind_t kind)
{
	int existing = find_by_path(slot.path);
	if (existing >= 0)
	{
		set_active(existing);
		return existing;
	}

	slots_.push_back(std::move(slot));
	kinds_.push_back(kind);

	int new_index = static_cast<int>(slots_.size()) - 1;
	set_active(new_index);
	return new_index;
}

bool dict_workspace_t::unload_dict(int slot_index)
{
	if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size()))
		return false;

	slots_.erase(slots_.begin() + slot_index);
	kinds_.erase(kinds_.begin() + slot_index);

	if (slots_.empty())
	{
		active_index_ = -1;
		return true;
	}

	if (slot_index < active_index_)
	{
		--active_index_;
	}
	else if (slot_index == active_index_)
	{
		if (active_index_ >= static_cast<int>(slots_.size()))
			active_index_ = static_cast<int>(slots_.size()) - 1;
	}

	return true;
}

void dict_workspace_t::unload_all()
{
	slots_.clear();
	kinds_.clear();
	active_index_ = -1;
}

bool dict_workspace_t::save_dict(int slot_index)
{
	if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size()))
		return false;

	auto & slot = slots_[slot_index];
	dict_writer_t::write(slot.data, slot.path);
	slot.dirty = false;
	slot.modified_records.clear();
	return true;
}

bool dict_workspace_t::save_dict_as(int slot_index, const std::string & path)
{
	if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size()))
		return false;

	auto & slot = slots_[slot_index];
	dict_writer_t::write(slot.data, path);
	slot.path = path;
	slot.dirty = false;
	slot.modified_records.clear();
	return true;
}

void dict_workspace_t::save_all_dirty()
{
	for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
	{
		if (kinds_[i] != dict_kind_t::user)
			continue;
		if (!slots_[i].dirty)
			continue;
		save_dict(i);
	}
}

int dict_workspace_t::find_by_path(const std::string & path) const
{
	for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
	{
		if (slots_[i].path == path)
			return i;
	}
	return -1;
}

void dict_workspace_t::set_active(int slot_index)
{
	if (slot_index < 0 || slot_index >= static_cast<int>(slots_.size()))
	{
		active_index_ = -1;
		return;
	}
	active_index_ = slot_index;
}

int dict_workspace_t::get_active_index() const
{
	return active_index_;
}

dict_slot_t * dict_workspace_t::get_active_slot()
{
	if (active_index_ < 0 || active_index_ >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[active_index_];
}

const dict_slot_t * dict_workspace_t::get_active_slot() const
{
	if (active_index_ < 0 || active_index_ >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[active_index_];
}

dict_slot_t * dict_workspace_t::get_slot(int index)
{
	if (index < 0 || index >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[index];
}

const dict_slot_t * dict_workspace_t::get_slot(int index) const
{
	if (index < 0 || index >= static_cast<int>(slots_.size()))
		return nullptr;
	return &slots_[index];
}

int dict_workspace_t::slot_count() const
{
	return static_cast<int>(slots_.size());
}

bool dict_workspace_t::is_user_slot(int index) const
{
	if (index < 0 || index >= static_cast<int>(kinds_.size()))
		return false;
	return kinds_[index] == dict_kind_t::user;
}

bool dict_workspace_t::is_base_slot(int index) const
{
	if (index < 0 || index >= static_cast<int>(kinds_.size()))
		return false;
	return kinds_[index] == dict_kind_t::base;
}

bool dict_workspace_t::has_any_unsaved() const
{
	for (int i = 0; i < static_cast<int>(slots_.size()); ++i)
	{
		if (kinds_[i] == dict_kind_t::user && slots_[i].dirty)
			return true;
	}
	return false;
}

const std::vector<dict_slot_t> & dict_workspace_t::slots() const
{
	return slots_;
}
