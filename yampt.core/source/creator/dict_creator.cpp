#include "dict_creator.hpp"
#include "../translator/translation_engine.hpp"
#include "../utility/app_logger.hpp"
#include "creator_base.hpp"
#include "creator_ordered.hpp"
#include "creator_single.hpp"
#include <hunspell/hunspell.hxx>

dict_creator_t::dict_creator_t(const std::string & plugin_path, const dict_t * base_dict)
    : m_ctx(plugin_path)
{
	m_ctx.base_dict = base_dict;

	if (m_ctx.is_loaded())
	{
		creator_single_t strategy(m_ctx);
		strategy.run();
	}
}

dict_creator_t::dict_creator_t(
    const std::string & path,
    const std::string & path_ext,
    translation_engine_t * translation_engine,
    base_mode_t base_mode,
    const std::string & dictionary_aff_path)
    : m_ctx(path, path_ext)
{
	m_ctx.translation_engine = translation_engine;
	m_ctx.base_mode = base_mode;
	m_ctx.dictionary_aff_path = dictionary_aff_path;

	if (m_ctx.is_both_loaded())
	{
		std::string native_ids;
		native_ids.reserve(m_ctx.esm.get_records().size() * 4);
		for (const auto & rec : m_ctx.esm.get_records())
			native_ids += rec.id;

		std::string foreign_ids;
		foreign_ids.reserve(m_ctx.esm_ext.get_records().size() * 4);
		for (const auto & rec : m_ctx.esm_ext.get_records())
			foreign_ids += rec.id;

		if (native_ids == foreign_ids)
		{
			app_logger_t::add_log("[info] record order: identical\r\n");
			creator_ordered_t strategy(m_ctx);
			strategy.run();
		}
		else
		{
			app_logger_t::add_log("[info] record order: different\r\n");
			creator_base_t strategy(m_ctx);
			strategy.run();
		}
	}
}

dict_creator_t::~dict_creator_t() = default;
