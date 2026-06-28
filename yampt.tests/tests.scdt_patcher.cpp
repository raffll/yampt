#include <catch2/catch_all.hpp>
#include "../yampt/utility/tools.hpp"
#include "../yampt/model/scdt_patcher.hpp"

static std::string size_byte(size_t value)
{
	return tools_t::convert_uint_to_string_byte_array(value).substr(0, 1);
}

static std::string size_word(size_t value)
{
	return tools_t::convert_uint_to_string_byte_array(value).substr(0, 2);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, single replacement", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Balmora PL";

	std::string scdt;
	scdt += std::string(5, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(5, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, new_text, false);

	REQUIRE(result.success);
	REQUIRE_FALSE(result.had_false_positive);

	const auto & patched = patcher.get_scdt();
	auto found_pos = patched.find(new_text);
	REQUIRE(found_pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(patched[found_pos - 1]);
	REQUIRE(stored_size == new_text.size());

	REQUIRE(patched.find(new_text, found_pos + new_text.size()) == std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, two sequential replacements", "[u]")
{
	std::string old_first = "Alpha";
	std::string new_first = "AlphaLong";
	std::string old_second = "Beta";
	std::string new_second = "BetaLong";

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_first.size());
	scdt += old_first;
	scdt += std::string(2, '\x00');
	scdt += size_byte(old_second.size());
	scdt += old_second;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result_first = patcher.apply_text_patch(old_first, new_first, false);
	const auto & result_second = patcher.apply_text_patch(old_second, new_second, false);

	REQUIRE(result_first.success);
	REQUIRE(result_second.success);

	const auto & patched = patcher.get_scdt();
	auto pos_first = patched.find(new_first);
	auto pos_second = patched.find(new_second);
	REQUIRE(pos_first != std::string::npos);
	REQUIRE(pos_second != std::string::npos);
	REQUIRE(pos_second > pos_first + new_first.size());
}

TEST_CASE("scdt_patcher_t::apply_text_patch, getpccell standalone", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Bal Molagmer Hall";

	size_t expr_size = 2 + 1 + 1 + old_text.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(expr_size);
	scdt += "X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(1, '\xAB');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, new_text, true);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(patched[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());

	size_t x_pos = patched.rfind('X', cell_pos);
	REQUIRE(x_pos != std::string::npos);

	size_t expr_size_pos = x_pos - 2;
	size_t expected_expr_size = (cell_pos + new_text.size()) - expr_size_pos;
	auto actual_expr_size = static_cast<unsigned char>(patched[expr_size_pos]);
	REQUIRE(actual_expr_size == expected_expr_size);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, getpccell with comparison", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Bal Molagmer Hall";
	std::string comparison = " == 1";

	size_t expr_size = 2 + 1 + 1 + old_text.size() + comparison.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(expr_size);
	scdt += "X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += comparison;
	scdt += std::string(1, '\x00');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, new_text, true);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(patched[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());

	size_t x_pos = patched.rfind('X', cell_pos);
	REQUIRE(x_pos != std::string::npos);

	size_t expr_size_pos = x_pos - 2;
	size_t end_of_expression = cell_pos + new_text.size() + comparison.size();
	size_t expected_expr_size = end_of_expression - expr_size_pos - 1;
	auto actual_expr_size = static_cast<unsigned char>(patched[expr_size_pos]);
	REQUIRE(actual_expr_size == expected_expr_size);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, false-positive retry", "[u]")
{
	std::string old_text = "Cave";
	std::string new_text = "Grotto";

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(99);
	scdt += old_text;
	scdt += std::string(2, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, new_text, false);

	REQUIRE(result.success);
	REQUIRE(result.had_false_positive);

	const auto & patched = patcher.get_scdt();
	auto found_pos = patched.find(new_text);
	REQUIRE(found_pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(patched[found_pos - 1]);
	REQUIRE(stored_size == new_text.size());

	REQUIRE(patched.find("Cave") != std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, multi-segment", "[u]")
{
	std::string seg_old_0 = "Hello";
	std::string seg_old_1 = "World";
	std::string seg_old_2 = "End";
	std::string seg_new_0 = "Witaj";
	std::string seg_new_1 = "Swiecie";
	std::string seg_new_2 = "Koniec";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += size_word(seg_old_0.size());
	scdt += seg_old_0;
	scdt += size_byte(seg_old_1.size() + 1);
	scdt += seg_old_1;
	scdt += size_byte(seg_old_2.size() + 1);
	scdt += seg_old_2;
	scdt += std::string(3, '\x00');

	std::vector<std::string> segments_old = { seg_old_0, seg_old_1, seg_old_2 };
	std::vector<std::string> segments_new = { seg_new_0, seg_new_1, seg_new_2 };

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(segments_old, segments_new);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();

	auto pos_0 = patched.find(seg_new_0);
	REQUIRE(pos_0 != std::string::npos);
	auto size_lo = static_cast<unsigned char>(patched[pos_0 - 2]);
	auto size_hi = static_cast<unsigned char>(patched[pos_0 - 1]);
	size_t stored_first_size = size_lo | (size_hi << 8);
	REQUIRE(stored_first_size == seg_new_0.size());

	auto pos_1 = patched.find(seg_new_1, pos_0);
	REQUIRE(pos_1 != std::string::npos);
	auto stored_second_size = static_cast<unsigned char>(patched[pos_1 - 1]);
	REQUIRE(stored_second_size == seg_new_1.size() + 1);

	auto pos_2 = patched.find(seg_new_2, pos_1);
	REQUIRE(pos_2 != std::string::npos);
	auto stored_third_size = static_cast<unsigned char>(patched[pos_2 - 1]);
	REQUIRE(stored_third_size == seg_new_2.size() + 1);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, unchanged segments", "[u]")
{
	std::string seg_0 = "Keep";
	std::string seg_1 = "Same";
	std::string seg_2 = "Text";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += size_word(seg_0.size());
	scdt += seg_0;
	scdt += size_byte(seg_1.size() + 1);
	scdt += seg_1;
	scdt += size_byte(seg_2.size() + 1);
	scdt += seg_2;
	scdt += std::string(3, '\x00');

	std::vector<std::string> segments_old = { seg_0, seg_1, seg_2 };
	std::vector<std::string> segments_new = { seg_0, seg_1, seg_2 };

	const auto & original_scdt = scdt;

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(segments_old, segments_new);

	REQUIRE(result.success);
	REQUIRE(patcher.get_scdt() == original_scdt);
}
