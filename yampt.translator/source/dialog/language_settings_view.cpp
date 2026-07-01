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
    , dictionaries_dir_(dictionaries_dir)
{
    auto * layout = new QVBoxLayout(this);

    auto * top_form = new QFormLayout;
    native_language_combo_ = new QComboBox(this);
    foreign_language_combo_ = new QComboBox(this);

    for (const auto & entry : languages)
    {
        native_language_combo_->addItem(entry.display_name, QString(entry.code));
        foreign_language_combo_->addItem(entry.display_name, QString(entry.code));
    }

    top_form->addRow("Native Language:", native_language_combo_);
    top_form->addRow("Foreign Language:", foreign_language_combo_);
    layout->addLayout(top_form);

    auto * separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    auto * separator_label = new QLabel("Auto-configured (editable)", this);
    separator_label->setEnabled(false);
    layout->addWidget(separator_label);

    auto * auto_form = new QFormLayout;

    encoding_combo_ = new QComboBox(this);
    encoding_combo_->addItem("Windows-1250");
    encoding_combo_->addItem("Windows-1251");
    encoding_combo_->addItem("Windows-1252");
    auto_form->addRow("Encoding:", encoding_combo_);

    spell_check_combo_ = new QComboBox(this);
    auto_form->addRow("Spell Check Dictionary:", spell_check_combo_);

    translation_target_combo_ = new QComboBox(this);
    populate_targets(translation_target_combo_);
    auto_form->addRow("Translation Target:", translation_target_combo_);

    partial_dict_combo_ = new QComboBox(this);
    auto_form->addRow("Partial-Mode Dictionary:", partial_dict_combo_);

    native_tag_edit_ = new QLineEdit(this);
    native_tag_edit_->setMaxLength(5);
    auto_form->addRow("Native Language Tag:", native_tag_edit_);

    foreign_tag_edit_ = new QLineEdit(this);
    foreign_tag_edit_->setMaxLength(5);
    auto_form->addRow("Foreign Language Tag:", foreign_tag_edit_);

    layout->addLayout(auto_form);
    layout->addStretch();

    scan_dictionaries(dictionaries_dir_);

    connect(native_language_combo_, &QComboBox::currentIndexChanged,
            this, &language_settings_view_t::on_native_language_changed);

    connect(foreign_language_combo_, &QComboBox::currentIndexChanged,
            this, &language_settings_view_t::on_foreign_language_changed);
}

void language_settings_view_t::load(const app_settings_t & settings)
{
    const auto native_code = QString::fromStdString(settings.native_language());
    const auto foreign_code = QString::fromStdString(settings.foreign_language());

    const int native_index = native_language_combo_->findData(native_code);
    if (native_index >= 0)
        native_language_combo_->setCurrentIndex(native_index);

    const int foreign_index = foreign_language_combo_->findData(foreign_code);
    if (foreign_index >= 0)
        foreign_language_combo_->setCurrentIndex(foreign_index);

    encoding_combo_->setCurrentIndex(settings.encoding_index());

    const auto spell_aff = QString::fromStdString(settings.spell_aff_path());
    select_combo_by_data(spell_check_combo_, spell_aff);

    const auto target = QString::fromStdString(settings.translation_target());
    const int target_index = translation_target_combo_->findText(target);
    if (target_index >= 0)
        translation_target_combo_->setCurrentIndex(target_index);

    const auto partial_aff = QString::fromStdString(settings.partial_dict_aff_path());
    select_combo_by_data(partial_dict_combo_, partial_aff);

    native_tag_edit_->setText(QString::fromStdString(settings.native_tag()));
    foreign_tag_edit_->setText(QString::fromStdString(settings.foreign_tag()));
}

void language_settings_view_t::apply(app_settings_t & settings) const
{
    const auto native_code = native_language_combo_->currentData().toString().toStdString();
    settings.set_native_language(native_code);

    const auto foreign_code = foreign_language_combo_->currentData().toString().toStdString();
    settings.set_foreign_language(foreign_code);

    settings.set_encoding_index(encoding_combo_->currentIndex());

    const auto spell_aff = spell_check_combo_->currentData().toString().toStdString();
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
        translation_target_combo_->currentText().toStdString());

    const auto partial_aff = partial_dict_combo_->currentData().toString().toStdString();
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

    settings.set_native_tag(native_tag_edit_->text().toStdString());
    settings.set_foreign_tag(foreign_tag_edit_->text().toStdString());
}

void language_settings_view_t::on_native_language_changed(int index)
{
    const auto code = native_language_combo_->itemData(index).toString();

    encoding_combo_->setCurrentIndex(encoding_index_for_language(code));

    const auto dict_prefix = spell_dict_prefix_for_language(code);
    select_combo_by_prefix(spell_check_combo_, dict_prefix);

    const auto target = translation_target_for_language(code);
    const int target_index = translation_target_combo_->findText(target);
    if (target_index >= 0)
        translation_target_combo_->setCurrentIndex(target_index);

    native_tag_edit_->setText(code);
}

void language_settings_view_t::on_foreign_language_changed(int index)
{
    const auto code = foreign_language_combo_->itemData(index).toString();

    const auto dict_prefix = spell_dict_prefix_for_language(code);
    select_combo_by_prefix(partial_dict_combo_, dict_prefix);

    foreign_tag_edit_->setText(code);
}

void language_settings_view_t::scan_dictionaries(const std::string & directory)
{
    spell_check_combo_->clear();
    partial_dict_combo_->clear();

    spell_check_combo_->addItem("None", QString(""));
    partial_dict_combo_->addItem("None", QString(""));

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

        spell_check_combo_->addItem(QString::fromStdString(stem),
                                    QString::fromStdString(aff_path));

        partial_dict_combo_->addItem(QString::fromStdString(stem),
                                     QString::fromStdString(aff_path));
    }
}
