#include "session.hpp"
#include "dict_document.hpp"
#include "plugin_document.hpp"
#include "yaml_document.hpp"

#include <algorithm>

session_t::session_t(codepage_t codepage)
    : codepage_(codepage)
{
}

document_t * session_t::open(const std::string & path)
{
    const auto normalized = normalize_path(path);

    auto * existing = find(normalized);
    if (existing)
        return existing;

    const auto dot_pos = normalized.rfind('.');
    if (dot_pos == std::string::npos)
        return nullptr;

    auto ext = normalized.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (ext == ".esp" || ext == ".esm")
    {
        auto doc = std::make_unique<plugin_document_t>(normalized);
        auto * raw = doc.get();
        docs_.push_back(std::move(doc));
        return raw;
    }

    if (ext == ".json" || ext == ".xml")
    {
        const auto slash_pos = normalized.rfind('/');
        const auto filename = (slash_pos != std::string::npos)
            ? normalized.substr(slash_pos + 1)
            : normalized;

        auto filename_upper = filename;
        std::transform(filename_upper.begin(), filename_upper.end(), filename_upper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        const auto kind = (filename_upper.find("_BASE_") != std::string::npos)
            ? dict_kind_t::base
            : dict_kind_t::user;

        auto doc = std::make_unique<dict_document_t>(normalized, codepage_, kind);
        auto * raw = doc.get();
        docs_.push_back(std::move(doc));
        ++dict_version_;
        return raw;
    }

    if (ext == ".yaml" || ext == ".yml")
    {
        auto doc = std::make_unique<yaml_document_t>(normalized);
        auto * raw = doc.get();
        docs_.push_back(std::move(doc));
        return raw;
    }

    return nullptr;
}

document_t * session_t::find(const std::string & path)
{
    const auto normalized = normalize_path(path);

    for (const auto & doc : docs_)
    {
        if (doc->path() == normalized)
            return doc.get();
    }

    return nullptr;
}

const document_t * session_t::find(const std::string & path) const
{
    const auto normalized = normalize_path(path);

    for (const auto & doc : docs_)
    {
        if (doc->path() == normalized)
            return doc.get();
    }

    return nullptr;
}

void session_t::close(const std::string & path)
{
    const auto normalized = normalize_path(path);

    for (auto it = docs_.begin(); it != docs_.end(); ++it)
    {
        if ((*it)->path() != normalized)
            continue;

        if (dynamic_cast<dict_document_t *>(it->get()))
            ++dict_version_;

        docs_.erase(it);
        return;
    }
}

void session_t::close_if(std::function<bool(const document_t &)> predicate)
{
    for (auto i = static_cast<int>(docs_.size()) - 1; i >= 0; --i)
    {
        if (!predicate(*docs_[static_cast<size_t>(i)]))
            continue;

        if (dynamic_cast<dict_document_t *>(docs_[static_cast<size_t>(i)].get()))
            ++dict_version_;

        docs_.erase(docs_.begin() + i);
    }
}

std::vector<document_t *> session_t::all()
{
    std::vector<document_t *> result;
    result.reserve(docs_.size());

    for (const auto & doc : docs_)
        result.push_back(doc.get());

    return result;
}

std::vector<dict_document_t *> session_t::all_dicts()
{
    std::vector<dict_document_t *> result;

    for (const auto & doc : docs_)
    {
        auto * dict_doc = dynamic_cast<dict_document_t *>(doc.get());
        if (dict_doc)
            result.push_back(dict_doc);
    }

    return result;
}

std::vector<const dict_document_t *> session_t::all_dicts() const
{
    std::vector<const dict_document_t *> result;

    for (const auto & doc : docs_)
    {
        const auto * dict_doc = dynamic_cast<const dict_document_t *>(doc.get());
        if (dict_doc)
            result.push_back(dict_doc);
    }

    return result;
}

std::vector<document_t *> session_t::all_dirty()
{
    std::vector<document_t *> result;

    for (const auto & doc : docs_)
    {
        if (doc->is_dirty())
            result.push_back(doc.get());
    }

    return result;
}

void session_t::save_all()
{
    for (const auto & doc : docs_)
    {
        if (doc->is_read_only())
            continue;

        if (!doc->is_dirty())
            continue;

        doc->save();
    }
}

bool session_t::has_any_unsaved() const
{
    for (const auto & doc : docs_)
    {
        if (doc->is_dirty())
            return true;
    }

    return false;
}

size_t session_t::count() const
{
    return docs_.size();
}

void session_t::set_codepage(codepage_t cp)
{
    codepage_ = cp;
}

codepage_t session_t::codepage() const
{
    return codepage_;
}

size_t session_t::dict_version() const
{
    return dict_version_;
}

std::string session_t::normalize_path(const std::string & path)
{
    std::string result = path;

    for (auto & c : result)
    {
        if (c == '\\')
            c = '/';
    }

    return result;
}
