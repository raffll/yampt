#include <catch2/catch_all.hpp>
#include <highlighter/script_tokenizer.hpp>

TEST_CASE("script_tokenizer_t::tokenize, sctx comment line", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("; this is a comment", tools_t::rec_type_t::sctx);

	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0].type == token_type_t::mwscript_comment);
	REQUIRE(tokens[0].start == 0);
	REQUIRE(tokens[0].end == 19);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx keyword messagebox", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("MessageBox \"hello\"", tools_t::rec_type_t::sctx);

	bool has_function = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_function)
		{
			has_function = true;
			REQUIRE(token.start == 0);
			REQUIRE(token.end == 10);
		}
	}
	REQUIRE(has_function);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx quoted string", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("set x to \"hello world\"", tools_t::rec_type_t::sctx);

	bool has_string = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_string)
		{
			has_string = true;
			REQUIRE(token.start == 9);
			REQUIRE(token.end == 22);
		}
	}
	REQUIRE(has_string);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx keyword inside string", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("set x to \"messagebox\"", tools_t::rec_type_t::sctx);

	for (const auto & token : tokens)
		REQUIRE(token.type != token_type_t::mwscript_function);
}

TEST_CASE("script_tokenizer_t::tokenize, text html tag", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("<font color=\"red\">hello</font>", tools_t::rec_type_t::text);

	int html_count = 0;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::html_tag)
			++html_count;
	}
	REQUIRE(html_count == 2);
}

TEST_CASE("script_tokenizer_t::tokenize, text non-html angle bracket", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("5 < 10 and 20 > 15", tools_t::rec_type_t::text);

	for (const auto & token : tokens)
		REQUIRE(token.type != token_type_t::html_tag);
}

TEST_CASE("script_tokenizer_t::tokenize, unknown type single normal", "[u]")
{
	script_tokenizer_t highlighter;
	const auto tokens = highlighter.tokenize("anything here", tools_t::rec_type_t::unknown);

	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0].type == token_type_t::normal);
	REQUIRE(tokens[0].start == 0);
	REQUIRE(tokens[0].end == 13);
}
