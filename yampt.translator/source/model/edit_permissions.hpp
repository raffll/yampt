#pragma once

#include <utility/domain_types.hpp>

struct record_permissions_t
{
	bool editable_in_editor;
	bool editable_inline;
	bool translatable_by_model;
	bool has_byte_limit;
};

struct document_permissions_t
{
	bool editable;
	bool saveable;
	bool exportable;
	bool inline_editable;
	bool status_changeable;
};

namespace edit_permissions {

constexpr record_permissions_t get_record(rec_type_t type)
{
	switch (type)
	{
	case rec_type_t::cell:
		return { true, true, true, true };
	case rec_type_t::dial:
		return { true, true, true, true };
	case rec_type_t::fnam:
		return { true, true, true, true };
	case rec_type_t::rnam:
		return { true, true, true, true };
	case rec_type_t::indx:
		return { true, true, true, true };
	case rec_type_t::info:
		return { true, true, true, true };
	case rec_type_t::gmst:
		return { true, true, true, false };
	case rec_type_t::desc:
		return { true, true, true, false };
	case rec_type_t::text:
		return { true, false, true, false };
	case rec_type_t::bnam:
		return { true, false, true, false };
	case rec_type_t::sctx:
		return { true, false, true, false };
	case rec_type_t::yaml:
		return { true, true, true, false };
	default:
		return { false, false, false, false };
	}
}

} // namespace edit_permissions
