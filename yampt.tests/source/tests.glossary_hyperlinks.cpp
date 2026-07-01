#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <editor/glossary.hpp>

namespace {

rc::Gen<std::string> gen_at_token()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(1, 8);
		std::string word;
		word.reserve(length);
		for (int index = 0; index < length; ++index)
			word += *rc::gen::inRange('a', 'z');
		return "@" + word;
	});
}

rc::Gen<std::string> gen_separator()
{
	return rc::gen::element<std::string>(" ", "  ", ", ", ". ", "\n", "\r\n");
}

rc::Gen<std::string> gen_plain_word()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(2, 10);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange('a', 'z');
		return result;
	});
}

rc::Gen<std::string> gen_text_with_at_tokens(int token_count)
{
	return rc::gen::exec([token_count]()
	{
		std::string result;
		std::vector<std::string> tokens;

		for (int index = 0; index < token_count; ++index)
		{
			if (!result.empty())
				result += *gen_separator();

			const auto plain = *gen_plain_word();
			result += plain;
			result += *gen_separator();

			const auto token = *gen_at_token();
			tokens.push_back(token);
			result += token;
		}

		return result;
	});
}

} // namespace

TEST_CASE("glossary_t::annotate, at-prefix detection property", "[u]")
{
	rc::prop(
		"all @-prefixed tokens are detected as dial_topic",
		[]()
	{
		const auto token_count = *rc::gen::inRange(1, 6);
		const auto text = *gen_text_with_at_tokens(token_count);

		glossary_t glossary;
		glossary.rebuild({});

		const auto results = glossary.annotate(text, tools_t::rec_type_t::info);

		int detected_count = 0;
		for (const auto & annotation : results)
		{
			if (annotation.kind == annotation_t::dial_topic)
				++detected_count;
		}

		RC_ASSERT(detected_count == token_count);

		for (const auto & annotation : results)
		{
			if (annotation.kind != annotation_t::dial_topic)
				continue;

			RC_ASSERT(annotation.start < text.size());
			RC_ASSERT(annotation.end <= text.size());
			RC_ASSERT(annotation.end > annotation.start);
			RC_ASSERT(text[annotation.start] == '@');

			const auto extracted = text.substr(
				annotation.start, annotation.end - annotation.start);
			RC_ASSERT(extracted == annotation.old_text);
			RC_ASSERT(extracted.size() >= 2);
		}
	});
}
