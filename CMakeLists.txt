cmake_minimum_required(VERSION 3.21)
project(yampt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "vcpkg toolchain file is required. Set CMAKE_TOOLCHAIN_FILE.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Werror)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)

# --- Dependency Discovery ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Gui)
find_package(Catch2 REQUIRED)
find_package(unofficial-hunspell REQUIRED)
find_package(sentencepiece REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yyjson REQUIRED)
find_package(rapidcheck REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(CTranslate2 REQUIRED)

# --- yampt_core (Static Library) ---
file(GLOB_RECURSE YAMPT_CORE_SOURCES "yampt.core/source/*.cpp")
add_library(yampt_core STATIC ${YAMPT_CORE_SOURCES})
target_include_directories(yampt_core
    PUBLIC yampt.core/source/
    PRIVATE external/CTranslate2/include/ external/CTranslate2/build/
)
target_link_libraries(yampt_core
    PUBLIC CTranslate2::CTranslate2
    PRIVATE sentencepiece::sentencepiece yyjson::yyjson nlohmann_json::nlohmann_json
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
add_executable(yampt_translator ${YAMPT_TRANSLATOR_SOURCES})
set_target_properties(yampt_translator PROPERTIES OUTPUT_NAME yTranslator AUTOMOC ON)
target_include_directories(yampt_translator PRIVATE yampt.translator/source/)
target_link_libraries(yampt_translator PRIVATE yampt_core yampt_qt Qt6::Core Qt6::Widgets Qt6::Network unofficial::hunspell::hunspell)

# --- yampt_editor (Executable) ---
file(GLOB_RECURSE YAMPT_EDITOR_SOURCES "yampt.editor/source/*.cpp")
add_executable(yampt_editor ${YAMPT_EDITOR_SOURCES})
set_target_properties(yampt_editor PROPERTIES OUTPUT_NAME yEditor AUTOMOC ON)
target_include_directories(yampt_editor PRIVATE yampt.editor/source/)
target_link_libraries(yampt_editor PRIVATE yampt_core yampt_qt Qt6::Core Qt6::Widgets Qt6::Network unofficial::hunspell::hunspell)

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
set_target_properties(yampt_tests PROPERTIES AUTOMOC ON)
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
    Catch2::Catch2WithMain
    rapidcheck
    unofficial::hunspell::hunspell
)

enable_testing()
add_test(NAME yampt_tests COMMAND yampt_tests)

# --- Resource Copy ---
if(EXISTS "${PROJECT_SOURCE_DIR}/providers/")
    file(GLOB PROVIDER_FILES "${PROJECT_SOURCE_DIR}/providers/*.json")
    file(COPY ${PROVIDER_FILES} DESTINATION "${CMAKE_BINARY_DIR}/bin/providers/")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/dictionaries/")
    file(COPY "${PROJECT_SOURCE_DIR}/dictionaries/" DESTINATION "${CMAKE_BINARY_DIR}/bin/dictionaries/")
endif()
