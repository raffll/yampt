#include "translation_suggestion_view.hpp"
#include "../translator/ctranslate2_translator.hpp"
#include "../translator/web_translator.hpp"
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
	m_provider_combo->setToolTip(tr("Select translation provider"));
	m_provider_combo->setFixedWidth(180);
	top_row->addWidget(m_provider_combo);

	m_translate_all_btn = new QPushButton(tr("Translate"), this);
	m_translate_all_btn->setToolTip(tr("Translate the selected entry"));
	m_translate_all_btn->setFixedWidth(100);
	top_row->addWidget(m_translate_all_btn);

	top_row->addStretch();
	layout->addLayout(top_row);

	m_result_text = new QPlainTextEdit(this);
	m_result_text->setReadOnly(true);
	m_result_text->setPlaceholderText(tr("Translation suggestion will appear here"));
	layout->addWidget(m_result_text);

	m_status_label = new QLabel(this);
	m_status_label->setStyleSheet("color: rgb(120, 120, 120); font-size: 11px;");
	layout->addWidget(m_status_label);

	setup_controls();
}

void translation_suggestion_view_t::setup_controls()
{
	m_ct2_provider = new ctranslate2_translator_t(this);
	m_providers.push_back(m_ct2_provider);
	m_provider_combo->addItem("CTranslate2");

	connect(m_translate_all_btn, &QPushButton::clicked, this, [this]() { emit translate_all_requested(); });

	connect(
	    m_provider_combo,
	    &QComboBox::currentIndexChanged,
	    this,
	    [this](int index)
	{
		select_provider(index);
		update_provider_status();
	});

	connect(
	    m_ct2_provider,
	    &ctranslate2_translator_t::translation_finished,
	    this,
	    [this](translation_suggestion_t result) { display_translation_result(result); });

	rebuild_language_list();
	update_provider_status();
}

void translation_suggestion_view_t::set_providers_dir(const std::string & dir)
{
	m_providers_dir = dir;
	rebuild_web_providers();
}

void translation_suggestion_view_t::rebuild_web_providers()
{
	for (auto * web_provider : m_web_providers)
		web_provider->deleteLater();

	m_web_providers.clear();

	while (m_provider_combo->count() > 1)
		m_provider_combo->removeItem(m_provider_combo->count() - 1);

	m_providers.resize(1);

	auto configs = web_translator_config::load_all(m_providers_dir);

	for (auto & config : configs)
	{
		auto * provider = new web_translator_t(config, this);
		provider->set_glossary_fn(m_glossary_fn);
		m_web_providers.push_back(provider);
		m_providers.push_back(provider);
		m_provider_combo->addItem(QString::fromStdString(config.display_name));

		connect(
		    provider,
		    &web_translator_t::translation_finished,
		    this,
		    [this](translation_suggestion_t result)
		{
			display_translation_result(result);
			update_provider_status();
		});
	}
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
	const auto source_language = settings.foreign_language();

	for (auto * web_provider : m_web_providers)
	{
		const auto & config = web_provider->config();
		const auto stored_key = settings.web_api_key(config.identifier);
		web_provider->set_api_key(stored_key);
		web_provider->set_source_language(source_language);
	}

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
			m_status_label->setText(tr("CTranslate2: model loaded"));
		else
			m_status_label->setText(tr("CTranslate2: no model"));

		return;
	}

	if (provider->is_available())
	{
		if (provider->has_quota())
		{
			m_status_label->setText(
			    QString::fromStdString(provider->name()) +
			    QString(": %L1 chars remaining").arg(provider->remaining_quota()));
		}
		else
		{
			m_status_label->setText(QString::fromStdString(provider->name()) + ": ready");
		}
	}
	else
	{
		m_status_label->setText(QString::fromStdString(provider->name()) + ": no API key");
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
		std::ofstream file_stream(lang_file);
		file_stream << "eng_Latn\n" << lang.code << "\n";
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
	auto * document = m_result_text->document();
	if (document->characterCount() > 1 && !document->toPlainText().endsWith('\n'))
		m_result_text->appendPlainText(QString());

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

	for (auto * web_provider : m_web_providers)
		web_provider->set_glossary_fn(m_glossary_fn);
}

void translation_suggestion_view_t::display_translation_result(const translation_suggestion_t & result)
{
	if (!result.success)
	{
		m_result_text->setPlainText(QString::fromStdString("[error] " + result.error));
		return;
	}

	m_result_text->setPlainText(QString::fromStdString(result.text));
}
