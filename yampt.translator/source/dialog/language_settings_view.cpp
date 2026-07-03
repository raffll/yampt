#include "dialog/language_settings_view.hpp"
#include <app_settings.hpp>
#include <filesystem>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace {

struct language_entry_t
{
	const char * display_name;
	const char * code;
	const char * nllb_target;
	const char * spell_prefix;
	int encoding_index;
};

constexpr language_entry_t languages[] = {
	{ "English", "EN", "eng_Latn", "en_US", 2 },
	{ "Polish", "PL", "pol_Latn", "pl_PL", 0 },
	{ "German", "DE", "deu_Latn", "de_DE", 2 },
	{ "French", "FR", "fra_Latn", "fr_FR", 2 },
	{ "Russian", "RU", "rus_Cyrl", "ru_RU", 1 },
	{ "Spanish", "ES", "spa_Latn", "es_ES", 2 },
	{ "Italian", "IT", "ita_Latn", "it_IT", 2 },
	{ "Portuguese", "PT", "por_Latn", "pt_PT", 2 },
	{ "Dutch", "NL", "nld_Latn", "nl_NL", 2 },
	{ "Czech", "CZ", "ces_Latn", "cs_CZ", 0 },
	{ "Ukrainian", "UA", "ukr_Cyrl", "uk_UA", 1 },
	{ "Romanian", "RO", "ron_Latn", "ro_RO", 2 },
	{ "Hungarian", "HU", "hun_Latn", "hu_HU", 0 },
	{ "Bulgarian", "BG", "bul_Cyrl", "bg_BG", 1 },
	{ "Croatian", "HR", "hrv_Latn", "hr_HR", 0 },
	{ "Slovak", "SK", "slk_Latn", "sk_SK", 0 },
	{ "Slovenian", "SI", "slv_Latn", "sl_SI", 0 },
	{ "Serbian", "RS", "srp_Cyrl", "sr_RS", 1 },
	{ "Danish", "DK", "dan_Latn", "da_DK", 2 },
	{ "Finnish", "FI", "fin_Latn", "fi_FI", 2 },
	{ "Norwegian", "NO", "nob_Latn", "nb_NO", 2 },
	{ "Swedish", "SE", "swe_Latn", "sv_SE", 2 },
	{ "Turkish", "TR", "tur_Latn", "tr_TR", 2 },
	{ "Greek", "GR", "ell_Grek", "el_GR", 2 },
	{ "Japanese", "JP", "jpn_Jpan", "ja_JP", 2 },
	{ "Korean", "KR", "kor_Hang", "ko_KR", 2 },
	{ "Chinese (Simplified)", "CN", "zho_Hans", "zh_CN", 2 },
	{ "Chinese (Traditional)", "TW", "zho_Hant", "zh_TW", 2 },
	{ "Arabic", "AR", "arb_Arab", "ar_SA", 2 },
	{ "Hindi", "IN", "hin_Deva", "hi_IN", 2 },
	{ "Vietnamese", "VN", "vie_Latn", "vi_VN", 2 },
	{ "Thai", "TH", "tha_Thai", "th_TH", 2 },
	{ "Indonesian", "ID", "ind_Latn", "id_ID", 2 },
};

constexpr int language_count = sizeof(languages) / sizeof(languages[0]);

const language_entry_t * find_language(const QString & code)
{
	for (int i = 0; i < language_count; ++i)
	{
		if (code == languages[i].code)
			return &languages[i];
	}
	return nullptr;
}

void select_combo_by_data(QComboBox * combo, const QString & value)
{
	for (int i = 0; i < combo->count(); ++i)
	{
		if (combo->itemData(i).toString() == value)
		{
			combo->setCurrentIndex(i);
			return;
		}
	}
}

void select_combo_by_prefix(QComboBox * combo, const QString & prefix)
{
	for (int i = 0; i < combo->count(); ++i)
	{
		if (combo->itemText(i) == prefix)
		{
			combo->setCurrentIndex(i);
			return;
		}
	}
}

} // namespace

language_settings_view_t::language_settings_view_t(const std::string & dictionaries_dir, QWidget * parent)
    : QWidget(parent)
    , m_dictionaries_dir(dictionaries_dir)
{
	auto * layout = new QVBoxLayout(this);

	auto * top_form = new QFormLayout;
	m_foreign_language_combo = new QComboBox(this);
	m_native_language_combo = new QComboBox(this);

	for (int i = 0; i < language_count; ++i)
	{
		m_foreign_language_combo->addItem(languages[i].display_name, QString(languages[i].code));
		m_native_language_combo->addItem(languages[i].display_name, QString(languages[i].code));
	}

	top_form->addRow("Foreign Language:", m_foreign_language_combo);
	top_form->addRow("Native Language:", m_native_language_combo);
	layout->addLayout(top_form);

	auto * separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(separator);

	auto * separator_label = new QLabel("Auto-configured (editable)", this);
	separator_label->setEnabled(false);
	layout->addWidget(separator_label);

	auto * auto_form = new QFormLayout;

	m_encoding_combo = new QComboBox(this);
	m_encoding_combo->addItem("Windows-1250");
	m_encoding_combo->addItem("Windows-1251");
	m_encoding_combo->addItem("Windows-1252");
	auto_form->addRow("Encoding:", m_encoding_combo);

	m_spell_check_combo = new QComboBox(this);
	auto_form->addRow("Spell Check Dictionary:", m_spell_check_combo);

	m_translation_target_combo = new QComboBox(this);
	for (int i = 0; i < language_count; ++i)
		m_translation_target_combo->addItem(languages[i].nllb_target);
	auto_form->addRow("Translation Target:", m_translation_target_combo);

	m_partial_dict_combo = new QComboBox(this);
	auto_form->addRow("Partial-Mode Dictionary:", m_partial_dict_combo);

	m_foreign_tag_edit = new QLineEdit(this);
	m_foreign_tag_edit->setMaxLength(5);
	auto_form->addRow("Foreign Language Tag:", m_foreign_tag_edit);

	m_native_tag_edit = new QLineEdit(this);
	m_native_tag_edit->setMaxLength(5);
	auto_form->addRow("Native Language Tag:", m_native_tag_edit);

	layout->addLayout(auto_form);
	layout->addStretch();

	scan_dictionaries(m_dictionaries_dir);

	connect(
	    m_native_language_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    &language_settings_view_t::on_native_language_changed);

	connect(
	    m_foreign_language_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    &language_settings_view_t::on_foreign_language_changed);
}

