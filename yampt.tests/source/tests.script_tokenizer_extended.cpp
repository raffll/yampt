#include <catch2/catch_all.hpp>
#include <highlighter/script_tokenizer.hpp>

TEST_CASE("script_tokenizer_t::tokenize, sctx say with two strings", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("say \"file.mp3\" \"Hello world\"", rec_type_t::sctx);

	int string_count = 0;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_string)
			++string_count;
	}
	REQUIRE(string_count == 2);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx choice multiple strings", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("Choice \"Yes\" 1 \"No\" 2 \"Maybe\" 3", rec_type_t::sctx);

	int string_count = 0;
	bool has_choice = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_string)
			++string_count;

		if (token.type == token_type_t::mwscript_function)
			has_choice = true;
	}
	REQUIRE(has_choice);
	REQUIRE(string_count == 3);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx empty input", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("", rec_type_t::sctx);
	REQUIRE(tokens.empty());
}

TEST_CASE("script_tokenizer_t::tokenize, sctx case insensitive keyword", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("MESSAGEBOX \"test\"", rec_type_t::sctx);

	bool has_function = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_function)
			has_function = true;
	}
	REQUIRE(has_function);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx keyword not in variable name", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("set countSays to 1", rec_type_t::sctx);

	for (const auto & token : tokens)
		REQUIRE(token.type != token_type_t::mwscript_function);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx multiline with comment and code", "[u]")
{
	script_tokenizer_t tokenizer;
	const std::string script = "; comment line\nMessageBox \"Hello\"";
	const auto tokens = tokenizer.tokenize(script, rec_type_t::sctx);

	bool has_comment = false;
	bool has_function = false;
	bool has_string = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_comment)
			has_comment = true;

		if (token.type == token_type_t::mwscript_function)
			has_function = true;

		if (token.type == token_type_t::mwscript_string)
			has_string = true;
	}
	REQUIRE(has_comment);
	REQUIRE(has_function);
	REQUIRE(has_string);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx indented comment", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("   ; indented comment", rec_type_t::sctx);

	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0].type == token_type_t::mwscript_comment);
}

TEST_CASE("script_tokenizer_t::tokenize, sctx getpccell with cell name", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("if ( GetPCCell \"Balmora\" == 1 )", rec_type_t::sctx);

	bool has_getpccell = false;
	bool has_string = false;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::mwscript_function && token.end - token.start == 9)
			has_getpccell = true;

		if (token.type == token_type_t::mwscript_string)
			has_string = true;
	}
	REQUIRE(has_getpccell);
	REQUIRE(has_string);
}

TEST_CASE("script_tokenizer_t::tokenize, text self-closing tag", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("<br>text<br>", rec_type_t::text);

	int html_count = 0;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::html_tag)
			++html_count;
	}
	REQUIRE(html_count == 2);
}

TEST_CASE("script_tokenizer_t::tokenize, text closing tag", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("</div>", rec_type_t::text);

	REQUIRE(tokens.size() == 1);
	REQUIRE(tokens[0].type == token_type_t::html_tag);
}

TEST_CASE("script_tokenizer_t::tokenize, text mixed content", "[u]")
{
	script_tokenizer_t tokenizer;
	const auto tokens = tokenizer.tokenize("<p>Hello <b>world</b></p>", rec_type_t::text);

	int html_count = 0;
	int normal_count = 0;
	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::html_tag)
			++html_count;

		if (token.type == token_type_t::normal)
			++normal_count;
	}
	REQUIRE(html_count == 4);
	REQUIRE(normal_count >= 2);
}

TEST_CASE("script_tokenizer_t::tokenize, text no gaps in coverage", "[u]")
{
	script_tokenizer_t tokenizer;
	const std::string input = "<font>test</font>";
	const auto tokens = tokenizer.tokenize(input, rec_type_t::text);

	size_t covered = 0;
	for (const auto & token : tokens)
		covered += (token.end - token.start);

	REQUIRE(covered == input.size());
}
