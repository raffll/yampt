#pragma once

#include <string>
#include <QJsonValue>

class QJsonDocument;

namespace web_response_utils {

QJsonValue json_value_from_string(const std::string & value);

std::string extract_by_path(const QJsonDocument & document, const std::string & response_path);

} // namespace web_response_utils
