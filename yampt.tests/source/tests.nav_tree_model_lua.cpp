#include <catch2/catch_all.hpp>
#include <model/nav_tree_model.hpp>
#include <rapidcheck/catch.h>
#include <scanner/conflict_detector.hpp>
#include <rapidcheck.h>

namespace rc {

template<>
struct Arbitrary<handler_class_t>
{
	static Gen<handler_class_t> arbitrary()
	{
		return gen::element(handler_class_t::blocking, handler_class_t::mutating, handler_class_t::passive);
	}
};

template<>
struct Arbitrary<conflict_severity_t>
{
	static Gen<conflict_severity_t> arbitrary()
	{
		return gen::element(
		    conflict_severity_t::blocking, conflict_severity_t::mutating, conflict_severity_t::overlapping);
	}
};

template<>
struct Arbitrary<handler_registration_t>
{
	static Gen<handler_registration_t> arbitrary()
	{
		return gen::build<handler_registration_t>(
		    gen::set(&handler_registration_t::interface_name, gen::nonEmpty<std::string>()),
		    gen::set(&handler_registration_t::method_name, gen::nonEmpty<std::string>()),
		    gen::set(&handler_registration_t::type_argument, gen::arbitrary<std::string>()),
		    gen::set(&handler_registration_t::callback_expression, gen::nonEmpty<std::string>()),
		    gen::set(&handler_registration_t::handler_body, gen::nonEmpty<std::string>()),
		    gen::set(&handler_registration_t::blocking_condition, gen::arbitrary<std::string>()),
		    gen::set(&handler_registration_t::classification, gen::arbitrary<handler_class_t>()),
		    gen::set(&handler_registration_t::script_path, gen::nonEmpty<std::string>()),
		    gen::set(&handler_registration_t::line_number, gen::inRange(1, 10000)),
		    gen::set(&handler_registration_t::mod_name, gen::nonEmpty<std::string>()));
	}
};

} // namespace rc

namespace {

rc::Gen<std::string> gen_interface_name()
{
	return rc::gen::element(
	    std::string("I.AI"),
	    std::string("I.Combat"),
	    std::string("I.Weather"),
	    std::string("I.UI"),
	    std::string("I.Dialogue"),
	    std::string("I.Magic"),
	    std::string("I.Movement"));
}

rc::Gen<handler_conflict_t> gen_conflict()
{
	return rc::gen::exec([]()
	{
		handler_conflict_t conflict;
		conflict.interface_name = *gen_interface_name();
		conflict.method_name = *rc::gen::nonEmpty<std::string>();
		conflict.type_argument = *rc::gen::arbitrary<std::string>();
		conflict.severity = *rc::gen::arbitrary<conflict_severity_t>();

		const auto count = *rc::gen::inRange(2, 5);
		for (int index = 0; index < count; ++index)
			conflict.registrations.push_back(*rc::gen::arbitrary<handler_registration_t>());

		return conflict;
	});
}

} // namespace

TEST_CASE(
    "nav_tree_model_t::set_lua_scan_result, lua section presence matches scan content",
    "[Feature: lua-view-integration][Property 1: Lua section presence]")
{
	rc::prop(
	    "Validates: Requirements 1.1, 1.3, 1.4",
	    []()
	{
		plugin_scan_t scan;
		nav_tree_model_t model(scan);

		const auto scenario = *rc::gen::inRange(0, 3);

		lua_scan_result_t result;

		if (scenario == 0)
		{
			const auto conflict_count = *rc::gen::inRange(1, 6);
			for (int index = 0; index < conflict_count; ++index)
				result.conflicts.push_back(*gen_conflict());
		}
		else if (scenario == 1)
		{
			const auto reg_count = *rc::gen::inRange(1, 10);
			for (int index = 0; index < reg_count; ++index)
				result.registrations.push_back(*rc::gen::arbitrary<handler_registration_t>());
		}

		model.set_lua_scan_result(result);

		const auto root_count = model.rowCount(QModelIndex());

		if (!result.conflicts.empty())
		{
			RC_ASSERT(root_count == 1);

			const auto lua_root = model.index(0, 0, QModelIndex());
			const auto lua_text = model.data(lua_root, Qt::DisplayRole).toString().toStdString();
			RC_ASSERT(lua_text == "Lua Handlers");

			const auto group_count = model.rowCount(lua_root);
			RC_ASSERT(group_count > 0);

			std::set<std::string> interface_names;
			for (const auto & conflict : result.conflicts)
				interface_names.insert(conflict.interface_name);

			RC_ASSERT(group_count == static_cast<int>(interface_names.size()));

			for (int group_row = 0; group_row < group_count; ++group_row)
			{
				const auto group_index = model.index(group_row, 0, lua_root);
				const auto group_text = model.data(group_index, Qt::DisplayRole).toString().toStdString();
				RC_ASSERT(!group_text.empty());
			}
		}
		else if (!result.registrations.empty())
		{
			RC_ASSERT(root_count == 1);

			const auto lua_root = model.index(0, 0, QModelIndex());
			const auto lua_text = model.data(lua_root, Qt::DisplayRole).toString().toStdString();
			RC_ASSERT(lua_text == "Lua Handlers");

			const auto group_count = model.rowCount(lua_root);
			RC_ASSERT(group_count > 0);

			std::set<std::string> mod_names;
			for (const auto & registration : result.registrations)
				mod_names.insert(registration.mod_name);

			RC_ASSERT(group_count == static_cast<int>(mod_names.size()));

			for (int group_row = 0; group_row < group_count; ++group_row)
			{
				const auto group_index = model.index(group_row, 0, lua_root);
				const auto group_text = model.data(group_index, Qt::DisplayRole).toString().toStdString();
				RC_ASSERT(!group_text.empty());
			}
		}
		else
		{
			RC_ASSERT(root_count == 0);
		}
	});
}
