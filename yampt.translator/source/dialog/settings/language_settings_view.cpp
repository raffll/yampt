#include <resource_paths.hpp>
#include "language_settings_view.hpp"
#include <io/codepage.hpp>
#include <utility/language_config.hpp>
#include <algorithm>
#include <filesystem>
#include <settings_store.hpp>
#include <QComboBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace {

const std::vector<language_entry_t> & get_languages()
{
	static const auto languages =
	    language_config::load(resource_paths::languages_file());
	return languages;
}

} // namespace

language_settings_view_t::language_settings_view_t(const std::string & dictionaries_dir, QWidget * parent)
    : QWidget(parent)
    , m_dictionaries_dir(dictionaries_dir)
{
	auto * layout = new QVBoxLayout(this);

	auto * language_form = new QFormLayout;
	m_foreign_language_combo = new QComboBox(this);
	m_native_language_combo = new QComboBox(this);

	auto sorted_languages = get_languages();
	std::sort(
	    sorted_languages.begin(),
	    sorted_languages.end(),
	    [](const language_entry_t & first, const language_entry_t & second)
	{ return first.display_name < second.display_name; });

	for (const auto & lang : sorted_languages)
	{
		const auto display = QString::fromStdString(lang.display_name);
		const auto code = QString::fromStdString(lang.code);
		m_foreign_language_combo->addItem(display, code);
		m_native_language_combo->addItem(display, code);
	}

	language_form->addRow(tr("Foreign Language:"), m_foreign_language_combo);
	language_form->addRow(tr("Native Language:"), m_native_language_combo);
	layout->addLayout(language_form);

	auto * separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(separator);

	auto * detail_form = new QFormLayout;

	m_foreign_spell_combo = new QComboBox(this);
	m_native_spell_combo = new QComboBox(this);
	detail_form->addRow(tr("Foreign Spell Check:"), m_foreign_spell_combo);
	detail_form->addRow(tr("Native Spell Check:"), m_native_spell_combo);

	m_foreign_tag_edit = new QLineEdit(this);
	m_foreign_tag_edit->setMaxLength(5);
	m_native_tag_edit = new QLineEdit(this);
	m_native_tag_edit->setMaxLength(5);
	detail_form->addRow(tr("Foreign Tag:"), m_foreign_tag_edit);
	detail_form->addRow(tr("Native Tag:"), m_native_tag_edit);

	layout->addLayout(detail_form);

	auto * encoding_separator = new QFrame(this);
	encoding_separator->setFrameShape(QFrame::HLine);
	encoding_separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(encoding_separator);

	auto * encoding_form = new QFormLayout;
	m_encoding_note = new QLabel(this);
	encoding_form->addRow(tr("Encoding:"), m_encoding_note);
	layout->addLayout(encoding_form);

	layout->addStretch();

	scan_dictionaries(m_dictionaries_dir);

	connect(
	    m_foreign_language_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    &language_settings_view_t::on_foreign_language_changed);

	connect(
	    m_native_language_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    &language_settings_view_t::on_native_language_changed);
}

void language_settings_view_t::load(const settings_store_t & settings)
{
	const auto foreign_code = QString::fromStdString(settings.foreign_language());
	const auto native_code = QString::fromStdString(settings.native_language());

	const int foreign_index = m_foreign_language_combo->findData(foreign_code);
	if (foreign_index >= 0)
		m_foreign_language_combo->setCurrentIndex(foreign_index);

	const int native_index = m_native_language_combo->findData(native_code);
	if (native_index >= 0)
		m_native_language_combo->setCurrentIndex(native_index);

	on_foreign_language_changed(m_foreign_language_combo->currentIndex());
	on_native_language_changed(m_native_language_combo->currentIndex());

	const auto foreign_tag = QString::fromStdString(settings.foreign_tag());
	if (!foreign_tag.isEmpty())
		m_foreign_tag_edit->setText(foreign_tag);

	const auto native_tag = QString::fromStdString(settings.native_tag());
	if (!native_tag.isEmpty())
		m_native_tag_edit->setText(native_tag);
}

