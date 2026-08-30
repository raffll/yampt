#include "ctranslate2_translator.hpp"
#include <QCoreApplication>

ctranslate2_translator_t::ctranslate2_translator_t(QObject * parent)
    : QObject(parent)
{}

std::string ctranslate2_translator_t::name() const
{
	return QCoreApplication::translate("yTranslator", "CTranslate2 (local)").toStdString();
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
		emit translation_finished(
		    { "", false, QCoreApplication::translate("yTranslator", "Model not loaded").toStdString() });
		return;
	}

	auto result = m_engine.translate(text);
	emit translation_finished({ result.text, result.success, result.error });
}

bool ctranslate2_translator_t::load_model(const std::string & model_path)
{
	return m_engine.load(model_path);
}
