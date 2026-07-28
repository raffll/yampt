# FindCTranslate2.cmake
# Custom find module for CTranslate2 library.
#
# Search order:
#   1. CTRANSLATE2_ROOT (user override)
#   2. ${PROJECT_SOURCE_DIR}/external/CTranslate2/build/ (local pre-built)
#   3. find_package(CTranslate2) (system-installed)
#
# Exports:
#   CTranslate2::CTranslate2 - Imported SHARED library target

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
        set(_ct2_include_dirs "${_ct2_include_dir}")
    endif()
endif()

# --- Tier 2: Local pre-built at external/CTranslate2/build/ ---
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
        set(_ct2_include_dirs
            "${_ct2_local_include}"
            "${_ct2_local_build}"
        )
    endif()
endif()

# --- Tier 3: System find_package ---
if(NOT _ct2_found)
    find_package(CTranslate2 QUIET)
    if(CTranslate2_FOUND)
        set(_ct2_found TRUE)
    endif()
endif()

# --- Fatal error if not found ---
if(NOT _ct2_found)
    message(FATAL_ERROR
        "CTranslate2 not found. Searched:\n"
        "  1. CTRANSLATE2_ROOT=${CTRANSLATE2_ROOT}\n"
        "  2. ${PROJECT_SOURCE_DIR}/external/CTranslate2/build/\n"
        "  3. System find_package(CTranslate2)\n"
        "\n"
        "To resolve, either:\n"
        "  - Build CTranslate2 at external/CTranslate2/ (cmake -S . -B build && cmake --build build)\n"
        "  - Set -DCTRANSLATE2_ROOT=/path/to/ctranslate2/install\n"
        "  - Install CTranslate2 system-wide so find_package can locate it"
    )
endif()

# --- Create imported target (Tier 1 and 2 only; Tier 3 already provides it) ---
if(NOT TARGET CTranslate2::CTranslate2)
    add_library(CTranslate2::CTranslate2 SHARED IMPORTED)
    set_target_properties(CTranslate2::CTranslate2 PROPERTIES
        IMPORTED_LOCATION "${_ct2_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${_ct2_include_dirs}"
    )
    if(WIN32)
        set_target_properties(CTranslate2::CTranslate2 PROPERTIES
            IMPORTED_IMPLIB "${_ct2_library}"
        )
    endif()
endif()

# Cleanup internal variables
unset(_ct2_found)
unset(_ct2_library)
unset(_ct2_include_dir)
unset(_ct2_include_dirs)
unset(_ct2_local_build)
unset(_ct2_local_include)
