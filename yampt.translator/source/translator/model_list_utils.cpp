#include "model_list_utils.hpp"
#include <algorithm>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

namespace model_list_utils {

static QJsonValue navigate_to_node(const QJsonDocument & document, const std::string & models_path)
{
	QJsonValue current;
	if (document.isObject())
		current = QJsonValue(document.object());
	else if (document.isArray())
		current = QJsonValue(document.array());
	else
		return {};

	const auto path = QString::fromStdString(models_path);
	const auto segments = path.split('.');

	for (const auto & segment : segments)
	{
		if (segment.isEmpty())
			continue;

		if (!current.isObject())
			return {};

		current = current.toObject().value(segment);
	}

	return current;
}

static std::string element_id(const QJsonValue & element, const QString & id_key)
{
	if (element.isString())
		return element.toString().toStdString();

	if (!element.isObject())
		return {};

	const auto id_value = element.toObject().value(id_key);
	if (!id_value.isString())
		return {};

	return id_value.toString().toStdString();
}

std::vector<std::string> extract_model_list(const QJsonDocument & document, const model_list_path_t & path)
{
	const auto node = navigate_to_node(document, path.models_path);
	if (!node.isArray())
		return {};

	const auto key = path.models_id_key.empty() ? QStringLiteral("id")
	                                             : QString::fromStdString(path.models_id_key);

	std::vector<std::string> models;
	for (const auto & element : node.toArray())
	{
		auto identifier = element_id(element, key);
		if (identifier.empty())
			continue;

		models.push_back(std::move(identifier));
	}

	return models;
}

std::string choose_selected_model(
    const std::string & previous_model,
    const std::vector<std::string> & new_list,
    const std::string & default_model)
{
	const bool previous_present = std::find(new_list.begin(), new_list.end(), previous_model) != new_list.end();
	if (previous_present)
		return previous_model;

	return default_model;
}

} // namespace model_list_utils
