#include "fog_fixer.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/tools.hpp"
#include <cstring>

static constexpr uint32_t interior_flag = 0x01;
static constexpr uint32_t behave_exterior_flag = 0x80;
static constexpr size_t data_sub_record_size = 12;
static constexpr size_t ambi_sub_record_size = 16;
static constexpr size_t fog_density_offset = 12;
static constexpr size_t sub_record_header_size = 8;

bool fog_fixer_t::is_interior_cell(const std::string & content)
{
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type != "DATA" || view.size != data_sub_record_size)
			continue;

		uint32_t flags = 0;
		std::memcpy(&flags, view.data, sizeof(uint32_t));
		return (flags & interior_flag) != 0;
	}

	return false;
}

bool fog_fixer_t::has_behave_exterior_flag(const std::string & content)
{
	sub_record_iter_t iter(content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type != "DATA" || view.size != data_sub_record_size)
			continue;

		uint32_t flags = 0;
		std::memcpy(&flags, view.data, sizeof(uint32_t));
		return (flags & behave_exterior_flag) != 0;
	}

	return false;
}

std::string fog_fixer_t::apply(const std::string & content)
{
	if (!is_interior_cell(content))
		return {};

	if (has_behave_exterior_flag(content))
		return {};

	sub_record_iter_t iter(content);
	sub_record_view_t view;
	size_t frmr_offset = 0;
	size_t ambi_write_offset = 0;

	while (iter.next(view))
	{
		if (view.type == "FRMR" && frmr_offset == 0)
			frmr_offset = view.offset;

		if (view.type != "AMBI" || view.size != ambi_sub_record_size)
			continue;

		const auto * fog_bytes = view.data + fog_density_offset;
		if (fog_bytes[0] != 0 || fog_bytes[1] != 0 || fog_bytes[2] != 0 || fog_bytes[3] != 0)
			return {};

		ambi_write_offset = view.offset + sub_record_header_size + fog_density_offset;
	}

	if (ambi_write_offset == 0)
		return {};

	std::string fixed;
	if (frmr_offset > 0)
		fixed = content.substr(0, frmr_offset);
	else
		fixed = content;

	const float default_fog_density = 0.01f;
	std::memcpy(&fixed[ambi_write_offset], &default_fog_density, sizeof(float));

	auto body_size = tools_t::convert_uint_to_string_byte_array(fixed.size() - 16);
	fixed.replace(4, 4, body_size);

	return fixed;
}
