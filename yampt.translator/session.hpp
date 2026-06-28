#pragma once

#include "model/document.hpp"
#include "../yampt/io/codepage.hpp"
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

	size_t dict_version() const;

private:
	document_t * handle_open_plugin(const std::string & normalized);
	document_t * handle_open_dict(const std::string & normalized);
	document_t * handle_open_yaml(const std::string & normalized);

	static std::string normalize_path(const std::string & path);

	std::vector<std::unique_ptr<document_t>> docs_;
	codepage_t codepage_;
	size_t dict_version_ = 0;
};
