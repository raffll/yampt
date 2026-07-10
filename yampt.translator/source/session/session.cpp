#include "session.hpp"
#include "../model/dict_document.hpp"
#include "../model/plugin_document.hpp"
#include "../model/yaml_document.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>

session_t::session_t(codepage_t codepage)
    : m_codepage(codepage)
{}

document_t * session_t::open(const std::string & path)
{
	const auto normalized = string_utils::normalize_path(path);

	auto * existing = find(normalized);
	if (existing)
		return existing;

	const auto dot_pos = normalized.rfind('.');
	if (dot_pos == std::string::npos)
		return nullptr;

	auto extension = normalized.substr(dot_pos);
	std::transform(
	    extension.begin(),
	    extension.end(),
	    extension.begin(),
	    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (extension == ".esp" || extension == ".esm")
		return handle_open_plugin(normalized);

	if (extension == ".json" || extension == ".xml")
		return handle_open_dict(normalized);

	if (extension == ".yaml" || extension == ".yml")
		return handle_open_yaml(normalized);

	return nullptr;
}

document_t * session_t::handle_open_plugin(const std::string & normalized)
{
	auto document = std::make_unique<plugin_document_t>(normalized);
	auto * raw_ptr = document.get();
	m_docs.push_back(std::move(document));
	return raw_ptr;
}

document_t * session_t::handle_open_dict(const std::string & normalized)
{
	const auto slash_pos = normalized.rfind('/');
	const auto filename = (slash_pos != std::string::npos) ? normalized.substr(slash_pos + 1) : normalized;

	auto filename_upper = filename;
	std::transform(
	    filename_upper.begin(),
	    filename_upper.end(),
	    filename_upper.begin(),
	    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

	const auto kind = (filename_upper.find("_BASE_") != std::string::npos) ? dict_kind_t::base : dict_kind_t::user;

	auto document = std::make_unique<dict_document_t>(normalized, m_codepage, kind);
	auto * raw_ptr = document.get();
	m_docs.push_back(std::move(document));
	++m_dict_version;
	return raw_ptr;
}

document_t * session_t::handle_open_yaml(const std::string & normalized)
{
	auto native_code = m_native_language.empty() ? "pl" : m_native_language;
	std::transform(
	    native_code.begin(), native_code.end(), native_code.begin(), [](unsigned char c) { return std::tolower(c); });

	auto document = std::make_unique<yaml_document_t>(normalized, native_code);
	auto * raw_ptr = document.get();
	m_docs.push_back(std::move(document));
	return raw_ptr;
}

document_t * session_t::find(const std::string & path)
{
	const auto normalized = string_utils::normalize_path(path);

	for (const auto & doc : m_docs)
	{
		if (doc->path() == normalized)
			return doc.get();
	}

	return nullptr;
}

const document_t * session_t::find(const std::string & path) const
{
	const auto normalized = string_utils::normalize_path(path);

	for (const auto & doc : m_docs)
	{
		if (doc->path() == normalized)
			return doc.get();
	}

	return nullptr;
}

void session_t::close(const std::string & path)
{
	const auto normalized = string_utils::normalize_path(path);

	for (auto it = m_docs.begin(); it != m_docs.end(); ++it)
	{
		if ((*it)->path() != normalized)
			continue;

		if (dynamic_cast<dict_document_t *>(it->get()))
			++m_dict_version;

		m_docs.erase(it);
		return;
	}
}

void session_t::close_if(std::function<bool(const document_t &)> predicate)
{
	for (auto i = static_cast<int>(m_docs.size()) - 1; i >= 0; --i)
	{
		if (!predicate(*m_docs[static_cast<size_t>(i)]))
			continue;

		if (dynamic_cast<dict_document_t *>(m_docs[static_cast<size_t>(i)].get()))
			++m_dict_version;

		m_docs.erase(m_docs.begin() + i);
	}
}

std::vector<document_t *> session_t::all()
{
	std::vector<document_t *> result;
	result.reserve(m_docs.size());

	for (const auto & doc : m_docs)
		result.push_back(doc.get());

	return result;
}

std::vector<dict_document_t *> session_t::all_dicts()
{
	std::vector<dict_document_t *> result;

	for (const auto & doc : m_docs)
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

	for (const auto & doc : m_docs)
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

	for (const auto & doc : m_docs)
	{
		if (doc->is_dirty())
			result.push_back(doc.get());
	}

	return result;
}

void session_t::save_all()
{
	for (const auto & doc : m_docs)
	{
		if (!doc->is_dirty())
			continue;

		doc->save();
	}
}

bool session_t::has_any_unsaved() const
{
	for (const auto & doc : m_docs)
	{
		if (doc->is_dirty())
			return true;
	}

	return false;
}

size_t session_t::count() const
{
	return m_docs.size();
}

void session_t::set_codepage(codepage_t cp)
{
	m_codepage = cp;
}

codepage_t session_t::codepage() const
{
	return m_codepage;
}

void session_t::set_native_language(const std::string & code)
{
	m_native_language = code;
}

const std::string & session_t::native_language() const
{
	return m_native_language;
}

size_t session_t::dict_version() const
{
	return m_dict_version;
}
