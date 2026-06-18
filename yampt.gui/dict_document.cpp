#include "dict_document.hpp"
#include "dict_workspace.hpp"

#include <algorithm>

dict_document_t::dict_document_t(dict_slot_t * slot, const std::string & slot_path,
                                 save_fn_t save_fn, bool read_only)
    : slot_(slot)
    , slot_path_(slot_path)
    , save_fn_(std::move(save_fn))
    , read_only_(read_only)
{
    std::replace(slot_path_.begin(), slot_path_.end(), '\\', '/');
}

std::string dict_document_t::path() const
{
    return slot_path_;
}

bool dict_document_t::is_dirty() const
{
    if (!slot_)
        return false;

    return slot_->dirty;
}

bool dict_document_t::is_read_only() const
{
    return read_only_;
}

std::vector<table_row_t> dict_document_t::build_rows() const
{
    std::vector<table_row_t> rows;
    if (!slot_)
        return rows;

    for (const auto & [type, chapter] : slot_->data)
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & rec = chapter.records[i];
            table_row_t row;
            row.type = type;
            row.key_text = rec.key_text;
            row.old_text = rec.old_text;
            row.new_text = rec.new_text;
            row.status = rec.status;
            row.chapter_index = i;
            rows.push_back(std::move(row));
        }
    }

    return rows;
}

void dict_document_t::commit_edit(tools_t::rec_type_t type, size_t chapter_index,
                                  const std::string & new_text)
{
    if (read_only_)
        return;

    if (!slot_)
        return;

    auto it = slot_->data.find(type);
    if (it == slot_->data.end())
        return;

    if (chapter_index >= it->second.records.size())
        return;

    it->second.records[chapter_index].new_text = new_text;
    slot_->dirty = true;
}

void dict_document_t::save()
{
    if (save_fn_)
        save_fn_(slot_path_);
}

int dict_document_t::translated_count() const
{
    if (!slot_)
        return 0;

    int count = 0;
    for (const auto & [type, chapter] : slot_->data)
    {
        for (const auto & rec : chapter.records)
        {
            if (!rec.new_text.empty())
                ++count;
        }
    }

    return count;
}

int dict_document_t::total_count() const
{
    if (!slot_)
        return 0;

    int count = 0;
    for (const auto & [type, chapter] : slot_->data)
        count += static_cast<int>(chapter.records.size());

    return count;
}

void dict_document_t::set_dirty(bool dirty)
{
    if (!slot_)
        return;

    slot_->dirty = dirty;
}

const std::string & dict_document_t::slot_path() const
{
    return slot_path_;
}

dict_slot_t * dict_document_t::slot()
{
    return slot_;
}

const dict_slot_t * dict_document_t::slot() const
{
    return slot_;
}
