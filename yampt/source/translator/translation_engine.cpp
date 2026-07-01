#include "translation_engine.hpp"
#include "../utility/tools.hpp"
#include <ctranslate2/translator.h>
#include <filesystem>
#include <fstream>
#include <sentencepiece_processor.h>

struct translation_engine_t::impl_t
{
	std::unique_ptr<ctranslate2::Translator> translator;
	std::unique_ptr<sentencepiece::SentencePieceProcessor> spm;
	std::string source_lang;
	std::string target_lang;
};

translation_engine_t::translation_engine_t()
    : m_impl(std::make_unique<impl_t>())
{}

translation_engine_t::~translation_engine_t()
{
	if (m_impl)
		m_impl->translator.release();
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

	auto spm_path = fs::path(model_pack_path) / "sentencepiece.bpe.model";
	if (!fs::exists(spm_path))
		return false;

	try
	{
		m_impl->spm = std::make_unique<sentencepiece::SentencePieceProcessor>();
		auto status = m_impl->spm->Load(spm_path.string());
		if (!status.ok())
		{
			unload();
			return false;
		}

		m_impl->translator = std::make_unique<ctranslate2::Translator>(
		    model_dir.string(), ctranslate2::Device::CPU, ctranslate2::ComputeType::DEFAULT);
	}
	catch (...)
	{
		unload();
		return false;
	}

	auto lang_path = fs::path(model_pack_path) / "languages.txt";
	if (fs::exists(lang_path))
	{
		std::ifstream f(lang_path);
		std::getline(f, m_impl->source_lang);
		std::getline(f, m_impl->target_lang);
	}

	if (m_impl->source_lang.empty())
		m_impl->source_lang = "eng_Latn";

	tools_t::add_log("[info] translation model loaded: " + m_impl->source_lang + " -> " + m_impl->target_lang + "\r\n");
	return true;
}

void translation_engine_t::unload()
{
	m_impl->translator.release();
	m_impl->spm.reset();
	m_impl->source_lang.clear();
	m_impl->target_lang.clear();
}

bool translation_engine_t::is_loaded() const
{
	return m_impl->translator != nullptr;
}

std::string translation_engine_t::source_language() const
{
	return m_impl->source_lang;
}

std::string translation_engine_t::target_language() const
{
	return m_impl->target_lang;
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
	if (!m_impl->translator)
		return { "", false, "model not loaded" };

	if (text.empty())
		return { "", true, "" };

	if (m_impl->target_lang.empty())
		return { "", false, "target language not set" };

	try
	{
		auto sentences = split_sentences(text);

		std::vector<std::vector<std::string>> token_batch;
		for (const auto & sentence : sentences)
		{
			std::vector<std::string> tokens;
			m_impl->spm->Encode(sentence, &tokens);
			tokens.insert(tokens.begin(), m_impl->source_lang);
			tokens.push_back("</s>");
			token_batch.push_back(std::move(tokens));
		}

		std::vector<std::vector<std::string>> target_prefix_batch;
		for (size_t i = 0; i < token_batch.size(); ++i)
			target_prefix_batch.push_back({ m_impl->target_lang });

		ctranslate2::TranslationOptions options;
		options.beam_size = 4;
		options.length_penalty = 0.6f;
		options.repetition_penalty = 1.5f;
		options.no_repeat_ngram_size = 3;

		size_t max_input_len = 0;
		for (const auto & t : token_batch)
		{
			if (t.size() > max_input_len)
				max_input_len = t.size();
		}

		options.max_decoding_length = max_input_len * 3 + 10;

		auto results = m_impl->translator->translate_batch(token_batch, target_prefix_batch, options);

		std::string translated;
		for (size_t i = 0; i < results.size(); ++i)
		{
			if (results[i].hypotheses.empty())
				continue;

			auto hypothesis = results[i].hypotheses[0];
			if (!hypothesis.empty() && hypothesis[0] == m_impl->target_lang)
				hypothesis.erase(hypothesis.begin());

			if (!translated.empty())
				translated += ' ';

			translated += m_impl->spm->DecodePieces(hypothesis);
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
