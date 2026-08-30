#include "web_response_utils.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace web_response_utils {

QJsonValue json_value_from_string(const std::string & value)
{
	const auto value_qstr = QString::fromStdString(value);

	bool is_integer = false;
	const auto integer_value = value_qstr.toLongLong(&is_integer);
	if (is_integer)
		return QJsonValue(integer_value);

	bool is_double = false;
	const auto double_value = value_qstr.toDouble(&is_double);
	if (is_double)
		return QJsonValue(double_value);

	return QJsonValue(value_qstr);
}

std::string extract_by_path(const QJsonDocument & document, const std::string & response_path)
{
	if (!document.isObject() && !document.isArray())
		return {};

	const auto path = QString::fromStdString(response_path);
	const auto segments = path.split('.');

	QJsonValue current;
	if (document.isObject())
		current = QJsonValue(document.object());
	else
		current = QJsonValue(document.array());

	for (const auto & segment : segments)
	{
		if (segment.isEmpty())
			continue;

		const auto bracket_pos = segment.indexOf('[');
		if (bracket_pos < 0)
		{
			if (!current.isObject())
				return {};

			current = current.toObject().value(segment);
			continue;
		}

		const auto close_pos = segment.indexOf(']');
		if (close_pos <= bracket_pos)
			return {};

		const auto field_name = segment.left(bracket_pos);
		const auto index_str = segment.mid(bracket_pos + 1, close_pos - bracket_pos - 1);

		bool index_ok = false;
		const auto index = index_str.toInt(&index_ok);
		if (!index_ok || index < 0)
			return {};

		if (!field_name.isEmpty())
		{
			if (!current.isObject())
				return {};

			current = current.toObject().value(field_name);
		}

		if (!current.isArray())
			return {};

		const auto array = current.toArray();
		if (index >= array.size())
			return {};

		current = array.at(index);
	}

	if (current.isString())
		return current.toString().toStdString();

	return {};
}

} // namespace web_response_utils
