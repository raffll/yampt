#include <catch2/catch_all.hpp>
#include <controller/field_edit_controller.hpp>
#include <scanner/plugin_scan.hpp>
#include <session/plugin_session.hpp>
#include <QSignalSpy>

namespace {

std::string build_sub_record(const std::string & sub_id, const std::string & content)
{
	return sub_id + domain_types::convert_uint_to_string_byte_array(content.size()) + content;
}

std::string build_weapon_record(const std::string & weapon_id, const std::string & display_name)
{
	std::string body;
	body += build_sub_record("NAME", weapon_id + std::string(1, '\0'));
	body += build_sub_record("FNAM", display_name + std::string(1, '\0'));

	std::string header = "WEAP";
	header += domain_types::convert_uint_to_string_byte_array(body.size());
	header += domain_types::convert_uint_to_string_byte_array(0);
	header += domain_types::convert_uint_to_string_byte_array(0);
	return header + body;
}

field_edit_request_t build_name_edit_request(const std::string & record_id, const std::string & input_text)
{
	field_edit_request_t request;
	request.record_type = "WEAP";
	request.record_id = record_id;
	request.sub_type = "FNAM";
	request.field = { "name", field_type_t::string_var, 0, 0, nullptr, nullptr, 0, nullptr };
	request.input_text = input_text;
	return request;
}

merge_lock_t build_whole_record_lock(const std::string & record_id, const std::string & frozen_content)
{
	merge_lock_t lock;
	lock.rec_type = "WEAP";
	lock.record_id = record_id;
	lock.scope = lock_scope_t::whole_record;
	lock.frozen_content = frozen_content;
	return lock;
}

} // namespace

TEST_CASE("field_edit_controller_t::commit_field_edit, edit of a locked record announces the refreshed locks", "[u][qt]")
{
	plugin_session_t session;
	auto & scan = session.scan();
	scan.set_active_plugin("Merged Patch.esp");

	const auto original = build_weapon_record("iron_helm", "Iron Helm");
	scan.copy_record_to_active_raw("WEAP", "iron_helm", original);
	scan.add_active_lock(build_whole_record_lock("iron_helm", original));

	field_edit_controller_t controller(session);
	QSignalSpy locks_spy(&controller, &field_edit_controller_t::locks_resynced);
	REQUIRE(locks_spy.isValid());

	const auto result = controller.commit_field_edit(build_name_edit_request("iron_helm", "Steel Helm"));

	REQUIRE(result.success);
	REQUIRE(locks_spy.count() == 1);
	REQUIRE(scan.active_locks().size() == 1);
	REQUIRE(scan.active_locks()[0].frozen_content == *scan.find_active_content("WEAP", "iron_helm"));
}

TEST_CASE("field_edit_controller_t::commit_field_edit, edit of an unlocked record does not announce locks", "[u][qt]")
{
	plugin_session_t session;
	auto & scan = session.scan();
	scan.set_active_plugin("Merged Patch.esp");

	scan.copy_record_to_active_raw("WEAP", "iron_helm", build_weapon_record("iron_helm", "Iron Helm"));

	field_edit_controller_t controller(session);
	QSignalSpy locks_spy(&controller, &field_edit_controller_t::locks_resynced);
	REQUIRE(locks_spy.isValid());

	const auto result = controller.commit_field_edit(build_name_edit_request("iron_helm", "Steel Helm"));

	REQUIRE(result.success);
	REQUIRE(locks_spy.count() == 0);
}
