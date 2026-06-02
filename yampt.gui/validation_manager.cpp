#include "validation_manager.hpp"

validation_result_t validation_manager_t::validate(tools_t::rec_type_t type,
                                                   const std::string & value) const
{
    const size_t byte_count = value.size();

    if (type == tools_t::rec_type_t::CELL)
    {
        if (byte_count > 63)
            return { validation_level_t::error, byte_count, 63 };
        return { validation_level_t::ok, byte_count, 63 };
    }

    if (type == tools_t::rec_type_t::FNAM)
    {
        if (byte_count > 31)
            return { validation_level_t::error, byte_count, 31 };
        return { validation_level_t::ok, byte_count, 31 };
    }

    if (type == tools_t::rec_type_t::RNAM)
    {
        if (byte_count > 32)
            return { validation_level_t::error, byte_count, 32 };
        return { validation_level_t::ok, byte_count, 32 };
    }

    if (type == tools_t::rec_type_t::INFO)
    {
        if (byte_count > 1024)
            return { validation_level_t::error, byte_count, 1024 };
        if (byte_count > 512)
            return { validation_level_t::caution, byte_count, 512 };
        return { validation_level_t::ok, byte_count, 512 };
    }

    return { validation_level_t::ok, byte_count, 0 };
}
