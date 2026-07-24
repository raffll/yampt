#pragma once

#include "model/document.hpp"
#include <io/codepage.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class dict_document_t;

class session_t
{
public:
	explicit session_t(codepage_t codepage);

	document_t * open(const std::string & path);
	document_t * find(const std::string & path);
	const document_t * find(const std::string & path) const;
	void close(const std::string & path);
	void close_if(std::function<bool(const document_t &)> predicate);

	std::vector<document_t *> all();
	std::vector<dict_document_t *> all_dicts();
	std::vector<const dict_document_t *> all_dicts() const;
	std::vector<document_t *> all_dirty();

	void save_all();
	bool has_any_unsaved() const;
	size_t count() const;

	void set_codepage(codepage_t cp);
	codepage_t codepage() const;

	void set_native_language(const std::string & code);
	const std::string & native_language() const;

	size_t dict_version() const;

private:
	document_t * handle_open_plugin(const std::string & normalized);
	document_t * handle_open_dict(const std::string & normalized);
	document_t * handle_open_yaml(const std::string & normalized);
	document_t * handle_open_loc(const std::string & normalized);

	std::vector<std::unique_ptr<document_t>> m_docs;
	codepage_t m_codepage;
	std::string m_native_language;
	size_t m_dict_version = 0;
};
