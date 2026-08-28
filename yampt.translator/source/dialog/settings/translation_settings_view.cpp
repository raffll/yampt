#include "translation_settings_view.hpp"
#include <resource_paths.hpp>
#include <settings_store.hpp>
#include <utility/language_config.hpp>
#include <filesystem>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

static bool is_setting_required(const web_translator_config_t & config, const std::string & setting_key)
{
	for (const auto & setting : config.settings)
	{
		if (setting.key == setting_key)
			return setting.required;
	}

	return true;
}

translation_settings_view_t::translation_settings_view_t(
    const std::string & providers_dir,
    const std::string & models_dir,
    QWidget * parent)
    : QWidget(parent)
    , m_providers_dir(providers_dir)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	auto * tabs = new QTabWidget(this);

	auto * models_tab = new QWidget;
	auto * models_layout = new QVBoxLayout(models_tab);
	models_layout->setSpacing(12);
	build_local_models_section(models_layout, models_dir);
	models_layout->addStretch();

	auto * providers_tab = new QWidget;
	auto * providers_layout = new QVBoxLayout(providers_tab);
	providers_layout->setSpacing(12);
	m_configs = web_translator_config::load_all(m_providers_dir);
	for (const auto & config : m_configs)
		build_provider_card(providers_layout, config);
	providers_layout->addStretch();

	auto * examples_tab = new QWidget;
	auto * examples_layout = new QVBoxLayout(examples_tab);
	examples_layout->setSpacing(12);
	build_examples_tab(examples_layout);

	tabs->addTab(models_tab, tr("Local Models"));
	tabs->addTab(providers_tab, tr("Web Providers"));
	tabs->addTab(examples_tab, tr("Examples"));
	layout->addWidget(tabs);
}

void translation_settings_view_t::build_local_models_section(QVBoxLayout * parent, const std::string & models_dir)
{
	namespace fs = std::filesystem;

	std::vector<std::string> model_names;
	if (!models_dir.empty() && fs::is_directory(models_dir))
	{
		for (const auto & entry : fs::directory_iterator(models_dir))
		{
			if (!entry.is_directory())
				continue;

			auto dir_path = entry.path();
			if (fs::exists(dir_path / "sentencepiece.bpe.model") && fs::is_directory(dir_path / "model"))
				model_names.push_back(dir_path.filename().string());
		}
	}

	auto * card = new QFrame(this);
	card->setFrameShape(QFrame::StyledPanel);
	auto * card_layout = new QVBoxLayout(card);
	card_layout->setContentsMargins(8, 6, 8, 6);
	card_layout->setSpacing(4);

	auto * title_label = new QLabel(tr("Local Models"), card);
	auto title_font = title_label->font();
	title_font.setBold(true);
	title_label->setFont(title_font);
	card_layout->addWidget(title_label);

	if (model_names.empty())
	{
		auto * empty_label = new QLabel(tr("No models found"), card);
		empty_label->setStyleSheet("color: rgb(120, 120, 120); font-style: italic;");
		card_layout->addWidget(empty_label);
	}
	else
	{
		const auto languages = language_config::load(resource_paths::languages_file());
		std::string language_list;
		for (const auto & lang : languages)
		{
			if (lang.code == "EN")
				continue;

			if (!language_list.empty())
				language_list += ", ";

			language_list += lang.code;
		}

		for (const auto & model_name : model_names)
		{
			auto * row = new QHBoxLayout;
			row->addWidget(new QLabel(QString::fromStdString(model_name), card));
			row->addStretch();

			auto * lang_label = new QLabel(QString::fromStdString(language_list), card);
			lang_label->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
			row->addWidget(lang_label);

			card_layout->addLayout(row);
		}
	}

	parent->addWidget(card);
}

void translation_settings_view_t::build_provider_card(QVBoxLayout * parent, const web_translator_config_t & config)
{
	auto * card = new QFrame(this);
	card->setFrameShape(QFrame::StyledPanel);
	auto * card_layout = new QVBoxLayout(card);
	card_layout->setContentsMargins(8, 6, 8, 6);
	card_layout->setSpacing(6);

	auto * header_row = new QHBoxLayout;
	auto * title_label = new QLabel(QString::fromStdString(config.display_name), card);
	auto title_font = title_label->font();
	title_font.setBold(true);
	title_label->setFont(title_font);
	header_row->addWidget(title_label);

	header_row->addStretch();

	auto * status_label = new QLabel(card);
	status_label->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
	header_row->addWidget(status_label);
	card_layout->addLayout(header_row);

	provider_card_t card_entry;
	card_entry.identifier = config.identifier;
	card_entry.status_label = status_label;
	m_provider_cards.push_back(card_entry);

	if (config.settings.empty())
	{
		auto * no_config_label = new QLabel(tr("No configuration needed"), card);
		no_config_label->setStyleSheet("color: rgb(120, 120, 120); font-style: italic;");
		card_layout->addWidget(no_config_label);
	}
	else
	{
		auto * form = new QFormLayout;
		form->setSpacing(4);

		for (const auto & setting : config.settings)
		{
			if (setting.type == setting_type_t::choice)
				continue;

			setting_widget_t entry;
			entry.provider_id = config.identifier;
			entry.setting_key = setting.key;
			entry.type = setting.type;

			auto * line_edit = new QLineEdit(card);
			if (setting.type == setting_type_t::password)
			{
				line_edit->setEchoMode(QLineEdit::Password);
				line_edit->setPlaceholderText(tr("Enter key..."));
			}

			entry.widget = line_edit;
			form->addRow(QString::fromStdString(setting.label) + ":", line_edit);

			m_setting_widgets.push_back(entry);
		}

		card_layout->addLayout(form);
	}

	parent->addWidget(card);
}

