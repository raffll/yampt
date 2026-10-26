#pragma once

class document_t;
class editor_controller_t;
class editor_view_t;
class glossary_t;
class record_table_model_t;
class settings_store_t;
class translation_suggestion_view_t;
struct table_row_t;

struct translation_request_deps_t
{
	translation_suggestion_view_t & translation_suggestion_view;
	editor_view_t & editor_view;
	record_table_model_t & table_model;
	editor_controller_t & editor_controller;
	glossary_t & glossary;
	settings_store_t & settings;
};

class translation_request_controller_t
{
public:
	explicit translation_request_controller_t(translation_request_deps_t deps);

	void translate_current_entry(document_t * active_doc);

private:
	const table_row_t * resolve_translatable_row(document_t * active_doc);
	void dispatch_translation(const table_row_t & row_data);

	translation_request_deps_t m_deps;
};
