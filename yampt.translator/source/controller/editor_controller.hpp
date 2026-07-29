#pragma once

#include "../editor/glossary.hpp"
#include "../model/document.hpp"
#include "../model/table_row.hpp"
#include <optional>
#include <string>
#include <vector>
#include <QString>

class dict_document_t;
class document_t;
class record_table_model_t;

struct editor_load_result_t
{
	std::string old_text;
	std::string new_text;
	status_t status = status_t::untranslated;
	std::string speaker_name;
	std::string gender;
	std::string enchantment;
	std::string details;
	std::vector<annotation_t> annotations;
	bool is_read_only = false;
};

class editor_controller_t
{
public:
	explicit editor_controller_t(glossary_t & annotations);

	int current_row() const;
	const QString & loaded_text() const;
	bool is_loading() const;
	void set_current_row(int row);
	void set_loaded_text(const QString & text);
	void set_loading(bool loading);

	editor_load_result_t load(document_t & doc, const table_row_t & row);

	void sync_propagated_rows(record_table_model_t & model, dict_document_t & doc);

	void set_pending_status(status_t status);
	std::optional<status_t> take_pending_status();

private:
	glossary_t & m_annotations;

	int m_current_row = -1;
	QString m_loaded_text;
	bool m_loading_record = false;
	std::optional<status_t> m_pending_status;
};
