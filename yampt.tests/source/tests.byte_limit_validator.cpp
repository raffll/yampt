#include <catch2/catch_all.hpp>
#include <editor/byte_limit_validator.hpp>

TEST_CASE("byte_limit_validator_t::validate, cell within limit", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::cell, "Balmora");
	REQUIRE(result.level == validation_level_t::ok);
	REQUIRE(result.byte_count == 7);
	REQUIRE(result.limit == 63);
}

TEST_CASE("byte_limit_validator_t::validate, cell exceeds limit", "[u]")
{
	byte_limit_validator_t validator;
	const std::string long_cell(64, 'A');
	const auto result = validator.validate(rec_type_t::cell, long_cell);
	REQUIRE(result.level == validation_level_t::error);
	REQUIRE(result.byte_count == 64);
}

TEST_CASE("byte_limit_validator_t::validate, fnam within limit", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::fnam, "Short Name");
	REQUIRE(result.level == validation_level_t::ok);
	REQUIRE(result.limit == 31);
}

TEST_CASE("byte_limit_validator_t::validate, fnam exceeds limit", "[u]")
{
	byte_limit_validator_t validator;
	const std::string long_name(32, 'X');
	const auto result = validator.validate(rec_type_t::fnam, long_name);
	REQUIRE(result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, info caution range", "[u]")
{
	byte_limit_validator_t validator;
	const std::string text(600, 'A');
	const auto result = validator.validate(rec_type_t::info, text);
	REQUIRE(result.level == validation_level_t::caution);
	REQUIRE(result.byte_count == 600);
}

TEST_CASE("byte_limit_validator_t::validate, info exceeds hard limit", "[u]")
{
	byte_limit_validator_t validator;
	const std::string text(1025, 'A');
	const auto result = validator.validate(rec_type_t::info, text);
	REQUIRE(result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, forbidden character pipe", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::info, "text|with|pipes");
	REQUIRE(result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, forbidden character tilde", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::fnam, "name~bad");
	REQUIRE(result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, forbidden control char", "[u]")
{
	byte_limit_validator_t validator;
	const std::string text = std::string("before") + '\x01' + "after";
	const auto result = validator.validate(rec_type_t::info, text);
	REQUIRE(result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, tab is allowed", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::info, "line1\tline2");
	REQUIRE(result.level == validation_level_t::ok);
}

TEST_CASE("byte_limit_validator_t::validate, quote allowed in script", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::sctx, "say \"hello\"");
	REQUIRE(result.level == validation_level_t::ok);
}

TEST_CASE("byte_limit_validator_t::validate, quote allowed in non-script", "[u]")
{
	byte_limit_validator_t validator;
	const auto result = validator.validate(rec_type_t::info, "He said \"hello\"");
	REQUIRE(result.level == validation_level_t::ok);
}

TEST_CASE("byte_limit_validator_t::validate, rnam at boundary", "[u]")
{
	byte_limit_validator_t validator;
	const std::string exact(32, 'R');
	const auto ok_result = validator.validate(rec_type_t::rnam, exact);
	REQUIRE(ok_result.level == validation_level_t::ok);

	const std::string over(33, 'R');
	const auto err_result = validator.validate(rec_type_t::rnam, over);
	REQUIRE(err_result.level == validation_level_t::error);
}

TEST_CASE("byte_limit_validator_t::validate, no limit for text type", "[u]")
{
	byte_limit_validator_t validator;
	const std::string huge(5000, 'X');
	const auto result = validator.validate(rec_type_t::text, huge);
	REQUIRE(result.level == validation_level_t::ok);
	REQUIRE(result.limit == 0);
}