void translation_settings_view_t::build_examples_tab(QVBoxLayout * parent)
{
	m_examples_empty_label = new QLabel(tr("No examples marked. Right-click a record and choose \"Mark as Example\"."), this);
	m_examples_empty_label->setStyleSheet("color: rgb(120, 120, 120); font-style: italic;");
	m_examples_empty_label->setWordWrap(true);
	parent->addWidget(m_examples_empty_label);

	m_examples_table = new QTableWidget(0, 2, this);
	m_examples_table->setHorizontalHeaderLabels({ tr("Original"), tr("Translation") });
	m_examples_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_examples_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_examples_table->verticalHeader()->setVisible(false);
	m_examples_table->verticalHeader()->setDefaultSectionSize(24);
	m_examples_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_examples_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_examples_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_examples_table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_examples_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	parent->addWidget(m_examples_table, 1);

	auto * button_row = new QHBoxLayout;
	button_row->addStretch();

	m_examples_remove_button = new QPushButton(tr("Remove"), this);
	m_examples_remove_button->setToolTip(tr("Remove the selected example"));
	button_row->addWidget(m_examples_remove_button);
	parent->addLayout(button_row);

	connect(
	    m_examples_remove_button,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		const int row = m_examples_table->currentRow();
		if (row < 0 || row >= static_cast<int>(m_examples.size()))
			return;

		m_examples.erase(m_examples.begin() + row);
		rebuild_examples_table();
	});
}

void translation_settings_view_t::rebuild_examples_table()
{
	if (!m_examples_table || !m_examples_empty_label)
		return;

	const bool has_examples = !m_examples.empty();
	m_examples_empty_label->setVisible(!has_examples);
	m_examples_table->setVisible(has_examples);

	if (m_examples_remove_button)
		m_examples_remove_button->setEnabled(has_examples);

	m_examples_table->setRowCount(static_cast<int>(m_examples.size()));

	for (int row = 0; row < static_cast<int>(m_examples.size()); ++row)
	{
		const auto & example = m_examples[static_cast<size_t>(row)];

		auto * original_item = new QTableWidgetItem(QString::fromStdString(example.original));
		original_item->setFlags(original_item->flags() & ~Qt::ItemIsEditable);
		m_examples_table->setItem(row, 0, original_item);

		auto * translation_item = new QTableWidgetItem(QString::fromStdString(example.translation));
		translation_item->setFlags(translation_item->flags() & ~Qt::ItemIsEditable);
		m_examples_table->setItem(row, 1, translation_item);
	}
}

void translation_settings_view_t::load(const settings_store_t & settings)
{
	for (auto & entry : m_setting_widgets)
	{
		const auto stored = settings.web_provider_setting(entry.provider_id, entry.setting_key);

		if (entry.type == setting_type_t::choice)
		{
			auto * combo = qobject_cast<QComboBox *>(entry.widget);
			if (combo && !stored.empty())
				combo->setCurrentText(QString::fromStdString(stored));
		}
		else
		{
			auto * line_edit = qobject_cast<QLineEdit *>(entry.widget);
			if (line_edit)
				line_edit->setText(QString::fromStdString(stored));
		}
	}

	for (const auto & card : m_provider_cards)
		update_status(card);

	m_examples = settings.translation_examples();
	rebuild_examples_table();
}

void translation_settings_view_t::apply(settings_store_t & settings) const
{
	for (const auto & entry : m_setting_widgets)
	{
		const auto value = read_widget_value(entry);
		settings.set_web_provider_setting(entry.provider_id, entry.setting_key, value);
	}

	settings.set_translation_examples(m_examples);
}

void translation_settings_view_t::update_status(const provider_card_t & card)
{
	const web_translator_config_t * config = nullptr;
	for (const auto & cfg : m_configs)
	{
		if (cfg.identifier == card.identifier)
		{
			config = &cfg;
			break;
		}
	}

	if (!config || !card.status_label)
		return;

	if (config->settings.empty())
	{
		card.status_label->setText(tr("Ready"));
		card.status_label->setStyleSheet("color: rgb(80, 160, 80); font-size: 11px;");
		return;
	}

	bool all_filled = true;
	for (const auto & entry : m_setting_widgets)
	{
		if (entry.provider_id != card.identifier)
			continue;

		if (!is_setting_required(*config, entry.setting_key))
			continue;

		const auto value = read_widget_value(entry);
		if (value.empty())
		{
			all_filled = false;
			break;
		}
	}

	if (all_filled)
	{
		card.status_label->setText(tr("Configured"));
		card.status_label->setStyleSheet("color: rgb(80, 160, 80); font-size: 11px;");
	}
	else
	{
		card.status_label->setText(tr("Not configured"));
		card.status_label->setStyleSheet("color: rgb(180, 80, 80); font-size: 11px;");
	}
}

std::string translation_settings_view_t::read_widget_value(const setting_widget_t & entry) const
{
	if (entry.type == setting_type_t::choice)
	{
		auto * combo = qobject_cast<QComboBox *>(entry.widget);
		return combo ? combo->currentText().toStdString() : std::string {};
	}

	auto * line_edit = qobject_cast<QLineEdit *>(entry.widget);
	return line_edit ? line_edit->text().toStdString() : std::string {};
}
