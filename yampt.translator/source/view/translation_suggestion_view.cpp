#include "translation_suggestion_view.hpp"
#include "../translator/ctranslate2_translator.hpp"
#include "../translator/deepl_translator.hpp"
#include "../translator/google_translator.hpp"
#include <io/app_settings.hpp>
#include <utility/tools.hpp>
#include <filesystem>
#include <fstream>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextCursor>
#include <QVBoxLayout>

translation_suggestion_view_t::translation_suggestion_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto * top_row = new QHBoxLayout;
	top_row->setSpacing(4);

	translate_all_btn_ = new QPushButton("Translate 10", this);
	translate_all_btn_->setToolTip("Translate next 10 untranslated entries");
	translate_all_btn_->setFixedWidth(100);
	top_row->addWidget(translate_all_btn_);

	top_row->addStretch();
	layout->addLayout(top_row);

	result_text_ = new QPlainTextEdit(this);
	result_text_->setReadOnly(true);
	result_text_->setPlaceholderText("Translation suggestion will appear here");
	layout->addWidget(result_text_);

	counter_label_ = new QLabel(this);
	counter_label_->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
	layout->addWidget(counter_label_);

	setup_controls();
}

void translation_suggestion_view_t::setup_controls()
{
	ct2_provider_ = new ctranslate2_translator_t(this);
	deepl_translator_ = new deepl_translator_t(this);
	google_translator_ = new google_translator_t(this);

	providers_.push_back(ct2_provider_);
	providers_.push_back(deepl_translator_);
	providers_.push_back(google_translator_);

	connect(translate_all_btn_, &QPushButton::clicked, this, [this]() { emit translate_all_requested(); });

	rebuild_language_list();
}

void translation_suggestion_view_t::set_source_text(const std::string & text)
{
	source_text_ = text;
}

void translation_suggestion_view_t::set_models_dir(const std::string & dir)
{
	models_dir_ = dir;
	rebuild_language_list();
}

void translation_suggestion_view_t::apply_provider_settings(const app_settings_t & settings)
{
	const int language_index = settings.translation_language_index();

	if (languages_.empty())
		rebuild_language_list();

	if (language_index >= 0 && language_index < static_cast<int>(languages_.size()))
		load_model_for_language(language_index);
	else if (!languages_.empty())
		load_model_for_language(0);
}

void translation_suggestion_view_t::rebuild_language_list()
{
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
				auto display = "EN -> " + label;
				languages_.push_back({ code, display, model_path });
			}
		}
	}
}

void translation_suggestion_view_t::load_model_for_language(int index)
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

void translation_suggestion_view_t::update_counter_label()
{
	if (ct2_provider_->is_available())
		counter_label_->setText("Model loaded");
	else
		counter_label_->setText("No model loaded");
}

ctranslate2_translator_t * translation_suggestion_view_t::ct2_provider() const
{
	return ct2_provider_;
}

void translation_suggestion_view_t::append_log(const std::string & msg)
{
	result_text_->moveCursor(QTextCursor::End);
	result_text_->insertPlainText(QString::fromStdString(msg));
	result_text_->verticalScrollBar()->setValue(result_text_->verticalScrollBar()->maximum());
}

void translation_suggestion_view_t::set_translate_all_enabled(bool enabled)
{
	translate_all_btn_->setEnabled(enabled);
}
