#include "eet_plugin_dialog.hpp"
#include "../model/sidebar_model.hpp"
#include "../view/display_name.hpp"
#include <utility/domain_types.hpp>
#include <utility/string_utils.hpp>
#include <algorithm>
#include <map>
#include <vector>
#include <QColor>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

eet_plugin_dialog_t::eet_plugin_dialog_t(
    const file_list_t & file_list,
    const std::string & plugin_name,
    QWidget * parent)
    : QDialog(parent)
    , m_file_list(file_list)
{
	setWindowTitle(tr("Select Source Plugin"));
	setModal(true);
	resize(450, 400);

	auto * layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(
	    tr("Select the plugin this translation file was made for:"),
	    this));

	auto * tree = new QTreeWidget(this);
	tree->setHeaderHidden(true);
	tree->setRootIsDecorated(true);
	tree->setIndentation(16);
	layout->addWidget(tree);

	populate_plugin_tree(plugin_name);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Ok)->setToolTip(
	    tr("Build a complete dictionary from the selected plugin"));
	buttons->button(QDialogButtonBox::Cancel)->setToolTip(tr("Cancel the import"));
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
	    [this, tree]()
	{
		auto * selected_item = tree->currentItem();
		if (!selected_item || !(selected_item->flags() & Qt::ItemIsSelectable))
			return;

		m_selected_plugin_path = selected_item->data(0, Qt::UserRole).toString().toStdString();
	});
}

std::optional<std::string> eet_plugin_dialog_t::selected_plugin_path() const
{
	return m_selected_plugin_path;
}

void eet_plugin_dialog_t::populate_plugin_tree(const std::string & plugin_name)
{
	auto * tree = findChild<QTreeWidget *>();

	const auto name_lower = string_utils::to_lower(plugin_name);

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
		const auto root_label = derive_root_label(root_path);

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

				file_path_parts_t parts;
				parts.set_name(plugin->filename);

				if (string_utils::to_lower(parts.name) == name_lower && !best_match_item)
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
