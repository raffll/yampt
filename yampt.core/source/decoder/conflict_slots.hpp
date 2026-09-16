#pragma once

#include "sub_record_iter.hpp"
#include <cstddef>
#include <string>
#include <vector>

struct slot_key_t
{
	std::string type;
	int occurrence;
};

struct aligned_slot_t
{
	slot_key_t key;
	std::vector<size_t> indices;
};

#include <cstdint>

struct slot_result_t
{
	std::vector<std::string> contents;
	std::vector<std::vector<sub_record_view_t>> parsed;
	std::vector<aligned_slot_t> aligned;
	std::vector<bool> is_deleted;
	std::vector<std::vector<uint64_t>> ref_identities;
};

namespace conflict_slots {

slot_result_t build(
    const std::string & rec_type,
    const std::vector<std::string> & version_contents,
    const std::vector<bool> & version_deleted);

slot_result_t build(
    const std::string & rec_type,
    std::vector<std::string> && version_contents,
    const std::vector<bool> & version_deleted);

slot_result_t build(
    const std::string & rec_type,
    const std::vector<std::string> & version_contents,
    const std::vector<bool> & version_deleted,
    const std::vector<std::vector<uint64_t>> & ref_identities);

void build_cell(slot_result_t & result);

} // namespace conflict_slots
