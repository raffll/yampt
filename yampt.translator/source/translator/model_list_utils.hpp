#pragma once

#include <string>
#include <vector>

class QJsonDocument;

namespace model_list_utils {

struct model_list_path_t
{
	std::string models_path;
	std::string models_id_key;
};

std::vector<std::string> extract_model_list(const QJsonDocument & document, const model_list_path_t & path);

std::string choose_selected_model(
    const std::string & previous_model,
    const std::vector<std::string> & new_list,
    const std::string & default_model);

} // namespace model_list_utils
