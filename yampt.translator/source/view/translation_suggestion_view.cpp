#include <resource_paths.hpp>
#include "translation_suggestion_view.hpp"
#include "../translator/ctranslate2_translator.hpp"
#include "../translator/model_list_utils.hpp"
#include "../translator/web_translator.hpp"
#include <utility/app_logger.hpp>
#include <utility/language_config.hpp>
#include <filesystem>
#include <fstream>
#include <settings_store.hpp>
#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QVBoxLayout>

namespace {

const provider_setting_t * find_model_setting(const web_translator_config_t & config)
{
	for (const auto & setting : config.settings)
	{
		if (setting.key == "model")
			return &setting;
	}

	return nullptr;
}

} // namespace

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

	auto * model_row = new QHBoxLayout;
	model_row->setSpacing(4);

	m_model_combo = new QComboBox(this);
	m_model_combo->setToolTip(tr("Select the AI model for this provider"));
	m_model_combo->setFixedWidth(180);
	model_row->addWidget(m_model_combo);

	m_refresh_models = new QPushButton(tr("Refresh"), this);
	m_refresh_models->setToolTip(tr("Fetch the model list from the provider"));
	m_refresh_models->setFixedWidth(100);
	model_row->addWidget(m_refresh_models);

	model_row->addStretch();
	layout->addLayout(model_row);

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
	m_provider_combo->addItem(tr("CTranslate2"));

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
	    m_refresh_models,
	    &QPushButton::clicked,
	    this,
	    [this]()
	{
		auto * web_provider = active_web_provider();
		if (web_provider == nullptr)
			return;

		web_provider->fetch_models();
	});

	connect(
	    m_model_combo,
	    &QComboBox::currentTextChanged,
	    this,
	    [this](const QString & value)
	{
		auto * web_provider = active_web_provider();
		if (web_provider == nullptr)
			return;

		const auto & identifier = web_provider->config().identifier;

		if (m_settings != nullptr)
		{
			m_settings->set_web_provider_setting(identifier, "model", value.toStdString());
			m_settings->sync();
		}

		web_provider->set_setting("model", value.toStdString());
	});

	connect(
	    m_ct2_provider,
	    &ctranslate2_translator_t::translation_finished,
	    this,
	    [this](translation_suggestion_t result) { on_provider_result(result); });

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
		provider->set_examples(m_examples);
		m_web_providers.push_back(provider);
		m_providers.push_back(provider);
		m_provider_combo->addItem(QString::fromStdString(config.display_name));

		connect(
		    provider,
		    &web_translator_t::translation_finished,
		    this,
		    [this](translation_suggestion_t result)
		{
			on_provider_result(result);
			update_provider_status();
		});

		connect(
		    provider,
		    &web_translator_t::models_fetched,
		    this,
		    [this, provider](std::vector<std::string> models)
		{
			if (active_web_provider() != provider)
				return;

			const auto * model_setting = find_model_setting(provider->config());
			const auto default_model = model_setting != nullptr ? model_setting->default_value : std::string();
			const auto current_text = m_model_combo->currentText().toStdString();
			const auto selected =
			    model_list_utils::choose_selected_model(current_text, models, default_model);
			populate_model_combo(models, selected);
			append_log("[info] fetched " + std::to_string(models.size()) + " models\n");
		});

		connect(
		    provider,
		    &web_translator_t::models_fetch_failed,
		    this,
		    [this, provider](std::string error)
		{
			if (active_web_provider() != provider)
				return;

			append_log("[error] " + error + "\n");
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

void translation_suggestion_view_t::set_settings_store(settings_store_t & settings)
{
	m_settings = &settings;
}

void translation_suggestion_view_t::apply_provider_settings(const settings_store_t & settings)
{
	const int language_index = settings.translation_language_index();
	const auto source_language = settings.foreign_language();

	m_target_language = settings.native_language();

	set_examples(settings.translation_examples());

	for (auto * web_provider : m_web_providers)
	{
		const auto & config = web_provider->config();

		std::unordered_map<std::string, std::string> provider_settings;
		for (const auto & setting : config.settings)
		{
			auto value = settings.web_provider_setting(config.identifier, setting.key);
			if (value.empty() && !setting.default_value.empty())
				value = setting.default_value;

			provider_settings[setting.key] = value;
		}

		web_provider->set_provider_settings(provider_settings);
		web_provider->set_source_language(source_language);
	}

	if (m_languages.empty())
		rebuild_language_list();

	if (language_index >= 0 && language_index < static_cast<int>(m_languages.size()))
		load_model_for_language(language_index);
	else if (!m_languages.empty())
		load_model_for_language(0);

	update_model_controls();
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
			    tr(": %L1 chars remaining").arg(provider->remaining_quota()));
		}
		else
		{
			m_status_label->setText(QString::fromStdString(provider->name()) + tr(": ready"));
		}
	}
	else
	{
		m_status_label->setText(QString::fromStdString(provider->name()) + tr(": no API key"));
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
			const auto languages =
			    language_config::load(resource_paths::languages_file());

			const auto & model_path = nllb_models[0];
			for (const auto & lang : languages)
			{
				if (lang.code == "EN")
					continue;

				auto display = "EN -> " + lang.code;
				m_languages.push_back({ lang.nllb_code, display, model_path });
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

void translation_suggestion_view_t::append_log(const std::string & msg)
{
	auto * document = m_result_text->document();
	if (document->characterCount() > 1 && !document->toPlainText().endsWith('\n'))
		m_result_text->appendPlainText(QString());

	m_result_text->moveCursor(QTextCursor::End);
	m_result_text->insertPlainText(QString::fromStdString(msg));
	m_result_text->verticalScrollBar()->setValue(m_result_text->verticalScrollBar()->maximum());
}

void translation_suggestion_view_t::select_provider(int index)
{
	if (index < 0 || index >= static_cast<int>(m_providers.size()))
		return;

	m_active_provider_index = index;

	if (m_provider_combo->currentIndex() != index)
		m_provider_combo->setCurrentIndex(index);

	update_model_controls();
	update_provider_status();
}

translator_t * translation_suggestion_view_t::active_provider() const
{
	if (m_active_provider_index < 0 || m_active_provider_index >= static_cast<int>(m_providers.size()))
		return nullptr;

	return m_providers[m_active_provider_index];
}

web_translator_t * translation_suggestion_view_t::active_web_provider() const
{
	if (m_active_provider_index <= 0)
		return nullptr;

	const auto web_index = m_active_provider_index - 1;
	if (web_index >= static_cast<int>(m_web_providers.size()))
		return nullptr;

	return m_web_providers[web_index];
}

void translation_suggestion_view_t::populate_model_combo(
    const std::vector<std::string> & models,
    const std::string & selected)
{
	const QSignalBlocker blocker(m_model_combo);
	m_model_combo->clear();

	for (const auto & model : models)
		m_model_combo->addItem(QString::fromStdString(model));

	const int selected_index = m_model_combo->findText(QString::fromStdString(selected));
	m_model_combo->setCurrentIndex(selected_index >= 0 ? selected_index : 0);
}

void translation_suggestion_view_t::update_model_controls()
{
	auto * web_provider = active_web_provider();
	if (web_provider == nullptr)
	{
		m_model_combo->hide();
		m_refresh_models->hide();
		return;
	}

	const auto & config = web_provider->config();
	const auto * model_setting = find_model_setting(config);
	if (model_setting == nullptr)
	{
		m_model_combo->hide();
		m_refresh_models->hide();
		return;
	}

	auto selected = model_setting->default_value;
	if (m_settings != nullptr)
	{
		const auto stored = m_settings->web_provider_setting(config.identifier, "model");
		if (!stored.empty())
			selected = stored;
	}

	m_model_combo->show();
	populate_model_combo(model_setting->choices, selected);

	if (config.models_endpoint.empty())
		m_refresh_models->hide();
	else
		m_refresh_models->show();
}

void translation_suggestion_view_t::set_glossary_fn(std::function<std::string(const std::string &)> fn)
{
	m_glossary_fn = std::move(fn);

	for (auto * web_provider : m_web_providers)
		web_provider->set_glossary_fn(m_glossary_fn);
}

void translation_suggestion_view_t::set_examples(const std::vector<translation_example_t> & examples)
{
	m_examples = examples;

	for (auto * web_provider : m_web_providers)
		web_provider->set_examples(m_examples);
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

void translation_suggestion_view_t::set_target_language(const std::string & language)
{
	m_target_language = language;
}

bool translation_suggestion_view_t::is_translating() const
{
	return m_translating;
}

void translation_suggestion_view_t::request_translation(const std::string & text)
{
	auto * provider = active_provider();
	if (!provider || !provider->is_available())
	{
		emit translation_failed("translation provider not available");
		return;
	}

	m_translating = true;
	m_line_queue.clear();
	m_line_results.clear();
	m_translate_all_btn->setEnabled(false);
	provider->translate(text, m_target_language);
}

void translation_suggestion_view_t::request_translation_lines(const std::vector<std::string> & lines)
{
	auto * provider = active_provider();
	if (!provider || !provider->is_available())
	{
		emit translation_failed("translation provider not available");
		return;
	}

	m_translating = true;
	m_line_queue = lines;
	m_line_results.clear();
	m_translate_all_btn->setEnabled(false);
	advance_line_queue();
}

void translation_suggestion_view_t::advance_line_queue()
{
	while (!m_line_queue.empty() && m_line_queue.front().empty())
	{
		m_line_results.push_back(std::string());
		m_line_queue.erase(m_line_queue.begin());
	}

	if (m_line_queue.empty())
	{
		m_translating = false;
		m_translate_all_btn->setEnabled(true);
		emit translation_lines_committed(m_line_results);
		return;
	}

	auto * provider = active_provider();
	if (!provider)
	{
		m_translating = false;
		m_translate_all_btn->setEnabled(true);
		emit translation_failed("translation provider became unavailable");
		return;
	}

	provider->translate(m_line_queue.front(), m_target_language);
}

void translation_suggestion_view_t::on_provider_result(const translation_suggestion_t & result)
{
	if (!m_translating)
	{
		display_translation_result(result);
		return;
	}

	if (!result.success)
	{
		m_translating = false;
		m_line_queue.clear();
		m_line_results.clear();
		m_translate_all_btn->setEnabled(true);
		display_translation_result(result);
		emit translation_failed(result.error);
		return;
	}

	if (!m_line_queue.empty())
	{
		m_line_results.push_back(result.text);
		m_line_queue.erase(m_line_queue.begin());
		advance_line_queue();
		return;
	}

	m_translating = false;
	m_translate_all_btn->setEnabled(true);
	display_translation_result(result);
	emit translation_committed(result.text);
}
