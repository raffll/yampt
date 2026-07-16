#include "translation_settings_view.hpp"
#include <settings_store.hpp>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QVBoxLayout>

translation_settings_view_t::translation_settings_view_t(const std::string & providers_dir, QWidget * parent)
    : QWidget(parent)
    , m_providers_dir(providers_dir)
{
	auto * layout = new QVBoxLayout(this);

	auto * description = new QLabel(tr("API keys for web translation providers. "
	                                    "Add provider configs as JSON files in the providers/ folder."), this);
	description->setWordWrap(true);
	layout->addWidget(description);

	m_provider_table = new QTableWidget(this);
	m_provider_table->setColumnCount(3);
	m_provider_table->setHorizontalHeaderLabels({ tr("Provider"), tr("API Key"), tr("Status") });
	m_provider_table->horizontalHeader()->setStretchLastSection(true);
	m_provider_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_provider_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_provider_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_provider_table->verticalHeader()->setVisible(false);
	m_provider_table->setSelectionMode(QAbstractItemView::NoSelection);
	m_provider_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(m_provider_table, 1);

	m_configs = web_translator_config::load_all(m_providers_dir);
	rebuild_table();
}

void translation_settings_view_t::rebuild_table()
{
	m_provider_table->setRowCount(static_cast<int>(m_configs.size()));

	for (int row = 0; row < static_cast<int>(m_configs.size()); ++row)
	{
		const auto & config = m_configs[row];

		auto * name_item = new QTableWidgetItem(QString::fromStdString(config.display_name));
		name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
		m_provider_table->setItem(row, 0, name_item);

		auto * key_edit = new QLineEdit(m_provider_table);
		key_edit->setEchoMode(QLineEdit::Password);
		key_edit->setPlaceholderText(tr("Enter API key..."));
		m_provider_table->setCellWidget(row, 1, key_edit);

		auto * status_item = new QTableWidgetItem(tr("Not configured"));
		status_item->setFlags(status_item->flags() & ~Qt::ItemIsEditable);
		status_item->setForeground(QColor(180, 80, 80));
		m_provider_table->setItem(row, 2, status_item);
	}
}

void translation_settings_view_t::load(const settings_store_t & settings)
{
	for (int row = 0; row < static_cast<int>(m_configs.size()); ++row)
	{
		const auto & config = m_configs[row];
		const auto stored_key = settings.web_api_key(config.identifier);

		auto * key_edit = qobject_cast<QLineEdit *>(m_provider_table->cellWidget(row, 1));
		if (key_edit)
			key_edit->setText(QString::fromStdString(stored_key));

		auto * status_item = m_provider_table->item(row, 2);
		if (status_item)
		{
			if (stored_key.empty())
			{
				status_item->setText(tr("Not configured"));
				status_item->setForeground(QColor(180, 80, 80));
			}
			else
			{
				status_item->setText(tr("Configured"));
				status_item->setForeground(QColor(80, 160, 80));
			}
		}
	}
}

void translation_settings_view_t::apply(settings_store_t & settings) const
{
	for (int row = 0; row < static_cast<int>(m_configs.size()); ++row)
	{
		const auto & config = m_configs[row];

		auto * key_edit = qobject_cast<QLineEdit *>(m_provider_table->cellWidget(row, 1));
		if (!key_edit)
			continue;

		const auto key_value = key_edit->text().toStdString();
		settings.set_web_api_key(config.identifier, key_value);
	}
}
