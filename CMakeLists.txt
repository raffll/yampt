cmake_minimum_required(VERSION 3.21)
project(yampt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Werror -Wno-missing-field-initializers)

if(NOT MSVC)
    set_source_files_properties(yampt.tests/source/tests.record_conflict.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-array-bounds"
    )
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)

# --- Dependency Discovery ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/linux")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Gui Test)
find_package(Catch2 REQUIRED)
find_package(sentencepiece REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yyjson REQUIRED)
find_package(rapidcheck REQUIRED)
find_package(CTranslate2 REQUIRED)

# Hunspell: vcpkg provides unofficial::hunspell::hunspell, system uses pkg-config
find_package(unofficial-hunspell QUIET)
if(TARGET unofficial::hunspell::hunspell)
    add_library(Hunspell::Hunspell ALIAS unofficial::hunspell::hunspell)
else()
    find_package(Hunspell REQUIRED)
endif()

# --- yampt_core (Static Library) ---
file(GLOB_RECURSE YAMPT_CORE_SOURCES "yampt.core/source/*.cpp")
add_library(yampt_core STATIC ${YAMPT_CORE_SOURCES})
target_include_directories(yampt_core
    PUBLIC yampt.core/source/
    PRIVATE external/CTranslate2/include/ external/CTranslate2/build/
)
target_link_libraries(yampt_core
    PUBLIC CTranslate2::CTranslate2
    PRIVATE sentencepiece::sentencepiece yyjson::yyjson nlohmann_json::nlohmann_json Hunspell::Hunspell
)

# --- yampt_cli (Executable) ---
file(GLOB_RECURSE YAMPT_CLI_SOURCES "yampt.cli/source/*.cpp")
add_executable(yampt_cli ${YAMPT_CLI_SOURCES})
set_target_properties(yampt_cli PROPERTIES OUTPUT_NAME yampt)
target_include_directories(yampt_cli PRIVATE yampt.cli/source/)
target_link_libraries(yampt_cli PRIVATE yampt_core)

# --- yampt_qt (Static Library) ---
file(GLOB_RECURSE YAMPT_QT_SOURCES "yampt.qt/source/*.cpp")
add_library(yampt_qt STATIC ${YAMPT_QT_SOURCES})
set_target_properties(yampt_qt PROPERTIES AUTOMOC ON)
target_include_directories(yampt_qt PUBLIC yampt.qt/source/)
target_link_libraries(yampt_qt PUBLIC yampt_core Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)

# --- yampt_translator (Executable) ---
file(GLOB_RECURSE YAMPT_TRANSLATOR_SOURCES "yampt.translator/source/*.cpp")
add_executable(yampt_translator ${YAMPT_TRANSLATOR_SOURCES} resources/icons.qrc)
set_target_properties(yampt_translator PROPERTIES OUTPUT_NAME yampt-translator AUTOMOC ON AUTORCC ON)
target_include_directories(yampt_translator PRIVATE yampt.translator/source/)
target_link_libraries(yampt_translator PRIVATE yampt_core yampt_qt Qt6::Core Qt6::Widgets Qt6::Network Hunspell::Hunspell)

# --- yampt_editor (Executable) ---
file(GLOB_RECURSE YAMPT_EDITOR_SOURCES "yampt.editor/source/*.cpp")
add_executable(yampt_editor ${YAMPT_EDITOR_SOURCES} resources/icons.qrc)
set_target_properties(yampt_editor PROPERTIES OUTPUT_NAME yampt-editor AUTOMOC ON AUTORCC ON)
target_include_directories(yampt_editor PRIVATE yampt.editor/source/)
target_link_libraries(yampt_editor PRIVATE yampt_core yampt_qt Qt6::Core Qt6::Widgets Qt6::Network Hunspell::Hunspell)

# --- yampt_tests (Executable) ---
file(GLOB_RECURSE YAMPT_TESTS_SOURCES "yampt.tests/source/*.cpp")
file(GLOB_RECURSE YAMPT_TRANSLATOR_TEST_SOURCES "yampt.translator/source/*.cpp")
file(GLOB_RECURSE YAMPT_EDITOR_TEST_SOURCES "yampt.editor/source/*.cpp")
list(FILTER YAMPT_TRANSLATOR_TEST_SOURCES EXCLUDE REGEX "main\\.cpp$")
list(FILTER YAMPT_EDITOR_TEST_SOURCES EXCLUDE REGEX "main\\.cpp$")

add_executable(yampt_tests
    ${YAMPT_TESTS_SOURCES}
    ${YAMPT_TRANSLATOR_TEST_SOURCES}
    ${YAMPT_EDITOR_TEST_SOURCES}
)
set_target_properties(yampt_tests PROPERTIES OUTPUT_NAME yampt-tests AUTOMOC ON)
target_include_directories(yampt_tests PRIVATE
    yampt.tests/source/
    yampt.core/source/
    yampt.qt/source/
    yampt.translator/source/
    yampt.editor/source/
    external/
)
target_link_libraries(yampt_tests PRIVATE
    yampt_core
    yampt_qt
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
    Catch2::Catch2WithMain
    rapidcheck
    Hunspell::Hunspell
)

enable_testing()
add_test(NAME yampt_tests COMMAND yampt_tests)

# --- YAMPT_DATA_DIR compile definition ---
set(YAMPT_INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/share/yampt" CACHE STRING "Installed data directory")
target_compile_definitions(yampt_qt PRIVATE "YAMPT_DATA_DIR=\"${YAMPT_INSTALL_DATADIR}\"")

# --- Resource Copy (for local dev builds) ---
if(EXISTS "${PROJECT_SOURCE_DIR}/providers/")
    file(GLOB PROVIDER_FILES "${PROJECT_SOURCE_DIR}/providers/*.json")
    file(COPY ${PROVIDER_FILES} DESTINATION "${CMAKE_BINARY_DIR}/bin/providers/")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/dictionaries/")
    file(COPY "${PROJECT_SOURCE_DIR}/dictionaries/" DESTINATION "${CMAKE_BINARY_DIR}/bin/dictionaries/")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/languages.json")
    file(COPY "${PROJECT_SOURCE_DIR}/languages.json" DESTINATION "${CMAKE_BINARY_DIR}/bin/")
endif()

# --- Install targets ---
include(GNUInstallDirs)

install(TARGETS yampt_cli DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS yampt_translator DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS yampt_editor DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${PROJECT_SOURCE_DIR}/languages.json"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/yampt)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/providers/"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/yampt/providers
    FILES_MATCHING PATTERN "*.json")

if(EXISTS "${PROJECT_SOURCE_DIR}/dictionaries/")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/dictionaries/"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/yampt/dictionaries)
endif()

install(FILES "${PROJECT_SOURCE_DIR}/LICENSE"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/yampt)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/docs/"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    FILES_MATCHING PATTERN "*.md")

install(FILES
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/CHANGELOG.md"
    DESTINATION ${CMAKE_INSTALL_DOCDIR})

# --- Icons and desktop files ---
install(FILES "${PROJECT_SOURCE_DIR}/resources/yampt-translator.svg"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES "${PROJECT_SOURCE_DIR}/resources/yampt-editor.svg"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES "${PROJECT_SOURCE_DIR}/resources/yampt-translator.desktop"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES "${PROJECT_SOURCE_DIR}/resources/yampt-editor.desktop"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

