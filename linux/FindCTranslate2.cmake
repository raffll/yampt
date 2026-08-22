# FindCTranslate2.cmake
# Finds CTranslate2 library.
#
# Search order:
#   1. CTRANSLATE2_ROOT (user override)
#   2. System-installed cmake config (ctranslate2Config.cmake)
#   3. ${PROJECT_SOURCE_DIR}/external/CTranslate2/build/ (local pre-built)
#
# Exports:
#   CTranslate2::CTranslate2 - Imported library target

if(TARGET CTranslate2::CTranslate2)
    return()
endif()

set(_ct2_found FALSE)

# --- Tier 1: CTRANSLATE2_ROOT override ---
if(CTRANSLATE2_ROOT)
    find_library(_ct2_library
        NAMES ctranslate2
        PATHS "${CTRANSLATE2_ROOT}/lib" "${CTRANSLATE2_ROOT}"
        NO_DEFAULT_PATH
    )
    find_path(_ct2_include_dir
        NAMES ctranslate2/translator.h
        PATHS "${CTRANSLATE2_ROOT}/include"
        NO_DEFAULT_PATH
    )

    if(_ct2_library AND _ct2_include_dir)
        set(_ct2_found TRUE)
        add_library(CTranslate2::CTranslate2 SHARED IMPORTED)
        set_target_properties(CTranslate2::CTranslate2 PROPERTIES
            IMPORTED_LOCATION "${_ct2_library}"
            INTERFACE_INCLUDE_DIRECTORIES "${_ct2_include_dir}"
        )
    endif()

    unset(_ct2_library CACHE)
    unset(_ct2_include_dir CACHE)
endif()

# --- Tier 2: System cmake config (lowercase package name) ---
if(NOT _ct2_found)
    find_package(ctranslate2 QUIET CONFIG)
    if(TARGET CTranslate2::ctranslate2)
        set(_ct2_found TRUE)
        if(NOT TARGET CTranslate2::CTranslate2)
            add_library(CTranslate2::CTranslate2 INTERFACE IMPORTED)
            target_link_libraries(CTranslate2::CTranslate2 INTERFACE CTranslate2::ctranslate2)
        endif()
    endif()
endif()

# --- Tier 3: Local pre-built at external/CTranslate2/build/ ---
if(NOT _ct2_found)
    set(_ct2_local_build "${PROJECT_SOURCE_DIR}/external/CTranslate2/build")
    set(_ct2_local_include "${PROJECT_SOURCE_DIR}/external/CTranslate2/include")

    find_library(_ct2_library
        NAMES ctranslate2
        PATHS "${_ct2_local_build}" "${_ct2_local_build}/Release"
        NO_DEFAULT_PATH
    )

    if(_ct2_library AND EXISTS "${_ct2_local_include}")
        set(_ct2_found TRUE)
        add_library(CTranslate2::CTranslate2 SHARED IMPORTED)
        set_target_properties(CTranslate2::CTranslate2 PROPERTIES
            IMPORTED_LOCATION "${_ct2_library}"
            INTERFACE_INCLUDE_DIRECTORIES "${_ct2_local_include};${_ct2_local_build}"
        )
    endif()

    unset(_ct2_library CACHE)
    unset(_ct2_local_build)
    unset(_ct2_local_include)
endif()

# --- Fatal error if not found ---
if(NOT _ct2_found)
    message(FATAL_ERROR
        "CTranslate2 not found. Searched:\n"
        "  1. CTRANSLATE2_ROOT=${CTRANSLATE2_ROOT}\n"
        "  2. System cmake config (find_package(ctranslate2 CONFIG))\n"
        "  3. ${PROJECT_SOURCE_DIR}/external/CTranslate2/build/\n"
        "\n"
        "To resolve, either:\n"
        "  - Install ctranslate2 from your package manager\n"
        "  - Build CTranslate2 at external/CTranslate2/ (cmake -S . -B build && cmake --build build)\n"
        "  - Set -DCTRANSLATE2_ROOT=/path/to/ctranslate2/install"
    )
endif()

unset(_ct2_found)
