#include "find_replace_service.hpp"

#include <QString>

find_replace_service_t::find_replace_service_t(record_table_model_t & model, document_t *& active_doc)
    : model_(model)
    , active_doc_(active_doc)
{
}

find_replace_service_t::find_result_t find_replace_service_t::find_next(
    const std::string & query, bool case_sensitive, bool regex_mode, int current_row)
{
    if (query.empty())
        return {};

    const int count = model_.rowCount();
    if (count == 0)
        return {};

    std::optional<std::regex> rx;
    if (regex_mode)
    {
        auto flags = std::regex_constants::ECMAScript;
        if (!case_sensitive)
            flags |= std::regex_constants::icase;

        try { rx.emplace(query, flags); }
        catch (...) { return {}; }
    }

    const auto q = case_sensitive
        ? QString::fromStdString(query)
        : QString::fromStdString(query).toLower();

    for (int i = 1; i <= count; ++i)
    {
        const int row = (current_row + i) % count;
        const auto * data = model_.row_at(row);
        if (!data)
            continue;

        const auto & text = data->new_text;
        bool found = false;

        if (rx)
        {
            found = std::regex_search(text, *rx);
        }
        else
        {
            auto haystack = QString::fromStdString(text);
            if (!case_sensitive)
                haystack = haystack.toLower();

            found = haystack.contains(q);
        }

        if (found)
            return {row, true};
    }

    return {};
}

find_replace_service_t::replace_result_t find_replace_service_t::replace_current(
    const std::string & query, const std::string & replacement,
    bool case_sensitive, bool regex_mode, int current_row)
{
    if (query.empty())
        return {};

    if (current_row < 0)
        return {};

    auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
    if (!dict_doc)
        return {};

    const auto * row_data = model_.row_at(current_row);
    if (!row_data)
        return {};

    auto & data = dict_doc->data_mut();
    auto it = data.find(row_data->type);
    if (it == data.end())
        return {};

    if (row_data->record_index >= it->second.records.size())
        return {};

    auto & entry = it->second.records[row_data->record_index];
    std::string result;

    if (regex_mode)
    {
        auto flags = std::regex_constants::ECMAScript;
        if (!case_sensitive)
            flags |= std::regex_constants::icase;

        try
        {
            std::regex rx(query, flags);
            result = std::regex_replace(entry.new_text, rx, replacement);
        }
        catch (...) { return {}; }
    }
    else
    {
        auto q_new = QString::fromStdString(entry.new_text);
        const auto q_query = QString::fromStdString(query);
        auto idx = q_new.indexOf(q_query, 0, case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if (idx < 0)
            return {};

        q_new.replace(idx, q_query.length(), QString::fromStdString(replacement));
        result = q_new.toStdString();
    }

    if (result == entry.new_text)
        return {};

    entry.new_text = result;
    entry.status = "in_progress";
    dict_doc->set_dirty(true);
    dict_doc->modified_records_insert(row_data->type, row_data->record_index);

    return {true, entry.new_text, entry.status};
}

find_replace_service_t::replace_all_result_t find_replace_service_t::replace_all(
    const std::string & query, const std::string & replacement,
    bool case_sensitive, bool regex_mode)
{
    if (query.empty())
        return {};

    auto * dict_doc = dynamic_cast<dict_document_t *>(active_doc_);
    if (!dict_doc)
        return {};

    int replaced_count = 0;

    std::optional<std::regex> rx;
    if (regex_mode)
    {
        auto flags = std::regex_constants::ECMAScript;
        if (!case_sensitive)
            flags |= std::regex_constants::icase;

        try { rx.emplace(query, flags); }
        catch (...) { return {}; }
    }

    for (auto & [type, chapter] : dict_doc->data_mut())
    {
        for (size_t idx = 0; idx < chapter.records.size(); ++idx)
        {
            auto & entry = chapter.records[idx];
            std::string result;

            if (rx)
            {
                result = std::regex_replace(entry.new_text, *rx, replacement);
            }
            else
            {
                auto q_text = QString::fromStdString(entry.new_text);
                const auto q_query = QString::fromStdString(query);
                const auto q_replacement = QString::fromStdString(replacement);
                int pos = 0;
                bool changed = false;

                while (true)
                {
                    int found = q_text.indexOf(q_query, pos, case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    if (found < 0)
                        break;

                    q_text.replace(found, q_query.length(), q_replacement);
                    pos = found + q_replacement.length();
                    changed = true;
                }

                if (!changed)
                    continue;

                result = q_text.toStdString();
            }

            if (result == entry.new_text)
                continue;

            entry.new_text = result;
            entry.status = "in_progress";
            dict_doc->modified_records_insert(type, idx);
            ++replaced_count;
        }
    }

    if (replaced_count > 0)
        dict_doc->set_dirty(true);

    return {replaced_count};
}
