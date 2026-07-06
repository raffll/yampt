#include "translation_suggestion_view.hpp"
#include "../translator/ctranslate2_translator.hpp"
#include "../translator/deepl_translator.hpp"
#include "../translator/google_translator.hpp"
#include <utility/app_logger.hpp>
#include <filesystem>
#include <fstream>
#include <settings_store.hpp>
#include <QComboBox>
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

	m_provider_combo = new QComboBox(this);
	m_provider_combo->addItem("CTranslate2");
	m_provider_combo->setToolTip("Select translation provider");
	m_provider_combo->setFixedWidth(120);
	m_provider_combo->setVisible(false);
	top_row->addWidget(m_provider_combo);

	m_translate_all_btn = new QPushButton("Translate", this);
	m_translate_all_btn->setToolTip("Translate the selected entry");
	m_translate_all_btn->setFixedWidth(100);
	top_row->addWidget(m_translate_all_btn);

	top_row->addStretch();
	layout->addLayout(top_row);

	m_result_text = new QPlainTextEdit(this);
	m_result_text->setReadOnly(true);
	m_result_text->setPlaceholderText("Translation suggestion will appear here");
	layout->addWidget(m_result_text);

	m_status_label = new QLabel(this);
	m_status_label->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
	layout->addWidget(m_status_label);

	setup_controls();
}

void translation_suggestion_view_t::setup_controls()
{
	m_ct2_provider = new ctranslate2_translator_t(this);
	m_deepl_translator = new deepl_translator_t(this);
	m_google_translator = new google_translator_t(this);

	m_providers.push_back(m_ct2_provider);
	m_providers.push_back(m_deepl_translator);
	m_providers.push_back(m_google_translator);

	connect(m_translate_all_btn, &QPushButton::clicked, this, [this]() { emit translate_all_requested(); });

	connect(m_provider_combo, &QComboBox::currentIndexChanged, this, [this](int index) { select_provider(index); });

	connect(
	    m_ct2_provider,
	    &ctranslate2_translator_t::translation_finished,
	    this,
	    [this](translation_suggestion_t result) { display_translation_result(result); });

	connect(
	    m_deepl_translator,
	    &deepl_translator_t::translation_finished,
	    this,
	    [this](translation_suggestion_t result)
	{
		display_translation_result(result);
		update_provider_status();
	});

	connect(
	    m_google_translator,
	    &google_translator_t::translation_finished,
	    this,
	    [this](translation_suggestion_t result) { display_translation_result(result); });

	rebuild_language_list();
	update_provider_status();
}

void translation_suggestion_view_t::set_source_text(const std::string & text)
{
	m_source_text = text;
}

void translation_suggestion_view_t::set_models_dir(const std::string & dir)
{
	m_models_dir = dir;
	rebuild_language_list();
}

void translation_suggestion_view_t::apply_provider_settings(const settings_store_t & settings)
{
	const int language_index = settings.translation_language_index();

	m_deepl_translator->set_api_key(settings.deepl_api_key());
	m_google_translator->set_api_key(settings.google_api_key());

	if (m_languages.empty())
		rebuild_language_list();

	if (language_index >= 0 && language_index < static_cast<int>(m_languages.size()))
		load_model_for_language(language_index);
	else if (!m_languages.empty())
		load_model_for_language(0);

	update_provider_status();
}

void translation_suggestion_view_t::update_provider_status()
{
	auto * provider = active_provider();
	if (!provider)
	{
		m_status_label->setText("");
		return;
	}

	if (provider == m_ct2_provider)
	{
		if (m_ct2_provider->is_available())
			m_status_label->setText("CTranslate2: model loaded");
		else
			m_status_label->setText("CTranslate2: no model");
	}
	else if (provider == m_deepl_translator)
	{
		if (m_deepl_translator->is_available())
		{
			const int remaining = m_deepl_translator->remaining_quota();
			m_status_label->setText(QString("DeepL: %L1 chars remaining").arg(remaining));
		}
		else
		{
			m_status_label->setText("DeepL: no API key");
		}
	}
	else if (provider == m_google_translator)
	{
		if (m_google_translator->is_available())
			m_status_label->setText("Google: active");
		else
			m_status_label->setText("Google: no API key");
	}
}

void translation_suggestion_view_t::rebuild_language_list()
{
	m_languages.clear();

	namespace fs = std::filesystem;

	if (!m_models_dir.empty() && fs::is_directory(m_models_dir))
	{
		std::vector<std::string> nllb_models;

		for (const auto & entry : fs::directory_iterator(m_models_dir))
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
				m_languages.push_back({ code, display, model_path });
			}
		}
	}
}

void translation_suggestion_view_t::load_model_for_language(int index)
{
	if (index < 0 || index >= static_cast<int>(m_languages.size()))
		return;

	const auto & lang = m_languages[index];
	if (lang.model_path.empty())
		return;

	namespace fs = std::filesystem;
	auto lang_file = fs::path(lang.model_path) / "languages.txt";
	{
		std::ofstream f(lang_file);
		f << "eng_Latn\n" << lang.code << "\n";
	}

	m_ct2_provider->load_model(lang.model_path);
	update_provider_status();
}

ctranslate2_translator_t * translation_suggestion_view_t::ct2_provider() const
{
	return m_ct2_provider;
}

void translation_suggestion_view_t::append_log(const std::string & msg)
{
	m_result_text->moveCursor(QTextCursor::End);
	m_result_text->insertPlainText(QString::fromStdString(msg));
	m_result_text->verticalScrollBar()->setValue(m_result_text->verticalScrollBar()->maximum());
}

void translation_suggestion_view_t::set_translate_all_enabled(bool enabled)
{
	m_translate_all_btn->setEnabled(enabled);
}

void translation_suggestion_view_t::set_batch_in_progress(bool in_progress)
{
	m_batch_in_progress = in_progress;
	m_provider_combo->setEnabled(!in_progress);
	m_translate_all_btn->setEnabled(!in_progress);
}

void translation_suggestion_view_t::select_provider(int index)
{
	if (index < 0 || index >= static_cast<int>(m_providers.size()))
		return;

	m_active_provider_index = index;

	if (m_provider_combo->currentIndex() != index)
		m_provider_combo->setCurrentIndex(index);

	update_provider_status();
}

translator_t * translation_suggestion_view_t::active_provider() const
{
	if (m_active_provider_index < 0 || m_active_provider_index >= static_cast<int>(m_providers.size()))
		return nullptr;

	return m_providers[m_active_provider_index];
}

void translation_suggestion_view_t::set_glossary_fn(std::function<std::string(const std::string &)> fn)
{
	m_glossary_fn = std::move(fn);
}

void translation_suggestion_view_t::display_translation_result(const translation_suggestion_t & result)
{
	if (!result.success)
	{
		m_result_text->setPlainText(QString::fromStdString("[error] " + result.error));
		return;
	}

	auto display_text = result.text;

	if (m_glossary_fn)
	{
		auto glossary_result = m_glossary_fn(result.text);
		if (!glossary_result.empty() && glossary_result != result.text)
			display_text += "\n\n[glossary]\n" + glossary_result;
	}

	m_result_text->setPlainText(QString::fromStdString(display_text));
}
