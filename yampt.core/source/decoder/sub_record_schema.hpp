#pragma once
#include <cstddef>
#include <string>
#include <vector>

enum class field_type_t
{
	u8,
	u16,
	u32,
	i8,
	i16,
	i32,
	f32,
	string_fixed,
	string_var,
	flags_u8,
	flags_u16,
	flags_u32,
	enum_u8,
	enum_u16,
	enum_u32,
	bool_bit,
	binary,
	raw,
	scvr_type,
	scvr_operator,
	scvr_subject,
	global_type
};

struct field_def_t
{
	const char * name;
	field_type_t type;
	size_t offset;
	size_t size;
	const char * const * enum_names;
	const char * const * flag_names;
	int flag_count;
	const char * group;
};

struct sub_record_schema_t
{
	const char * parent_type;
	const char * sub_type;
	size_t expected_size;
	const field_def_t * fields;
	size_t field_count;
	bool repeatable = false;
	const char * label = nullptr;
};

const sub_record_schema_t * find_schema(
    const std::string & record_type,
    const std::string & sub_type,
    size_t data_size);

const sub_record_schema_t * find_largest_schema(const std::string & record_type, const std::string & sub_type);

const sub_record_schema_t * find_cell_data_schema(const char * data, size_t data_size);

bool has_content_dependent_schema(const std::string & record_type, const std::string & sub_type);
const sub_record_schema_t * content_dependent_schema(
    const std::string & record_type,
    const std::string & sub_type,
    const char * data,
    size_t data_size);

const field_def_t * find_field_by_name(const sub_record_schema_t & schema, const char * field_name);

const std::vector<sub_record_schema_t> & all_schemas();

namespace enam_layout
{
constexpr size_t slot_size = 24;
constexpr size_t identity_prefix_length = 8;
constexpr size_t magnitude_min_offset = 16;
constexpr size_t magnitude_max_offset = 20;
constexpr size_t magnitude_field_size = 4;
} // namespace enam_layout

namespace npco_layout
{
constexpr size_t record_size = 36;
constexpr size_t item_id_offset = 4;
constexpr size_t item_id_length = 32;
} // namespace npco_layout

namespace npcs_layout
{
constexpr size_t record_size = 32;
} // namespace npcs_layout

namespace fact_layout
{
constexpr size_t reaction_value_size = 4;
} // namespace fact_layout

namespace record_layout
{
constexpr size_t header_size = 16;
constexpr size_t size_field_offset = 4;
constexpr size_t size_field_length = 4;
} // namespace record_layout

namespace object_index_layout
{
constexpr size_t index_size = 4;
} // namespace object_index_layout

namespace leveled_layout
{
constexpr size_t level_size = 2;
} // namespace leveled_layout

enum class sub_record_kind_t
{
	single_value,
	multi_value,
	repeatable
};

struct record_sub_record_t
{
	const char * sub_type;
	const char * label;
	sub_record_kind_t kind;
};

const std::vector<record_sub_record_t> & record_composition(const std::string & record_type);

const char * effect_name_by_index(int index);
const char * skill_name_by_index(int index);
