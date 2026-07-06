#include "make_base_dialog.hpp"
#include "../model/sidebar_model.hpp"
#include "../view/display_name.hpp"
#include <utility/app_logger.hpp>
#include <algorithm>
#include <filesystem>
#include <map>
#include <settings_store.hpp>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QTreeWidget>
#include <QVBoxLayout>

make_base_dialog_t::make_base_dialog_t(
    const file_list_t & file_list,
    const settings_store_t & settings,
    const std::string & source_plugin_path,
    QWidget * parent)
    : QDialog(parent)
    , m_file_list(file_list)
    , m_settings(settings)
{
	setWindowTitle("Make Base");
	setModal(true);
	resize(400, 350);

	auto * layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel("Select the native ESM:", this));

	auto * tree = new QTreeWidget(this);
	tree->setHeaderHidden(true);
	tree->setRootIsDecorated(true);
	tree->setIndentation(16);
	layout->addWidget(tree);

	populate_plugin_tree(source_plugin_path);

	auto * mode_group = new QGroupBox("Identical text handling", this);
	auto * mode_layout = new QVBoxLayout(mode_group);
	auto * radio_full = new QRadioButton("Full (identical marked as Translated)", mode_group);
	auto * radio_partial = new QRadioButton("Partial (identical marked as Untranslated)", mode_group);
	radio_full->setChecked(true);
	radio_full->setToolTip("Use to create base dictionary from a fully translated file");
	radio_partial->setToolTip("Use to create base dictionary from a partially translated file");
	mode_layout->addWidget(radio_full);
	mode_layout->addWidget(radio_partial);

	auto * partial_explanation = new QLabel(mode_group);
	partial_explanation->setWordWrap(true);
	partial_explanation->setText(
	    "When old_text equals new_text, the text is tokenized by non-alphanumeric "
	    "characters (tokens shorter than 3 characters are ignored). Each token is "
	    "checked against the Hunspell dictionary:\n"
	    "\u2022 If any token is found \u2192 status = Untranslated (contains source language words)\n"
	    "\u2022 If no tokens are found \u2192 status = To Verify (likely a proper noun)");
	partial_explanation->setStyleSheet("color: #888; margin-left: 20px; margin-top: 4px;");
	partial_explanation->setVisible(false);
	mode_layout->addWidget(partial_explanation);

	auto * dict_label = new QLabel("Source dictionary:", mode_group);
	dict_label->setStyleSheet("margin-left: 20px; margin-top: 4px;");
	dict_label->setVisible(false);
	mode_layout->addWidget(dict_label);

	auto * dict_combo = new QComboBox(mode_group);
	dict_combo->setToolTip("Hunspell dictionary used to detect untranslated words");
	dict_combo->setVisible(false);
	mode_layout->addWidget(dict_combo);

	populate_dictionary_combo();

	const auto partial_aff_setting = m_settings.partial_dict_aff_path();
	if (!partial_aff_setting.empty())
	{
		for (int i = 0; i < dict_combo->count(); ++i)
		{
			if (dict_combo->itemData(i).toString().toStdString() == partial_aff_setting)
			{
				dict_combo->setCurrentIndex(i);
				break;
			}
		}
	}

	connect(radio_partial, &QRadioButton::toggled, partial_explanation, &QLabel::setVisible);
	connect(radio_partial, &QRadioButton::toggled, dict_label, &QLabel::setVisible);
	connect(radio_partial, &QRadioButton::toggled, dict_combo, &QComboBox::setVisible);

	layout->addWidget(mode_group);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(
	    tree,
	    &QTreeWidget::itemDoubleClicked,
	    this,
	    [this](QTreeWidgetItem * item, int)
	{
		if (item && (item->flags() & Qt::ItemIsSelectable))
			accept();
	});

	connect(
	    this,
	    &QDialog::accepted,
	    this,
	    [this, tree, radio_partial, dict_combo, &source_plugin_path]()
	{
		auto * selected_item = tree->currentItem();
		if (!selected_item || !(selected_item->flags() & Qt::ItemIsSelectable))
			return;

		const auto native_path = selected_item->data(0, Qt::UserRole).toString().toStdString();
		const auto * native_entry = m_file_list.get(native_path);

		std::string foreign_lang;
		const auto * foreign_entry = m_file_list.get(source_plugin_path);
		if (foreign_entry)
			foreign_lang = foreign_entry->language_tag;

		std::string native_lang;
		if (native_entry)
			native_lang = native_entry->language_tag;

		make_base_params_t params;
		params.native_path = native_path;
		params.foreign_lang = foreign_lang;
		params.native_lang = native_lang;
		params.base_mode = radio_partial->isChecked() ? base_mode_t::partial : base_mode_t::full;
		if (radio_partial->isChecked() && dict_combo->count() > 0)
			params.dictionary_aff_path = dict_combo->currentData().toString().toStdString();

		m_result = params;
	});
}

