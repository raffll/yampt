#include <catch2/catch_all.hpp>
#include <converter/scdt_patcher.hpp>
#include <utility/app_logger.hpp>

static std::string size_byte(size_t value)
{
	return domain_types::convert_uint_to_string_byte_array(value).substr(0, 1);
}

static std::string size_word(size_t value)
{
	return domain_types::convert_uint_to_string_byte_array(value).substr(0, 2);
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
	const auto & result = patcher.apply_text_patch(old_text, { new_text, false, false });

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
	const auto & result_first = patcher.apply_text_patch(old_first, { new_first, false, false });
	const auto & result_second = patcher.apply_text_patch(old_second, { new_second, false, false });

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

	size_t old_expr_size = 4 + old_text.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_expr_size);
	scdt += " X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(1, '\xAB');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, true, false });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(patched[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());

	size_t x_pos = patched.rfind('X', cell_pos);
	REQUIRE(x_pos != std::string::npos);

	size_t expr_size_pos = x_pos - 2;
	size_t expected_expr_size = old_expr_size + new_text.size() - old_text.size();
	auto actual_expr_size = static_cast<unsigned char>(patched[expr_size_pos]);
	REQUIRE(actual_expr_size == expected_expr_size);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, getpccell with comparison", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Bal Molagmer Hall";
	std::string comparison = " == 1";

	size_t old_expr_size = 4 + old_text.size() + comparison.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_expr_size);
	scdt += " X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += comparison;
	scdt += std::string(1, '\x00');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, true, false });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(patched[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());

	size_t x_pos = patched.rfind('X', cell_pos);
	REQUIRE(x_pos != std::string::npos);

	size_t expr_size_pos = x_pos - 2;
	size_t expected_expr_size = old_expr_size + new_text.size() - old_text.size();
	auto actual_expr_size = static_cast<unsigned char>(patched[expr_size_pos]);
	REQUIRE(actual_expr_size == expected_expr_size);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, getpccell short cell name not padded", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Vos";

	size_t old_expr_size = 4 + old_text.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_expr_size);
	scdt += " X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(1, '\xAB');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, true, false });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	auto inner_size = static_cast<unsigned char>(patched[cell_pos - 1]);
	REQUIRE(inner_size == new_text.size());
}

