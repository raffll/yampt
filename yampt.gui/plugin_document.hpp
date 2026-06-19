#pragma once

#include "document.hpp"
#include <algorithm>
#include <string>

class plugin_document_t : public document_t
{
public:
	explicit plugin_document_t(const std::string & path)
	    : path_(path)
	{
		std::replace(path_.begin(), path_.end(), '\\', '/');
	}

	std::string path() const override
	{
		return path_;
	}

	bool is_dirty() const override
	{
		return false;
	}

	bool is_read_only() const override
	{
		return true;
	}

	std::vector<table_row_t> build_rows() const override
	{
		return {};
	}

	void commit_edit(tools_t::rec_type_t, size_t, const std::string &) override
	{}

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
	std::string path_;
};
