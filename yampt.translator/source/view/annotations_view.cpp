#include "annotations_view.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>
#include <set>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

constexpr int column_annotation = 0;
constexpr int column_source = 1;

} // namespace

annotations_view_t::annotations_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	m_tree = new QTreeWidget(this);
	m_tree->setColumnCount(2);
	m_tree->setHeaderLabels({ tr("Annotation"), tr("Source") });
	m_tree->setRootIsDecorated(false);
	m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree->header()->setSectionResizeMode(column_annotation, QHeaderView::Interactive);
	m_tree->header()->setSectionResizeMode(column_source, QHeaderView::Interactive);
	m_tree->header()->setStretchLastSection(true);
	m_tree->header()->setSectionsMovable(false);
	m_tree->setColumnWidth(column_annotation, 220);
	layout->addWidget(m_tree);

	connect(m_tree, &QTreeWidget::itemClicked, this, &annotations_view_t::on_item_clicked);
}

struct annotation_entry_t
{
	std::string old_text;
	std::string new_text;
	std::string source;
};

static std::vector<annotation_entry_t> deduplicate_and_sort(
    const std::vector<annotation_t> & annotations,
    annotation_t::kind_t kind)
{
	std::set<std::string> seen;
	std::vector<annotation_entry_t> result;

	for (const auto & a : annotations)
	{
		if (a.kind != kind)
			continue;

		auto key = a.old_text + "\x01" + a.new_text + "\x01" + a.source;
		if (seen.count(key))
			continue;

		seen.insert(key);
		result.push_back({ a.old_text, a.new_text, a.source });
	}

	std::sort(
	    result.begin(),
	    result.end(),
	    [](const annotation_entry_t & lhs, const annotation_entry_t & rhs) { return lhs.old_text < rhs.old_text; });

	return result;
}

static void add_annotation_section(
    QTreeWidget * tree,
    const std::vector<annotation_entry_t> & entries,
    const QString & header_text,
    const QColor & header_color)
{
	if (entries.empty())
		return;

	auto * header = new QTreeWidgetItem(tree, { header_text });
	header->setForeground(column_annotation, header_color);
	header->setFlags(Qt::NoItemFlags);
	header->setFirstColumnSpanned(true);

	for (const auto & entry : entries)
	{
		const auto annotation = QString::fromStdString(entry.old_text + " \xe2\x86\x92 " + entry.new_text);
		const auto source = entry.source.empty()
		    ? QString()
		    : QString::fromStdString(std::string(string_utils::extract_filename(entry.source)));

		auto * item = new QTreeWidgetItem(tree, { annotation, source });
		item->setData(column_annotation, Qt::UserRole, QString::fromStdString(entry.new_text));
	}
}

void annotations_view_t::update_annotations(
    const std::vector<annotation_t> & annotations,
    const std::string & speaker_name,
    const std::string & gender,
    const std::string & enchantment)
{
	m_tree->clear();

	if (!enchantment.empty())
	{
		auto * item = new QTreeWidgetItem(m_tree, { tr("Enchantment: %1").arg(QString::fromStdString(enchantment)) });
		item->setFirstColumnSpanned(true);
	}

	if (!speaker_name.empty())
	{
		auto * item = new QTreeWidgetItem(
		    m_tree,
		    { tr("Speaker: %1 (%2)").arg(QString::fromStdString(speaker_name), QString::fromStdString(gender)) });
		item->setFirstColumnSpanned(true);
	}

	const auto hyperlinks = deduplicate_and_sort(annotations, annotation_t::dial_topic);
	add_annotation_section(m_tree, hyperlinks, tr("--- Hyperlinks ---"), QColor(70, 130, 200));

	const auto glossary = deduplicate_and_sort(annotations, annotation_t::glossary_term);
	add_annotation_section(m_tree, glossary, tr("--- Glossary ---"), QColor(50, 150, 50));

	const auto inflection = deduplicate_and_sort(annotations, annotation_t::inflection_form);
	add_annotation_section(m_tree, inflection, tr("--- Inflection ---"), QColor(190, 140, 60));
}

void annotations_view_t::on_item_clicked(QTreeWidgetItem * item, int)
{
	if (!item)
		return;

	auto new_text = item->data(column_annotation, Qt::UserRole).toString();
	if (new_text.isEmpty())
		return;

	QApplication::clipboard()->setText(new_text);
	emit annotation_clicked(new_text.toStdString());
}

void annotations_view_t::clear()
{
	m_tree->clear();
}
