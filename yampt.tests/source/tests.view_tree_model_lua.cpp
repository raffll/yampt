#include <catch2/catch_all.hpp>
#include <model/view_tree_model.hpp>
#include <rapidcheck/catch.h>
#include <scanner/conflict_detector.hpp>
#include <rapidcheck.h>

namespace rc {

Gen<std::string> gen_printable_ascii_nonempty()
{
	return gen::nonEmpty(gen::container<std::string>(gen::inRange<char>(0x20, 0x7F)));
}

Gen<std::string> gen_printable_ascii()
{
	return gen::container<std::string>(gen::inRange<char>(0x20, 0x7F));
}

template<>
struct Arbitrary<handler_class_t>
{
	static Gen<handler_class_t> arbitrary()
	{
		return gen::element(handler_class_t::blocking, handler_class_t::mutating, handler_class_t::passive);
	}
};

template<>
struct Arbitrary<handler_registration_t>
{
	static Gen<handler_registration_t> arbitrary()
	{
		return gen::build<handler_registration_t>(
		    gen::set(&handler_registration_t::interface_name, gen_printable_ascii_nonempty()),
		    gen::set(&handler_registration_t::method_name, gen_printable_ascii_nonempty()),
		    gen::set(&handler_registration_t::type_argument, gen_printable_ascii()),
		    gen::set(&handler_registration_t::callback_expression, gen_printable_ascii_nonempty()),
		    gen::set(&handler_registration_t::handler_body, gen_printable_ascii_nonempty()),
		    gen::set(&handler_registration_t::blocking_condition, gen_printable_ascii()),
		    gen::set(&handler_registration_t::classification, gen::arbitrary<handler_class_t>()),
		    gen::set(&handler_registration_t::script_path, gen_printable_ascii_nonempty()),
		    gen::set(&handler_registration_t::line_number, gen::inRange(1, 10000)),
		    gen::set(&handler_registration_t::mod_name, gen_printable_ascii_nonempty()));
	}
};

} // namespace rc

namespace {

std::string classification_text(handler_class_t classification)
{
	switch (classification)
	{
	case handler_class_t::blocking:
		return "Blocking";

	case handler_class_t::mutating:
		return "Mutating";

	case handler_class_t::passive:
		return "Passive";
	}

	return "";
}

rc::Gen<handler_conflict_t> gen_conflict()
{
	return rc::gen::exec([]()
	{
		handler_conflict_t conflict;
		conflict.interface_name = *rc::gen_printable_ascii_nonempty();
		conflict.method_name = *rc::gen_printable_ascii_nonempty();
		conflict.type_argument = *rc::gen_printable_ascii();
		conflict.severity = *rc::gen::element(
		    conflict_severity_t::blocking, conflict_severity_t::mutating, conflict_severity_t::overlapping);

		const auto count = *rc::gen::inRange(2, 6);
		for (int index = 0; index < count; ++index)
			conflict.registrations.push_back(*rc::gen::arbitrary<handler_registration_t>());

		return conflict;
	});
}

bool rows_contain_value(const std::vector<view_tree_model_t::view_node_t> & rows, const std::string & value)
{
	for (const auto & row : rows)
	{
		for (const auto & cell : row.values)
		{
			if (cell == value)
				return true;
		}
	}

	return false;
}

} // namespace

TEST_CASE(
    "view_tree_model_t::set_lua_conflict, conflict detail completeness",
    "[Feature: lua-view-integration][Property 4: Conflict detail completeness]")
{
	rc::prop(
	    "Validates: Requirements 2.1, 2.2",
	    []()
	{
		const auto conflict = *gen_conflict();
		const auto reg_count = conflict.registrations.size();

		view_tree_model_t model;
		model.set_lua_conflict(conflict);

		RC_ASSERT(model.columnCount(QModelIndex()) == static_cast<int>(reg_count) + 1);

		const auto & rows = model.rows();
		RC_ASSERT(rows.size() == 7u);

		for (size_t col = 0; col < reg_count; ++col)
		{
			const auto & reg = conflict.registrations[col];
			const auto section = static_cast<int>(col) + 1;

			const auto header = model.headerData(section, Qt::Horizontal, Qt::DisplayRole).toString().toStdString();
			RC_ASSERT(header == reg.mod_name);

			RC_ASSERT(rows[3].values[col] == classification_text(reg.classification));
			RC_ASSERT(rows[4].values[col] == reg.script_path);
			RC_ASSERT(rows[5].values[col] == reg.callback_expression);
			RC_ASSERT(rows[6].values[col] == reg.handler_body);
		}
	});
}

TEST_CASE(
    "view_tree_model_t::set_lua_registration, registration detail completeness",
    "[Feature: lua-view-integration][Property 5: Registration detail completeness]")
{
	rc::prop(
	    "rows contain interface name, method name, type argument, script path, and handler body",
	    []()
	{
		const auto registration = *rc::gen::arbitrary<handler_registration_t>();

		view_tree_model_t model;
		model.set_lua_registration(registration);

		const auto & rows = model.rows();
		RC_ASSERT(!rows.empty());
		RC_ASSERT(rows_contain_value(rows, registration.interface_name));
		RC_ASSERT(rows_contain_value(rows, registration.method_name));
		RC_ASSERT(rows_contain_value(rows, registration.type_argument));
		RC_ASSERT(rows_contain_value(rows, registration.script_path));
		RC_ASSERT(rows_contain_value(rows, registration.handler_body));
	});
}
