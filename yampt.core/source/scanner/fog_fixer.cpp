#include "fog_fixer.hpp"
#include "../decoder/sub_record_iter.hpp"
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

	while (iter.next(view))
	{
		if (view.type != "AMBI" || view.size != ambi_sub_record_size)
			continue;

		const auto * fog_bytes = view.data + fog_density_offset;
		if (fog_bytes[0] != 0 || fog_bytes[1] != 0 || fog_bytes[2] != 0 || fog_bytes[3] != 0)
			return {};

		auto fixed = content;
		const float default_fog_density = 0.01f;
		const auto write_offset = view.offset + sub_record_header_size + fog_density_offset;
		std::memcpy(&fixed[write_offset], &default_fog_density, sizeof(float));
		return fixed;
	}

	return {};
}
