#pragma once

#include "../controller/editor_controller.hpp"
#include "../model/dict_document.hpp"
#include "../model/record_table_model.hpp"
#include <utility/domain_types.hpp>
#include <functional>
#include <QList>

struct shortcuts_deps_t
{
	editor_controller_t & editor_controller;
	record_table_model_t & table_model;
	std::function<document_t *()> active_document;
	std::function<void(bool)> set_unsaved_changes;
	std::function<void()> update_status_counts;
	std::function<void(int)> load_record;
	std::function<QList<int>()> selected_rows;
};

class shortcuts_controller_t
{
public:
	explicit shortcuts_controller_t(shortcuts_deps_t deps);

	void copy_original();
	void commit_status(status_t new_status);
	void reset_to_original(const QList<int> & rows);

private:
	QList<int> target_rows() const;
	bool copy_original_row(int row);
	bool commit_status_row(int row, status_t new_status);
	bool reset_to_original_row(int row);

	shortcuts_deps_t m_deps;
};
