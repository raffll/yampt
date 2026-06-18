#pragma once

#include "document.hpp"

#include <functional>
#include <string>

struct dict_slot_t;

class dict_document_t : public document_t
{
public:
    using save_fn_t = std::function<void(const std::string & path)>;

    dict_document_t(dict_slot_t * slot, const std::string & slot_path,
                    save_fn_t save_fn, bool read_only);

    std::string path() const override;
    bool is_dirty() const override;
    bool is_read_only() const override;

    std::vector<table_row_t> build_rows() const override;
    void commit_edit(tools_t::rec_type_t type, size_t chapter_index,
                     const std::string & new_text) override;
    void save() override;

    int translated_count() const override;
    int total_count() const override;

    void set_dirty(bool dirty) override;

    const std::string & slot_path() const;
    dict_slot_t * slot();
    const dict_slot_t * slot() const;

private:
    dict_slot_t * slot_ = nullptr;
    std::string slot_path_;
    save_fn_t save_fn_;
    bool read_only_ = false;
};
