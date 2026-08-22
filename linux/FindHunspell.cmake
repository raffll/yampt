# FindHunspell.cmake
# Finds system-installed Hunspell via pkg-config.
#
# Exports:
#   Hunspell::Hunspell - Imported interface target

if(TARGET Hunspell::Hunspell)
    return()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(_HUNSPELL REQUIRED hunspell)

add_library(Hunspell::Hunspell INTERFACE IMPORTED)
set_target_properties(Hunspell::Hunspell PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_HUNSPELL_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${_HUNSPELL_LIBRARIES}"
    INTERFACE_LINK_DIRECTORIES "${_HUNSPELL_LIBRARY_DIRS}"
)

unset(_HUNSPELL_INCLUDE_DIRS)
unset(_HUNSPELL_LIBRARIES)
unset(_HUNSPELL_LIBRARY_DIRS)
