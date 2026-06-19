#include "ctranslate2_provider.hpp"

ctranslate2_provider_t::ctranslate2_provider_t(QObject * parent)
    : QObject(parent)
{
}

std::string ctranslate2_provider_t::name() const
{
    return "CTranslate2 (local)";
}

bool ctranslate2_provider_t::is_available() const
{
    return engine_.is_loaded();
}

bool ctranslate2_provider_t::is_async() const
{
    return false;
}

bool ctranslate2_provider_t::has_quota() const
{
    return false;
}

int ctranslate2_provider_t::remaining_quota() const
{
    return -1;
}

void ctranslate2_provider_t::translate(const std::string & text, const std::string &)
{
    if (!engine_.is_loaded())
    {
        emit translation_finished({"", false, "Model not loaded"});
        return;
    }

    auto result = engine_.translate(text);
    emit translation_finished({result.text, result.success, result.error});
}

bool ctranslate2_provider_t::load_model(const std::string & model_path)
{
    return engine_.load(model_path);
}