void language_settings_view_t::apply(settings_store_t & settings) const
{
	const auto foreign_code = m_foreign_language_combo->currentData().toString();
	const auto native_code = m_native_language_combo->currentData().toString();

	settings.set_foreign_language(foreign_code.toStdString());
	settings.set_native_language(native_code.toStdString());

	const auto * native_lang = language_config::find_by_code(get_languages(), native_code.toStdString());
	if (native_lang)
	{
		settings.set_encoding_index(codepage_to_index(native_lang->codepage));
		settings.set_translation_target(native_lang->nllb_code);

		int language_index = 0;
		const auto foreign = foreign_code.toStdString();
		for (const auto & lang : get_languages())
		{
			if (lang.code == foreign)
				continue;

			if (lang.code == native_code.toStdString())
			{
				settings.set_translation_language_index(language_index);
				break;
			}

			++language_index;
		}
	}

	const auto native_spell_aff = m_native_spell_combo->currentData().toString().toStdString();
	settings.set_spell_aff_path(native_spell_aff);

	if (!native_spell_aff.empty())
	{
		auto dic_path = native_spell_aff;
		auto dot_pos = dic_path.rfind(".aff");
		if (dot_pos != std::string::npos)
			dic_path.replace(dot_pos, 4, ".dic");
		settings.set_spell_dic_path(dic_path);
	}
	else
	{
		settings.set_spell_dic_path("");
	}

	const auto foreign_spell_aff = m_foreign_spell_combo->currentData().toString().toStdString();
	settings.set_partial_dict_aff_path(foreign_spell_aff);

	if (!foreign_spell_aff.empty())
	{
		auto dic_path = foreign_spell_aff;
		auto dot_pos = dic_path.rfind(".aff");
		if (dot_pos != std::string::npos)
			dic_path.replace(dot_pos, 4, ".dic");
		settings.set_partial_dict_dic_path(dic_path);
	}
	else
	{
		settings.set_partial_dict_dic_path("");
	}

	settings.set_foreign_tag(m_foreign_tag_edit->text().toStdString());
	settings.set_native_tag(m_native_tag_edit->text().toStdString());
}

void language_settings_view_t::on_foreign_language_changed(int index)
{
	const auto code = m_foreign_language_combo->itemData(index).toString();
	const auto * lang = language_config::find_by_code(get_languages(), code.toStdString());
	if (!lang)
		return;

	update_spell_combo(m_foreign_spell_combo, lang->dictionary_prefix);
	m_foreign_tag_edit->setText(code);
}

void language_settings_view_t::on_native_language_changed(int index)
{
	const auto code = m_native_language_combo->itemData(index).toString();
	const auto * lang = language_config::find_by_code(get_languages(), code.toStdString());
	if (!lang)
		return;

	update_spell_combo(m_native_spell_combo, lang->dictionary_prefix);
	m_native_tag_edit->setText(code);

	if (m_encoding_note)
		m_encoding_note->setText(QString::fromStdString(codepage_name(lang->codepage)));
}

void language_settings_view_t::update_spell_combo(QComboBox * combo, const std::string & prefix)
{
	const auto target = QString::fromStdString(prefix);

	for (int i = 0; i < combo->count(); ++i)
	{
		if (combo->itemText(i) == target)
		{
			combo->setCurrentIndex(i);
			return;
		}
	}

	combo->setCurrentIndex(0);
}

void language_settings_view_t::scan_dictionaries(const std::string & directory)
{
	m_foreign_spell_combo->clear();
	m_native_spell_combo->clear();

	m_foreign_spell_combo->addItem(tr("None"), QString(""));
	m_native_spell_combo->addItem(tr("None"), QString(""));

	if (directory.empty())
		return;

	const std::filesystem::path dict_path(directory);
	if (!std::filesystem::exists(dict_path))
		return;

	for (const auto & entry : std::filesystem::directory_iterator(dict_path))
	{
		if (!entry.is_regular_file())
			continue;

		const auto extension = entry.path().extension().string();
		if (extension != ".aff")
			continue;

		auto dic_file = entry.path();
		dic_file.replace_extension(".dic");
		if (!std::filesystem::exists(dic_file))
			continue;

		const auto stem = entry.path().stem().string();
		const auto aff_path = entry.path().string();

		m_foreign_spell_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
		m_native_spell_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
	}
}
