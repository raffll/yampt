#include "translation_suggestion_tab.hpp"
#include "../provider/ctranslate2_provider.hpp"
#include "../provider/deepl_provider.hpp"
#include "../provider/google_provider.hpp"
#include "../../yampt/utility/tools.hpp"

#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextCursor>
#include <QVBoxLayout>

#include <filesystem>
#include <fstream>

translation_suggestion_tab_t::translation_suggestion_tab_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto * top_row = new QHBoxLayout;
	top_row->setSpacing(4);

	source_combo_ = new QComboBox(this);
	source_combo_->setVisible(false);
	top_row->addWidget(source_combo_);

	language_combo_ = new QComboBox(this);
	top_row->addWidget(language_combo_);

	translate_all_btn_ = new QPushButton("Translate 10", this);
	translate_all_btn_->setToolTip("Translate next 10 untranslated entries");
	translate_all_btn_->setFixedWidth(100);
	top_row->addWidget(translate_all_btn_);

	layout->addLayout(top_row);

	result_text_ = new QPlainTextEdit(this);
	result_text_->setReadOnly(true);
	result_text_->setPlaceholderText("Translation suggestion will appear here");
	layout->addWidget(result_text_);

	counter_label_ = new QLabel(this);
	counter_label_->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
	layout->addWidget(counter_label_);

	ct2_provider_ = new ctranslate2_provider_t(this);
	deepl_provider_ = new deepl_provider_t(this);
	google_provider_ = new google_provider_t(this);

	providers_.push_back(ct2_provider_);
	providers_.push_back(deepl_provider_);
	providers_.push_back(google_provider_);

	source_combo_->addItem(QString::fromStdString(ct2_provider_->name()));

	connect(translate_all_btn_, &QPushButton::clicked, this, [this]() { emit translate_all_requested(); });
	connect(
	    source_combo_,
	    qOverload<int>(&QComboBox::currentIndexChanged),
	    this,
	    [this](int)
	{
		rebuild_language_combo();
		update_counter_label();
	});
	connect(
	    language_combo_,
	    qOverload<int>(&QComboBox::currentIndexChanged),
	    this,
	    &translation_suggestion_tab_t::on_language_changed);

	rebuild_language_combo();

	if (language_combo_->count() > 0)
		load_model_for_language(0);
}

void translation_suggestion_tab_t::set_source_text(const std::string & text)
{
	source_text_ = text;
}

void translation_suggestion_tab_t::set_models_dir(const std::string & dir)
{
	models_dir_ = dir;
	rebuild_language_combo();

	if (language_combo_->count() > 0 && !ct2_provider_->is_available())
		load_model_for_language(language_combo_->currentIndex());
}

int translation_suggestion_tab_t::language_index() const
{
	return language_combo_->currentIndex();
}

void translation_suggestion_tab_t::set_language_index(int index)
{
	if (index >= 0 && index < language_combo_->count())
		language_combo_->setCurrentIndex(index);
}

void translation_suggestion_tab_t::on_language_changed(int index)
{
	if (source_combo_->currentIndex() == 0)
		load_model_for_language(index);
}

void translation_suggestion_tab_t::rebuild_language_combo()
{
	language_combo_->blockSignals(true);
	language_combo_->clear();
	languages_.clear();

	namespace fs = std::filesystem;

	if (!models_dir_.empty() && fs::is_directory(models_dir_))
	{
		std::vector<std::string> nllb_models;

		for (const auto & entry : fs::directory_iterator(models_dir_))
		{
			if (!entry.is_directory())
				continue;

			auto dir = entry.path();
			if (fs::exists(dir / "sentencepiece.bpe.model") && fs::is_directory(dir / "model"))
				nllb_models.push_back(dir.string());
		}

		if (!nllb_models.empty())
		{
			static const std::vector<std::pair<std::string, std::string>> nllb_targets = {
				{ "pol_Latn", "PL" }, { "deu_Latn", "DE" }, { "fra_Latn", "FR" }, { "rus_Cyrl", "RU" },
				{ "spa_Latn", "ES" }, { "ita_Latn", "IT" }, { "ces_Latn", "CS" }, { "nld_Latn", "NL" },
				{ "por_Latn", "PT" }, { "ukr_Cyrl", "UK" },
			};

			const auto & model_path = nllb_models[0];
			for (const auto & [code, label] : nllb_targets)
			{
				auto display = QString("EN -> %1").arg(QString::fromStdString(label));
				languages_.push_back({ code, display.toStdString(), model_path });
				language_combo_->addItem(display);
			}
		}
	}

	language_combo_->blockSignals(false);
}

void translation_suggestion_tab_t::load_model_for_language(int index)
{
	if (index < 0 || index >= static_cast<int>(languages_.size()))
		return;

	const auto & lang = languages_[index];
	if (lang.model_path.empty())
		return;

	namespace fs = std::filesystem;
	auto lang_file = fs::path(lang.model_path) / "languages.txt";
	{
		std::ofstream f(lang_file);
		f << "eng_Latn\n" << lang.code << "\n";
	}

	ct2_provider_->load_model(lang.model_path);

	if (ct2_provider_->is_available())
		counter_label_->setText(QString::fromStdString("Model loaded: " + lang.display));
	else
		counter_label_->setText("Failed to load model");
}

void translation_suggestion_tab_t::update_counter_label()
{
	if (ct2_provider_->is_available())
		counter_label_->setText("Model loaded");
	else
		counter_label_->setText("No model loaded");
}

ctranslate2_provider_t * translation_suggestion_tab_t::ct2_provider() const
{
	return ct2_provider_;
}

void translation_suggestion_tab_t::append_log(const std::string & msg)
{
	result_text_->moveCursor(QTextCursor::End);
	result_text_->insertPlainText(QString::fromStdString(msg));
	result_text_->verticalScrollBar()->setValue(result_text_->verticalScrollBar()->maximum());
}

void translation_suggestion_tab_t::set_translate_all_enabled(bool enabled)
{
	translate_all_btn_->setEnabled(enabled);
}
