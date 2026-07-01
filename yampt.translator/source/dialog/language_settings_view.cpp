#include "dialog/language_settings_view.hpp"
#include <io/app_settings.hpp>

#include <filesystem>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace
{

struct language_entry_t
{
    const char * display_name;
    const char * code;
};

constexpr language_entry_t languages[] = {
    {"English", "EN"},
    {"Polish", "PL"},
    {"German", "DE"},
    {"French", "FR"},
    {"Russian", "RU"},
};

int encoding_index_for_language(const QString & code)
{
    if (code == "PL")
        return 0;
    if (code == "RU")
        return 1;
    return 2;
}

QString translation_target_for_language(const QString & code)
{
    if (code == "PL")
        return "pol_Latn";
    if (code == "DE")
        return "deu_Latn";
    if (code == "FR")
        return "fra_Latn";
    if (code == "RU")
        return "rus_Cyrl";
    return "eng_Latn";
}

QString spell_dict_prefix_for_language(const QString & code)
{
    if (code == "PL")
        return "pl_PL";
    if (code == "DE")
        return "de_DE";
    if (code == "FR")
        return "fr_FR";
    if (code == "RU")
        return "ru_RU";
    return "en_US";
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

void populate_targets(QComboBox * combo)
{
    combo->addItem("eng_Latn");
    combo->addItem("pol_Latn");
    combo->addItem("deu_Latn");
    combo->addItem("fra_Latn");
    combo->addItem("rus_Cyrl");
    combo->addItem("spa_Latn");
    combo->addItem("ita_Latn");
    combo->addItem("por_Latn");
    combo->addItem("nld_Latn");
    combo->addItem("ces_Latn");
    combo->addItem("ukr_Cyrl");
    combo->addItem("ron_Latn");
    combo->addItem("hun_Latn");
    combo->addItem("bul_Cyrl");
    combo->addItem("hrv_Latn");
    combo->addItem("slk_Latn");
    combo->addItem("slv_Latn");
    combo->addItem("srp_Cyrl");
    combo->addItem("dan_Latn");
    combo->addItem("fin_Latn");
    combo->addItem("nob_Latn");
    combo->addItem("swe_Latn");
    combo->addItem("tur_Latn");
    combo->addItem("ell_Grek");
    combo->addItem("jpn_Jpan");
    combo->addItem("kor_Hang");
    combo->addItem("zho_Hans");
    combo->addItem("zho_Hant");
    combo->addItem("arb_Arab");
    combo->addItem("hin_Deva");
    combo->addItem("vie_Latn");
    combo->addItem("tha_Thai");
    combo->addItem("ind_Latn");
}

} // namespace

language_settings_view_t::language_settings_view_t(const std::string & dictionaries_dir, QWidget * parent)
    : QWidget(parent)
    , m_dictionaries_dir(dictionaries_dir)
{
    auto * layout = new QVBoxLayout(this);

    auto * top_form = new QFormLayout;
    m_native_language_combo = new QComboBox(this);
    m_foreign_language_combo = new QComboBox(this);

    for (const auto & entry : languages)
    {
        m_native_language_combo->addItem(entry.display_name, QString(entry.code));
        m_foreign_language_combo->addItem(entry.display_name, QString(entry.code));
    }

    top_form->addRow("Native Language:", m_native_language_combo);
    top_form->addRow("Foreign Language:", m_foreign_language_combo);
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
    populate_targets(m_translation_target_combo);
    auto_form->addRow("Translation Target:", m_translation_target_combo);

    m_partial_dict_combo = new QComboBox(this);
    auto_form->addRow("Partial-Mode Dictionary:", m_partial_dict_combo);

    m_native_tag_edit = new QLineEdit(this);
    m_native_tag_edit->setMaxLength(5);
    auto_form->addRow("Native Language Tag:", m_native_tag_edit);

    m_foreign_tag_edit = new QLineEdit(this);
    m_foreign_tag_edit->setMaxLength(5);
    auto_form->addRow("Foreign Language Tag:", m_foreign_tag_edit);

    layout->addLayout(auto_form);
    layout->addStretch();

    scan_dictionaries(m_dictionaries_dir);

    connect(m_native_language_combo, &QComboBox::currentIndexChanged,
            this, &language_settings_view_t::on_native_language_changed);

    connect(m_foreign_language_combo, &QComboBox::currentIndexChanged,
            this, &language_settings_view_t::on_foreign_language_changed);
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

    m_encoding_combo->setCurrentIndex(settings.encoding_index());

    const auto spell_aff = QString::fromStdString(settings.spell_aff_path());
    select_combo_by_data(m_spell_check_combo, spell_aff);

    const auto target = QString::fromStdString(settings.translation_target());
    const int target_index = m_translation_target_combo->findText(target);
    if (target_index >= 0)
        m_translation_target_combo->setCurrentIndex(target_index);

    const auto partial_aff = QString::fromStdString(settings.partial_dict_aff_path());
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

    settings.set_translation_target(
        m_translation_target_combo->currentText().toStdString());

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

    m_encoding_combo->setCurrentIndex(encoding_index_for_language(code));

    const auto dict_prefix = spell_dict_prefix_for_language(code);
    select_combo_by_prefix(m_spell_check_combo, dict_prefix);

    const auto target = translation_target_for_language(code);
    const int target_index = m_translation_target_combo->findText(target);
    if (target_index >= 0)
        m_translation_target_combo->setCurrentIndex(target_index);

    m_native_tag_edit->setText(code);
}

void language_settings_view_t::on_foreign_language_changed(int index)
{
    const auto code = m_foreign_language_combo->itemData(index).toString();

    const auto dict_prefix = spell_dict_prefix_for_language(code);
    select_combo_by_prefix(m_partial_dict_combo, dict_prefix);

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

        m_spell_check_combo->addItem(QString::fromStdString(stem),
                                    QString::fromStdString(aff_path));

        m_partial_dict_combo->addItem(QString::fromStdString(stem),
                                     QString::fromStdString(aff_path));
    }
}
