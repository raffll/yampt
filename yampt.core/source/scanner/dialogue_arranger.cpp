#include "dialogue_arranger.hpp"
#include "../utility/record_behavior.hpp"
#include "plugin_scan.hpp"
#include <unordered_map>

namespace dialogue_arranger {

namespace {

constexpr char topic_separator = '|';

enum class slot_kind_t
{
	record,
	topic
};

struct output_slot_t
{
	slot_kind_t kind;
	size_t index;
};

struct topic_bucket_t
{
	std::string topic;
	std::optional<merge_record_t> dial_record;
	std::vector<merge_record_t> info_records;
};

class topic_collector_t
{
public:
	void add_record(const merge_record_t & record, size_t record_index)
	{
		const auto decode_mode = decode_mode_for(record.rec_type);

		if (decode_mode == decode_mode_t::dial)
		{
			bucket_for(record.record_id).dial_record = record;
			return;
		}

		if (decode_mode == decode_mode_t::info)
		{
			bucket_for(topic_of_info(record.record_id)).info_records.push_back(record);
			return;
		}

		m_slots.push_back({ slot_kind_t::record, record_index });
	}

	const std::vector<output_slot_t> & slots() const
	{
		return m_slots;
	}

	const topic_bucket_t & bucket(size_t bucket_index) const
	{
		return m_buckets[bucket_index];
	}

private:
	topic_bucket_t & bucket_for(const std::string & topic)
	{
		const auto [it_bucket, inserted] = m_bucket_lookup.try_emplace(topic, m_buckets.size());
		if (inserted)
		{
			m_buckets.push_back({ topic, std::nullopt, {} });
			m_slots.push_back({ slot_kind_t::topic, it_bucket->second });
		}

		return m_buckets[it_bucket->second];
	}

	std::vector<output_slot_t> m_slots;
	std::vector<topic_bucket_t> m_buckets;
	std::unordered_map<std::string, size_t> m_bucket_lookup;
};

void emit_topic_dial(const topic_bucket_t & bucket, const arrange_input_t & input, arrange_result_t & result)
{
	if (bucket.dial_record)
	{
		result.records.push_back(*bucket.dial_record);
		return;
	}

	const auto dial_content = input.find_topic_dial(bucket.topic);
	if (!dial_content)
	{
		result.missing_topics.push_back(bucket.topic);
		return;
	}

	result.records.push_back({ record_type_for_decode_mode(decode_mode_t::dial), bucket.topic, *dial_content });
	result.inserted_topics.push_back(bucket.topic);
}

std::optional<std::string> find_winner_dial(plugin_scan_t & scan, const std::string & topic)
{
	const auto * dial_entry = scan.find(record_type_for_decode_mode(decode_mode_t::dial), topic);
	if (dial_entry == nullptr)
		return std::nullopt;

	const record_version_t * winner = nullptr;
	for (const auto & version : dial_entry->versions)
	{
		if (scan.is_active_plugin(version.plugin_idx))
			continue;

		if (winner == nullptr || version.plugin_idx > winner->plugin_idx)
			winner = &version;
	}

	if (winner == nullptr)
		return std::nullopt;

	return scan.read_record_content(winner->plugin_idx, winner->record_index);
}

} // namespace

std::string topic_of_info(const std::string & info_record_id)
{
	const auto separator_pos = info_record_id.rfind(topic_separator);
	if (separator_pos == std::string::npos)
		return {};

	return info_record_id.substr(0, separator_pos);
}

arrange_result_t arrange(const arrange_input_t & input)
{
	topic_collector_t collector;
	for (size_t record_index = 0; record_index < input.records.size(); ++record_index)
		collector.add_record(input.records[record_index], record_index);

	arrange_result_t result;
	result.records.reserve(input.records.size());

	for (const auto & slot : collector.slots())
	{
		if (slot.kind == slot_kind_t::record)
		{
			result.records.push_back(input.records[slot.index]);
			continue;
		}

		const auto & bucket = collector.bucket(slot.index);
		emit_topic_dial(bucket, input, result);
		result.records.insert(result.records.end(), bucket.info_records.begin(), bucket.info_records.end());
	}

	return result;
}

arrange_result_t arrange_active(plugin_scan_t & scan)
{
	const auto lookup = [&scan](const std::string & topic) { return find_winner_dial(scan, topic); };
	return arrange({ scan.active_store().records(), lookup });
}

} // namespace dialogue_arranger
