#pragma once

#include <string>

namespace resource_paths
{

// Shared read-only data
// Linux: ~/.yampt/ -> /usr/share/yampt/ -> exe dir
// Windows: exe dir
std::string data_dir();
std::string languages_file();
std::string providers_dir();
std::string dictionaries_dir();
std::string translations_dir();

// User-writable data
// Linux: ~/.yampt/
// Windows: exe dir
std::string config_dir();
std::string workspace_dir();
std::string models_dir();

} // namespace resource_paths