void language_settings_view_t::load(const app_settings_t & settings)
{
	const auto native_code = QString::fromStdString(settings.native_language());
	const auto foreign_code = QString::fromStdString(settings.foreign_language());

	const int native_index = m_native_language_combo->findData(native_code);
	if (native_index >= 0)
		m_native_language_combo->setCurrentIndex(native_index);

	const int foreign_index = m_foreign_language_combo->findData(foreign_code);
	if (foreign_index >= 0)
		m_foreign_language_combo->setCurrentIndex(foreign_index);

	on_native_language_changed(m_native_language_combo->currentIndex());
	on_foreign_language_changed(m_foreign_language_combo->currentIndex());

	m_encoding_combo->setCurrentIndex(settings.encoding_index());

	const auto spell_aff = QString::fromStdString(settings.spell_aff_path());
	if (!spell_aff.isEmpty())
		select_combo_by_data(m_spell_check_combo, spell_aff);

	const auto target = QString::fromStdString(settings.translation_target());
	const int target_index = m_translation_target_combo->findText(target);
	if (target_index >= 0)
		m_translation_target_combo->setCurrentIndex(target_index);

	const auto partial_aff = QString::fromStdString(settings.partial_dict_aff_path());
	if (!partial_aff.isEmpty())
		select_combo_by_data(m_partial_dict_combo, partial_aff);

	m_native_tag_edit->setText(QString::fromStdString(settings.native_tag()));
	m_foreign_tag_edit->setText(QString::fromStdString(settings.foreign_tag()));
}

void language_settings_view_t::apply(app_settings_t & settings) const
{
	const auto native_code = m_native_language_combo->currentData().toString().toStdString();
	settings.set_native_language(native_code);

	const auto foreign_code = m_foreign_language_combo->currentData().toString().toStdString();
	settings.set_foreign_language(foreign_code);

	settings.set_encoding_index(m_encoding_combo->currentIndex());

	const auto spell_aff = m_spell_check_combo->currentData().toString().toStdString();
	settings.set_spell_aff_path(spell_aff);

	if (!spell_aff.empty())
	{
		auto dic_path = spell_aff;
		auto dot_pos = dic_path.rfind(".aff");
		if (dot_pos != std::string::npos)
			dic_path.replace(dot_pos, 4, ".dic");
		settings.set_spell_dic_path(dic_path);
	}
	else
	{
		settings.set_spell_dic_path("");
	}

	settings.set_translation_target(m_translation_target_combo->currentText().toStdString());

	const auto partial_aff = m_partial_dict_combo->currentData().toString().toStdString();
	settings.set_partial_dict_aff_path(partial_aff);

	if (!partial_aff.empty())
	{
		auto dic_path = partial_aff;
		auto dot_pos = dic_path.rfind(".aff");
		if (dot_pos != std::string::npos)
			dic_path.replace(dot_pos, 4, ".dic");
		settings.set_partial_dict_dic_path(dic_path);
	}
	else
	{
		settings.set_partial_dict_dic_path("");
	}

	settings.set_native_tag(m_native_tag_edit->text().toStdString());
	settings.set_foreign_tag(m_foreign_tag_edit->text().toStdString());
}

void language_settings_view_t::on_native_language_changed(int index)
{
	const auto code = m_native_language_combo->itemData(index).toString();
	const auto * lang = find_language(code);
	if (!lang)
		return;

	m_encoding_combo->setCurrentIndex(lang->encoding_index);

	select_combo_by_prefix(m_spell_check_combo, QString(lang->spell_prefix));

	const int target_index = m_translation_target_combo->findText(QString(lang->nllb_target));
	if (target_index >= 0)
		m_translation_target_combo->setCurrentIndex(target_index);

	m_native_tag_edit->setText(code);
}

void language_settings_view_t::on_foreign_language_changed(int index)
{
	const auto code = m_foreign_language_combo->itemData(index).toString();
	const auto * lang = find_language(code);
	if (!lang)
		return;

	select_combo_by_prefix(m_partial_dict_combo, QString(lang->spell_prefix));

	m_foreign_tag_edit->setText(code);
}

void language_settings_view_t::scan_dictionaries(const std::string & directory)
{
	m_spell_check_combo->clear();
	m_partial_dict_combo->clear();

	m_spell_check_combo->addItem("None", QString(""));
	m_partial_dict_combo->addItem("None", QString(""));

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

		m_spell_check_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
		m_partial_dict_combo->addItem(QString::fromStdString(stem), QString::fromStdString(aff_path));
	}
}