TEST_CASE("scdt_patcher_t::apply_text_patch, getpccell comparison shrink keeps expr size consistent", "[u]")
{
	std::string old_text = "Odai Plateau";
	std::string new_text = "Vos";
	std::string comparison = " == 0";

	size_t old_expr_size = 4 + old_text.size() + comparison.size();

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_expr_size);
	scdt += " X";
	scdt += std::string(1, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += comparison;
	scdt += std::string(1, '\x06');
	scdt += std::string(3, '\x01');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, true, false });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto cell_pos = patched.find(new_text);
	REQUIRE(cell_pos != std::string::npos);

	size_t x_pos = patched.rfind('X', cell_pos);
	size_t expr_size_pos = x_pos - 2;
	auto actual_expr_size = static_cast<unsigned char>(patched[expr_size_pos]);
	size_t expected_expr_size = old_expr_size + new_text.size() - old_text.size();
	REQUIRE(actual_expr_size == expected_expr_size);

	size_t suffix_end = cell_pos + new_text.size() + comparison.size();
	REQUIRE(actual_expr_size == suffix_end - expr_size_pos - 1);
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
	const auto & result = patcher.apply_text_patch(old_text, { new_text, false, false });

	REQUIRE(result.success);
	REQUIRE(result.had_false_positive);

	const auto & patched = patcher.get_scdt();
	auto found_pos = patched.find(new_text);
	REQUIRE(found_pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(patched[found_pos - 1]);
	REQUIRE(stored_size == new_text.size());

	REQUIRE(patched.find("Cave") != std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, does not match inside identifier", "[u]")
{
	std::string identifier = "T_Glob_NineholesBet";
	std::string old_text = "Bet";
	std::string new_text = "Zaklad";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += size_byte(identifier.size());
	scdt += identifier;
	scdt += std::string(2, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, false, false });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();

	REQUIRE(patched.find(identifier) != std::string::npos);

	auto new_pos = patched.find(new_text);
	REQUIRE(new_pos != std::string::npos);
	auto stored_size = static_cast<unsigned char>(patched[new_pos - 1]);
	REQUIRE(stored_size == new_text.size());
}

TEST_CASE("scdt_patcher_t::apply_text_patch, addtopic opcode picks correct occurrence", "[u]")
{
	const std::string shared = "scrib";
	const std::string new_text = "pedrak";
	const std::string place_opcode = std::string("\xe6\x10", 2);
	const std::string topic_opcode = std::string("\x22\x10", 2);

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += place_opcode;
	scdt += size_byte(shared.size());
	scdt += shared;
	scdt += std::string(2, '\x00');
	scdt += topic_opcode;
	scdt += size_byte(shared.size());
	scdt += shared;
	scdt += std::string(3, '\x00');

	text_patch_params_t params;
	params.new_text = new_text;
	params.pad_short_to_minimum = true;
	params.required_opcode = topic_opcode;

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(shared, params);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();

	const auto place_pos = patched.find(place_opcode);
	REQUIRE(place_pos != std::string::npos);
	REQUIRE(patched.compare(place_pos + place_opcode.size() + 1, shared.size(), shared) == 0);

	const auto topic_pos = patched.find(topic_opcode);
	REQUIRE(topic_pos != std::string::npos);
	REQUIRE(patched.compare(topic_pos + topic_opcode.size() + 1, new_text.size(), new_text) == 0);
}

static size_t blob_stored_size_word(const std::string & scdt, size_t blob_pos)
{
	const auto lo = static_cast<unsigned char>(scdt[blob_pos - 2]);
	const auto hi = static_cast<unsigned char>(scdt[blob_pos - 1]);
	return static_cast<size_t>(lo) | (static_cast<size_t>(hi) << 8);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, messagebox single string", "[u]")
{
	const std::string old_blob = "'Hurricane of the North'";
	const std::string new_blob = "'Huragan Polnocy'";

	std::string scdt;
	scdt += std::string(4, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(old_blob.size());
	scdt += old_blob;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(old_blob, new_blob);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(new_blob);
	REQUIRE(pos != std::string::npos);
	REQUIRE(blob_stored_size_word(patched, pos) == new_blob.size());
	REQUIRE(patched.find(old_blob) == std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, messagebox keeps vertical bar literal", "[u]")
{
	const std::string old_blob = "Thorn Fleet Yards, Black Marsh | 3E 371 Empire of Tamriel";
	const std::string new_blob = "Stocznie flotowe Thorna | Cesarstwo Tamriel 3E 371";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(old_blob.size());
	scdt += old_blob;
	scdt += std::string(4, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(old_blob, new_blob);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(new_blob);
	REQUIRE(pos != std::string::npos);
	REQUIRE(blob_stored_size_word(patched, pos) == new_blob.size());
	REQUIRE(patched.find('|') != std::string::npos);
	REQUIRE(patched.find('\x0a') == std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, choice whole argument blob with quotes and numbers", "[u]")
{
	const std::string old_blob = "\"House Hlaalu will support the Empire's claim.\" 11 \"Nevermind.\" 10";
	const std::string new_blob = "\"Rod Hlaalu poprze roszczenia Cesarstwa.\" 11 \"Niewazne.\" 10";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\xc9');
	scdt += std::string(1, '\x10');
	scdt += size_word(old_blob.size());
	scdt += old_blob;
	scdt += std::string(4, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(old_blob, new_blob);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(new_blob);
	REQUIRE(pos != std::string::npos);
	REQUIRE(blob_stored_size_word(patched, pos) == new_blob.size());
	REQUIRE(patched.find(old_blob) == std::string::npos);
	REQUIRE(patched[pos] == '"');
}

TEST_CASE("scdt_patcher_t::apply_message_patch, second occurrence resolved by size field", "[u]")
{
	const std::string old_blob = "Nevermind.";
	const std::string new_blob = "Niewazne.";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += size_word(3);
	scdt += "abc";
	scdt += std::string(2, '\x00');
	scdt += "Nevermind.";
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(old_blob.size());
	scdt += old_blob;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(old_blob, new_blob);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(new_blob);
	REQUIRE(pos != std::string::npos);
	REQUIRE(blob_stored_size_word(patched, pos) == new_blob.size());
}

TEST_CASE("scdt_patcher_t::apply_message_patch, unchanged blob leaves scdt intact", "[u]")
{
	const std::string blob = "Keep this exact.";

	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(blob.size());
	scdt += blob;
	scdt += std::string(3, '\x00');

	const auto original_scdt = scdt;

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(blob, blob);

	REQUIRE(result.success);
	REQUIRE(patcher.get_scdt() == original_scdt);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, blob not present fails", "[u]")
{
	std::string scdt;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(5);
	scdt += "Hello";
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch("Absent", "Nieobecny");

	REQUIRE_FALSE(result.success);
}

TEST_CASE("scdt_patcher_t::apply_message_patch, size field must match blob length", "[u]")
{
	const std::string old_blob = "Yes";
	const std::string new_blob = "Tak";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += size_word(99);
	scdt += old_blob;
	scdt += std::string(2, '\x00');
	scdt += std::string(1, '\x10');
	scdt += size_word(old_blob.size());
	scdt += old_blob;
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_message_patch(old_blob, new_blob);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(new_blob);
	REQUIRE(pos != std::string::npos);
	REQUIRE(blob_stored_size_word(patched, pos) == new_blob.size());
	REQUIRE(patched.find(old_blob) != std::string::npos);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, addtopic pads short text to four bytes", "[u]")
{
	std::string old_text = "Tear";
	std::string new_text = "\xa3za";

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(1, '\x00');
	scdt += size_byte(3);
	scdt += "Tel";
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, false, true });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto pos = patched.find(new_text);
	REQUIRE(pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(patched[pos - 1]);
	REQUIRE(stored_size == 4);
	REQUIRE(patched[pos + new_text.size()] == '\x00');

	auto tel_pos = patched.find("Tel", pos);
	REQUIRE(tel_pos != std::string::npos);
	auto tel_size = static_cast<unsigned char>(patched[tel_pos - 1]);
	REQUIRE(tel_size == 3);
}

TEST_CASE("scdt_patcher_t::apply_text_patch, addtopic exact size for four or more", "[u]")
{
	std::string old_text = "Balmora";
	std::string new_text = "Suran";

	std::string scdt;
	scdt += std::string(3, '\x00');
	scdt += size_byte(old_text.size());
	scdt += old_text;
	scdt += std::string(3, '\x00');
	const auto original_size = scdt.size();

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_text_patch(old_text, { new_text, false, true });

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	auto pos = patched.find(new_text);
	REQUIRE(pos != std::string::npos);

	auto stored_size = static_cast<unsigned char>(patched[pos - 1]);
	REQUIRE(stored_size == new_text.size());
	REQUIRE(patched.size() == original_size - (old_text.size() - new_text.size()));
}

TEST_CASE("scdt_patcher_t::apply_button_patch, patches null-terminated button segment", "[u]")
{
	const std::string message = "Choose a color.";
	const std::string btn_old = "White";
	const std::string btn_new = "Bialy";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += size_word(message.size());
	scdt += message;
	scdt += std::string(1, '\x00');
	scdt += std::string(1, '\x02');
	scdt += size_byte(std::string("Red").size() + 1);
	scdt += "Red";
	scdt += std::string(1, '\x00');
	scdt += size_byte(btn_old.size() + 1);
	scdt += btn_old;
	scdt += std::string(1, '\x00');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & msg_result = patcher.apply_message_patch(message, message);
	REQUIRE(msg_result.success);

	const auto & result = patcher.apply_button_patch(btn_old, btn_new);
	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(btn_new);
	REQUIRE(pos != std::string::npos);

	const auto stored_size = static_cast<unsigned char>(patched[pos - 1]);
	REQUIRE(stored_size == btn_new.size() + 1);
	REQUIRE(patched[pos + btn_new.size()] == '\x00');
}

TEST_CASE("scdt_patcher_t::apply_button_patch, size field must include null terminator", "[u]")
{
	const std::string btn_old = "Yes";
	const std::string btn_new = "Tak";

	std::string scdt;
	scdt += std::string(1, '\x10');
	scdt += size_byte(btn_old.size());
	scdt += btn_old;
	scdt += std::string(2, '\x00');
	scdt += size_byte(btn_old.size() + 1);
	scdt += btn_old;
	scdt += std::string(1, '\x00');
	scdt += std::string(3, '\x00');

	scdt_patcher_t patcher(scdt);
	const auto & result = patcher.apply_button_patch(btn_old, btn_new);

	REQUIRE(result.success);

	const auto & patched = patcher.get_scdt();
	const auto pos = patched.find(btn_new);
	REQUIRE(pos != std::string::npos);
	REQUIRE(static_cast<unsigned char>(patched[pos - 1]) == btn_new.size() + 1);
	REQUIRE(patched.find(btn_old) != std::string::npos);
}
