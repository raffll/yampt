#pragma once

#include "includes.hpp"
#include "tools.hpp"
#include "esmreader.hpp"

class dict_creator_t
{
public:
    const auto & get_name() { return esm.getName(); }
    const auto & get_dict() const { return dict; }

    dict_creator_t(
        const std::string & plugin_path,
        const tools_t::dict_t * base_dict = nullptr);

    dict_creator_t(
        const std::string & path,
        const std::string & path_ext);

private:
    struct pattern_t
    {
        std::string str;
        size_t pos;
        bool missing;
    };

    using patterns_ext_t = std::vector<pattern_t>;
    using patterns = std::map<std::string, size_t>;

    struct ids
    {
        const std::string & rec_id;
        const std::string & key_id;
        const std::string & val_id;
        const tools_t::rec_type_t type;
    };

    void make_dict();
    void build_indexes();
    void reset_counters();
    std::string translate_dialog_topic(std::string to_translate);
    void insert_entry(const std::string & id, const std::string & old_text,
                     const std::string & new_text, tools_t::rec_type_t type);
    std::vector<std::string> make_script_messages(const std::string & script_text);
    void print_log_line(const tools_t::rec_type_t type);

    void make_dict_CELL();
    void make_dict_CELL_default();
    void make_dict_CELL_REGN();
    void make_dict_GMST();
    void make_dict_FNAM();
    void make_dict_DESC();
    void make_dict_TEXT();
    void make_dict_RNAM();
    void make_dict_INDX();
    void make_dict_DIAL();
    void make_dict_FLAG();
    void make_dict_INFO();
    void make_dict_script(const ids & ids);
    void make_dict_FNAM_glossary();

    void make_dict_CELL_Unordered();
    void make_dict_CELL_Unordered_Default();
    void make_dict_CELL_Unordered_REGN();
    patterns_ext_t make_dict_CELL_Unordered_PatternsExt();
    patterns make_dict_CELL_Unordered_Patterns();
    std::string make_dict_CELL_Unordered_Pattern(esm_reader_t & esm_cur);
    void make_dict_CELL_Unordered_AddMissing(const patterns_ext_t & patterns_ext);

    void make_dict_DIAL_Unordered();
    patterns_ext_t make_dict_DIAL_Unordered_PatternsExt();
    patterns make_dict_DIAL_Unordered_Patterns();
    std::string make_dict_DIAL_Unordered_Pattern(esm_reader_t & esm_cur, size_t i);
    void make_dict_DIAL_Unordered_AddMissing(const patterns_ext_t & patterns_ext);

    void make_dict_Script_Unordered(const ids & ids);
    patterns_ext_t make_dict__Unordered_PatternsExt(const ids & ids);
    patterns make_dict__Unordered_Patterns(const ids & ids);

    esm_reader_t esm;
    esm_reader_t esm_ext;
    esm_reader_t & esm_ref;
    const tools_t::dict_t * base_dict = nullptr;
    tools_t::dict_t dict;
    bool is_make_mode = false;

    int counter_created = 0;
    int counter_missing = 0;
    int counter_doubled = 0;
    int counter_identical = 0;
    int counter_all = 0;

    std::unordered_map<std::string, size_t> gmst_index;
    std::unordered_map<std::string, size_t> fnam_index;
    std::unordered_map<std::string, size_t> desc_index;
    std::unordered_map<std::string, size_t> text_index;
    std::unordered_map<std::string, size_t> rnam_index;
    std::unordered_map<std::string, size_t> indx_index;
    std::unordered_map<std::string, size_t> flag_index;
    std::unordered_map<std::string, size_t> info_index;
};
