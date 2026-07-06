#pragma once

#include "../controller/editor_controller.hpp"
#include "../model/dict_document.hpp"
#include "../model/record_table_model.hpp"
#include <utility/domain_types.hpp>
#include <functional>

struct shortcuts_deps_t
{
	editor_controller_t & editor_controller;
	record_table_model_t & table_model;
	std::function<document_t *()> active_document;
	std::function<void(bool)> set_unsaved_changes;
	std::function<void()> update_status_counts;
	std::function<void(int)> load_record;
};

class shortcuts_controller_t
{
public:
	explicit shortcuts_controller_t(shortcuts_deps_t deps);

	void copy_original();
	void commit_status(status_t new_status);

private:
	shortcuts_deps_t m_deps;
};
