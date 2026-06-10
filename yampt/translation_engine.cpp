#include "translation_engine.hpp"
#include <ctranslate2/translator.h>
#include <sentencepiece_processor.h>
#include <filesystem>
#include <fstream>

struct translation_engine_t::impl_t
{
	std::unique_ptr<ctranslate2::Translator> translator;
	std::unique_ptr<sentencepiece::SentencePieceProcessor> source_spm;
	std::unique_ptr<sentencepiece::SentencePieceProcessor> target_spm;
	std::string source_lang;
	std::string target_lang;
	std::string target_prefix;
};

translation_engine_t::translation_engine_t()
    : impl_(std::make_unique<impl_t>())
{}

translation_engine_t::~translation_engine_t() = default;

translation_engine_t::translation_engine_t(translation_engine_t &&) noexcept = default;
translation_engine_t & translation_engine_t::operator=(translation_engine_t &&) noexcept = default;

bool translation_engine_t::load(const std::string & model_pack_path)
{
	unload();

	namespace fs = std::filesystem;

	if (!fs::exists(model_pack_path))
		return false;

	auto model_dir = fs::path(model_pack_path) / "model";
	if (!fs::is_directory(model_dir))
		return false;

	auto source_spm_path = fs::path(model_pack_path) / "source.spm";
	if (!fs::exists(source_spm_path))
		return false;

	auto target_spm_path = fs::path(model_pack_path) / "target.spm";
	if (!fs::exists(target_spm_path))
		return false;

	try
	{
		impl_->source_spm = std::make_unique<sentencepiece::SentencePieceProcessor>();
		auto status = impl_->source_spm->Load(source_spm_path.string());
		if (!status.ok())
		{
			unload();
			return false;
		}

		impl_->target_spm = std::make_unique<sentencepiece::SentencePieceProcessor>();
		status = impl_->target_spm->Load(target_spm_path.string());
		if (!status.ok())
		{
			unload();
			return false;
		}

		impl_->translator = std::make_unique<ctranslate2::Translator>(
			model_dir.string(),
			ctranslate2::Device::CPU,
			ctranslate2::ComputeType::DEFAULT);
	}
	catch (...)
	{
		unload();
		return false;
	}

	auto dir_name = fs::path(model_pack_path).filename().string();
	auto sep = dir_name.find('-');
	if (sep != std::string::npos)
	{
		impl_->source_lang = dir_name.substr(0, sep);
		impl_->target_lang = dir_name.substr(sep + 1);
	}

	auto prefix_path = fs::path(model_pack_path) / "target_prefix.txt";
	if (fs::exists(prefix_path))
	{
		std::ifstream f(prefix_path);
		std::getline(f, impl_->target_prefix);
	}

	return true;
}

void translation_engine_t::unload()
{
	impl_->translator.reset();
	impl_->source_spm.reset();
	impl_->target_spm.reset();
	impl_->source_lang.clear();
	impl_->target_lang.clear();
}

bool translation_engine_t::is_loaded() const
{
	return impl_->translator != nullptr;
}

std::string translation_engine_t::source_language() const
{
	return impl_->source_lang;
}

std::string translation_engine_t::target_language() const
{
	return impl_->target_lang;
}

translation_result_t translation_engine_t::translate(const std::string & text) const
{
	if (!impl_->translator)
		return { "", false, "model not loaded" };

	if (text.empty())
		return { "", true, "" };

	try
	{
		std::vector<std::string> tokens;
		impl_->source_spm->Encode(text, &tokens);

		if (!impl_->target_prefix.empty())
			tokens.insert(tokens.begin(), impl_->target_prefix);

		auto results = impl_->translator->translate_batch({ tokens });

		if (results.empty() || results[0].hypotheses.empty())
			return { "", false, "empty translation result" };

		auto translated = impl_->target_spm->DecodePieces(results[0].hypotheses[0]);
		return { translated, true, "" };
	}
	catch (const std::exception & e)
	{
		return { "", false, e.what() };
	}
	catch (...)
	{
		return { "", false, "unknown translation error" };
	}
}
