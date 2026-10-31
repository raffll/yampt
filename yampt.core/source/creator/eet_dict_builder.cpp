#include "eet_dict_builder.hpp"
#include "text_match_index.hpp"

namespace {

bool fill_by_exact_id(record_entry_t & native_entry, const chapter_t & pool_chapter)
{
	const auto * pool_entry = pool_chapter.find(native_entry.key_text);
	if (pool_entry == nullptr)
		return false;

	if (pool_entry->status != status_t::translated)
		return false;

	native_entry.new_text = pool_entry->new_text;
	native_entry.status = status_t::translated;
	return true;
}

void fill_by_text_match(record_entry_t & native_entry, const text_match_index_t & index)
{
	const auto outcome = index.find(native_entry.old_text);

	if (outcome.result == text_match_index_t::find_result_t::found)
	{
		native_entry.new_text = outcome.translation;
		native_entry.status = status_t::translated;
		return;
	}

	if (outcome.result == text_match_index_t::find_result_t::ambiguous)
	{
		native_entry.new_text = outcome.translation;
		native_entry.status = status_t::ambiguous;
		return;
	}
}

void fill_native_chapter(chapter_t & native_chapter, const chapter_t & pool_chapter, const text_match_index_t & index)
{
	for (auto & native_entry : native_chapter.records)
	{
		if (fill_by_exact_id(native_entry, pool_chapter))
			continue;

		fill_by_text_match(native_entry, index);
	}
}

void insert_absent_cell_values(chapter_t & native_cell_chapter, const chapter_t & pool_cell_chapter)
{
	for (const auto & pool_entry : pool_cell_chapter.records)
	{
		if (pool_entry.status != status_t::translated)
			continue;

		if (native_cell_chapter.find(pool_entry.key_text) != nullptr)
			continue;

		record_entry_t inserted;
		inserted.key_text = pool_entry.key_text;
		inserted.old_text = pool_entry.old_text;
		inserted.new_text = pool_entry.new_text;
		inserted.status = status_t::translated;
		native_cell_chapter.insert(inserted);
	}
}

} // namespace

dict_t eet_dict_builder_t::build(const dict_t & native_dict, const dict_t & eet_pool)
{
	text_match_index_t index;
	index.build(eet_pool);

	dict_t result = native_dict;

	for (auto & [type, native_chapter] : result)
	{
		const auto it_pool = eet_pool.find(type);
		const auto empty_chapter = chapter_t {};
		const auto & pool_chapter = (it_pool != eet_pool.end()) ? it_pool->second : empty_chapter;
		fill_native_chapter(native_chapter, pool_chapter, index);
	}

	const auto it_pool_cell = eet_pool.find(rec_type_t::cell);
	if (it_pool_cell != eet_pool.end())
		insert_absent_cell_values(result.at(rec_type_t::cell), it_pool_cell->second);

	return result;
}
