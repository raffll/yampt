#pragma once

#include "annotation_manager.hpp"
#include "history_manager.hpp"
#include "validation_manager.hpp"
#include "table_row.hpp"

#include <string>
#include <vector>

#include <QString>

class dict_document_t;
class document_t;

struct editor_load_result_t
{
	std::string old_text;
	std::string new_text;
	std::string status;
	std::string speaker_name;
	std::string gender;
	std::string enchantment;
	std::string adapted_from;
	std::vector<annotation_t> annotations;
	bool is_read_only = false;
};

struct commit_result_t
{
	std::string new_text;
	std::string status;
	int propagated_count = 0;
	bool success = false;
};

class editor_controller_t
{
public:
	editor_controller_t(history_manager_t & history, validation_manager_t & validation,
	                    annotation_manager_t & annotations);

	int current_row() const;
	const QString & loaded_text() const;
	bool is_loading() const;
	void set_current_row(int row);
	void set_loaded_text(const QString & text);
	void set_loading(bool loading);

	editor_load_result_t load(document_t & doc, const table_row_t & row);
	commit_result_t commit(dict_document_t & doc, const table_row_t & row, const std::string & new_text);
	int propagate(dict_document_t & doc, const std::string & old_text, const std::string & new_text);

private:
	history_manager_t & history_;
	validation_manager_t & validation_;
	annotation_manager_t & annotations_;

	int current_row_ = -1;
	QString loaded_text_;
	bool loading_record_ = false;
};
