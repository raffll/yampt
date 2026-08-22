# FindSentencepiece.cmake
# Finds system-installed sentencepiece via pkg-config.
#
# Exports:
#   sentencepiece::sentencepiece - Imported interface target

if(TARGET sentencepiece::sentencepiece)
    return()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(_SENTENCEPIECE REQUIRED sentencepiece)
pkg_check_modules(_ABSL_STATUS QUIET absl_status)

add_library(sentencepiece::sentencepiece INTERFACE IMPORTED)
set_target_properties(sentencepiece::sentencepiece PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_SENTENCEPIECE_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${_SENTENCEPIECE_LIBRARIES};${_ABSL_STATUS_LIBRARIES}"
    INTERFACE_LINK_DIRECTORIES "${_SENTENCEPIECE_LIBRARY_DIRS};${_ABSL_STATUS_LIBRARY_DIRS}"
)

unset(_SENTENCEPIECE_INCLUDE_DIRS)
unset(_SENTENCEPIECE_LIBRARIES)
unset(_SENTENCEPIECE_LIBRARY_DIRS)
