#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.translator/source/controller/editor_controller.hpp"
#include "../../yampt.translator/source/editor/glossary.hpp"
#include "../../yampt.translator/source/model/dict_document.hpp"
#include "../../yampt.translator/source/model/record_table_model.hpp"
#include "../../yampt.translator/source/model/table_columns.hpp"
#include "../../yampt.translator/source/model/table_row.hpp"
#include <io/dict_writer.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <QModelIndex>
#include <QString>

namespace {

std::string write_propagated_group_dict()
{
	dict_t data;
	auto & chapter = data[rec_type_t::cell];

	record_entry_t entry_source;
	entry_source.key_text = "key_a";
	entry_source.old_text = "Old Cell";
	entry_source.new_text = "First Value";
	entry_source.status = status_t::propagated;
	chapter.records.push_back(std::move(entry_source));

	record_entry_t entry_sibling;
	entry_sibling.key_text = "key_b";
	entry_sibling.old_text = "Old Cell";
	entry_sibling.new_text = "First Value";
	entry_sibling.status = status_t::propagated;
	chapter.records.push_back(std::move(entry_sibling));

	record_entry_t entry_translated;
	entry_translated.key_text = "key_c";
	entry_translated.old_text = "Old Cell";
	entry_translated.new_text = "Manual Translation";
	entry_translated.status = status_t::translated;
	chapter.records.push_back(std::move(entry_translated));

	namespace fs = std::filesystem;
	auto path = (fs::temp_directory_path() / "yampt_gui_repropagate.json").string();
	path = string_utils::normalize_path(path);

	app_logger_t::reset_log();
	dict_writer_t::write(data, path);
	app_logger_t::reset_log();
	return path;
}

QString translation_cell_text(const record_table_model_t & model, int row)
{
	const auto translation_column = model.columns().position_of(col_translation);
	const auto cell_index = model.index(row, translation_column);
	return model.data(cell_index, Qt::DisplayRole).toString();
}

} // namespace

TEST_CASE("record_table_model_t re-propagation reflects new text in sibling cells", "[gui]")
{
	const auto path = write_propagated_group_dict();
	dict_document_t doc(path, codepage_t::windows_1252, dict_kind_t::user);

	record_table_model_t model;
	model.set_columns(document_kind_t::dict);
	model.rebuild(doc.build_rows());

	REQUIRE(model.rowCount() == 3);

	glossary_t annotations;
	editor_controller_t controller(annotations);

	const auto * selected_row = model.row_at(0);
	REQUIRE(selected_row != nullptr);
	controller.set_current_row(0);

	table_row_t commit_target = *selected_row;
	const auto result = doc.commit(commit_target, "Second Value", status_t::in_progress);

	REQUIRE(result.success);
	REQUIRE(result.propagated_count == 1);

	controller.sync_propagated_rows(model, doc);

	REQUIRE(translation_cell_text(model, 1).toStdString() == "Second Value");
	REQUIRE(model.row_at(1)->status == status_t::propagated);

	REQUIRE(translation_cell_text(model, 2).toStdString() == "Manual Translation");
	REQUIRE(model.row_at(2)->status == status_t::translated);

	std::filesystem::remove(path);
}