std::optional<make_base_params_t> make_base_dialog_t::result() const
{
	return m_result;
}

void make_base_dialog_t::populate_plugin_tree(const std::string & source_plugin_path)
{
	auto * tree = findChild<QTreeWidget *>();

	auto source_sep = source_plugin_path.find_last_of("/\\");
	auto source_filename =
	    source_sep != std::string::npos ? source_plugin_path.substr(source_sep + 1) : source_plugin_path;
	auto source_lower = source_filename;
	std::transform(
	    source_lower.begin(),
	    source_lower.end(),
	    source_lower.begin(),
	    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	struct plugin_item_t
	{
		std::string path;
		std::string display;
		std::string filename;
		std::string root_path;
		std::string subfolder;
	};

	std::vector<plugin_item_t> plugins;
	for (const auto * entry : m_file_list.all())
	{
		if (entry->type != file_type_t::plugin)
			continue;

		if (entry->path == source_plugin_path)
			continue;

		plugins.push_back(
		    { entry->path,
		      derive_display_name(*entry, false, false),
		      entry->filename,
		      entry->root_path,
		      entry->workspace_subfolder });
	}

	if (plugins.empty())
		return;

	std::sort(
	    plugins.begin(),
	    plugins.end(),
	    [](const plugin_item_t & left, const plugin_item_t & right) { return left.filename < right.filename; });

	struct root_builder_t
	{
		std::map<std::string, std::vector<plugin_item_t *>> subfolder_items;
		std::vector<plugin_item_t *> root_items;
	};

	std::map<std::string, root_builder_t> roots;
	for (auto & plugin : plugins)
	{
		auto & builder = roots[plugin.root_path];
		if (plugin.subfolder.empty())
			builder.root_items.push_back(&plugin);
		else
			builder.subfolder_items[plugin.subfolder].push_back(&plugin);
	}

	QTreeWidgetItem * best_match_item = nullptr;

	for (auto & [root_path, builder] : roots)
	{
		auto root_label = root_path;
		auto sep = root_label.find_last_of("/\\");
		if (sep != std::string::npos)
			root_label = root_label.substr(sep + 1);

		if (root_label == "workspace")
			root_label = workspace_label;

		auto * root_node = new QTreeWidgetItem(tree);
		root_node->setText(0, QString::fromStdString(root_label));
		root_node->setFlags(Qt::ItemIsEnabled);
		QFont bold = root_node->font(0);
		bold.setBold(true);
		root_node->setFont(0, bold);

		auto add_items = [&](QTreeWidgetItem * parent, std::vector<plugin_item_t *> & items)
		{
			for (auto * plugin : items)
			{
				auto * item = new QTreeWidgetItem(parent);
				item->setText(0, QString::fromStdString(plugin->display));
				item->setData(0, Qt::UserRole, QString::fromStdString(plugin->path));
				item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				item->setForeground(0, QColor(100, 180, 100));

				auto name_lower = plugin->filename;
				std::transform(
				    name_lower.begin(),
				    name_lower.end(),
				    name_lower.begin(),
				    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				if (name_lower == source_lower && !best_match_item)
					best_match_item = item;
			}
		};

		add_items(root_node, builder.root_items);

		for (auto & [subfolder, items] : builder.subfolder_items)
		{
			auto * folder_node = new QTreeWidgetItem(root_node);
			folder_node->setText(0, QString::fromStdString(subfolder));
			folder_node->setFlags(Qt::ItemIsEnabled);
			folder_node->setForeground(0, QColor(130, 130, 130));
			add_items(folder_node, items);
			folder_node->setExpanded(true);
		}

		root_node->setExpanded(true);
	}

	if (best_match_item)
		tree->setCurrentItem(best_match_item);
}

void make_base_dialog_t::populate_dictionary_combo()
{
	auto * dict_combo = findChild<QComboBox *>();
	if (!dict_combo)
		return;

	auto dict_dir = app_logger_t::get_exe_dir();
	if (!dict_dir.empty() && dict_dir.back() != '/' && dict_dir.back() != '\\')
		dict_dir += '/';
	dict_dir += "dictionaries";

	const std::filesystem::path dict_path(dict_dir);
	if (!std::filesystem::exists(dict_path))
		return;

	for (const auto & entry : std::filesystem::directory_iterator(dict_path))
	{
		if (!entry.is_regular_file())
			continue;

		if (entry.path().extension().string() != ".aff")
			continue;

		auto dic_file = entry.path();
		dic_file.replace_extension(".dic");
		if (!std::filesystem::exists(dic_file))
			continue;

		const auto stem = entry.path().stem().string();
		const auto aff_path = entry.path().string();
		dict_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
	}
}
