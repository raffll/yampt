#pragma once

#include <utility/tools.hpp>
#include <string>
#include <vector>

enum class token_type_t
{
	normal,
	mwscript_function,
	mwscript_comment,
	mwscript_string,
	html_tag,
};

struct token_t
{
	size_t start;
	size_t end;
	token_type_t type;
};

class script_tokenizer_t
{
public:
	std::vector<token_t> tokenize(const std::string & text, tools_t::rec_type_t type) const;

private:
	std::vector<token_t> tokenize_sctx(const std::string & text) const;
	std::vector<token_t> tokenize_text(const std::string & text) const;
};
