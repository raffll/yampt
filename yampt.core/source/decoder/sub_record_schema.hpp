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
	raw
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
};

struct sub_record_schema_t
{
	const char * parent_type;
	const char * sub_type;
	size_t expected_size;
	const field_def_t * fields;
	size_t field_count;
};

const sub_record_schema_t * find_schema(
    const std::string & record_type,
    const std::string & sub_type,
    size_t data_size);

const std::vector<sub_record_schema_t> & all_schemas();

const char * effect_name_by_index(int index);
const char * skill_name_by_index(int index);
