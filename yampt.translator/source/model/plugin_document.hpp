#pragma once

#include "document.hpp"
#include <algorithm>
#include <string>

class plugin_document_t : public document_t
{
public:
	explicit plugin_document_t(const std::string & path)
	    : m_path(path)
	{
		std::replace(m_path.begin(), m_path.end(), '\\', '/');
	}

	document_kind_t kind() const override
	{
		return document_kind_t::plugin;
	}

	std::string path() const override
	{
		return m_path;
	}

	bool is_dirty() const override
	{
		return false;
	}

	bool is_read_only() const override
	{
		return true;
	}

	document_permissions_t permissions() const override
	{
		return { false, false, false, false, false };
	}

	std::vector<table_row_t> build_rows() const override
	{
		return {};
	}

	void commit_edit(rec_type_t, size_t, const std::string &) override
	{}

	commit_result_t commit(const table_row_t &, const std::string &, status_t) override
	{
		return {};
	}

	commit_result_t commit_status(const table_row_t &, status_t) override
	{
		return {};
	}

	commit_result_t reset_to_original(const table_row_t &) override
	{
		return {};
	}

	void save() override
	{}

	int translated_count() const override
	{
		return 0;
	}

	int total_count() const override
	{
		return 0;
	}

	void set_dirty(bool) override
	{}

private:
	std::string m_path;
};
