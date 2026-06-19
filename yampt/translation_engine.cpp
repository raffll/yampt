#include "translation_engine.hpp"
#include "tools.hpp"
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

translation_engine_t::~translation_engine_t()
{
	if (impl_)
		impl_->translator.release();
}

translation_engine_t::translation_engine_t(translation_engine_t &&) noexcept = default;
translation_engine_t & translation_engine_t::operator=(translation_engine_t &&) noexcept = default;

bool translation_engine_t::load(const std::string & model_pack_path)
{
	unload();

	namespace fs = std::filesystem;

	tools_t::add_log("[info] loading translation model \"" + model_pack_path + "\"\r\n");

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
		    model_dir.string(), ctranslate2::Device::CPU, ctranslate2::ComputeType::DEFAULT);
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

	tools_t::add_log("[info] translation model loaded\r\n");
	return true;
}

void translation_engine_t::unload()
{
	impl_->translator.release();
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

static std::vector<std::string> split_sentences(const std::string & text)
{
	std::vector<std::string> sentences;
	std::string current;

	for (size_t i = 0; i < text.size(); ++i)
	{
		current += text[i];

		if (text[i] == '.' || text[i] == '!' || text[i] == '?')
		{
			while (i + 1 < text.size() && (text[i + 1] == '.' || text[i + 1] == '!' || text[i + 1] == '?'))
			{
				++i;
				current += text[i];
			}

			while (i + 1 < text.size() && (text[i + 1] == ' ' || text[i + 1] == '\t'))
			{
				++i;
				current += text[i];
			}

			if (!current.empty())
			{
				sentences.push_back(current);
				current.clear();
			}
		}
	}

	if (!current.empty())
		sentences.push_back(current);

	if (sentences.empty())
		sentences.push_back(text);

	return sentences;
}

translation_result_t translation_engine_t::translate(const std::string & text) const
{
	if (!impl_->translator)
		return { "", false, "model not loaded" };

	if (text.empty())
		return { "", true, "" };

	try
	{
		auto sentences = split_sentences(text);

		std::vector<std::vector<std::string>> token_batch;
		for (const auto & sentence : sentences)
		{
			std::vector<std::string> tokens;
			impl_->source_spm->Encode(sentence, &tokens);
			token_batch.push_back(std::move(tokens));
		}

		std::vector<std::vector<std::string>> target_prefix_batch;
		if (!impl_->target_prefix.empty())
		{
			for (size_t i = 0; i < token_batch.size(); ++i)
				target_prefix_batch.push_back({ impl_->target_prefix });
		}

		ctranslate2::TranslationOptions options;
		options.repetition_penalty = 1.3f;
		options.no_repeat_ngram_size = 3;

		size_t max_input_len = 0;
		for (const auto & t : token_batch)
		{
			if (t.size() > max_input_len)
				max_input_len = t.size();
		}

		options.max_decoding_length = max_input_len * 3 + 10;

		auto results = impl_->translator->translate_batch(
		    token_batch,
		    target_prefix_batch.empty() ? std::vector<std::vector<std::string>> {} : target_prefix_batch,
		    options);

		std::string translated;
		for (size_t i = 0; i < results.size(); ++i)
		{
			if (results[i].hypotheses.empty())
				continue;

			if (!translated.empty())
				translated += ' ';

			translated += impl_->target_spm->DecodePieces(results[i].hypotheses[0]);
		}

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
