#include "ctranslate2_translator.hpp"

ctranslate2_translator_t::ctranslate2_translator_t(QObject * parent)
    : QObject(parent)
{}

std::string ctranslate2_translator_t::name() const
{
	return "CTranslate2 (local)";
}

bool ctranslate2_translator_t::is_available() const
{
	return m_engine.is_loaded();
}

bool ctranslate2_translator_t::is_async() const
{
	return false;
}

bool ctranslate2_translator_t::has_quota() const
{
	return false;
}

int ctranslate2_translator_t::remaining_quota() const
{
	return -1;
}

void ctranslate2_translator_t::translate(const std::string & text, const std::string &)
{
	if (!m_engine.is_loaded())
	{
		emit translation_finished({ "", false, "Model not loaded" });
		return;
	}

	auto result = m_engine.translate(text);
	emit translation_finished({ result.text, result.success, result.error });
}

translation_result_t ctranslate2_translator_t::translate_sync(const std::string & text)
{
	if (!m_engine.is_loaded())
		return { "", false, "Model not loaded" };

	return m_engine.translate(text);
}

bool ctranslate2_translator_t::load_model(const std::string & model_path)
{
	return m_engine.load(model_path);
}
