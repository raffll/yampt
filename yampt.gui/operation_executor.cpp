#include "operation_executor.hpp"

#include "../yampt/dict_creator.hpp"
#include "../yampt/dict_merger.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/esm_converter.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

std::string operation_executor_t::make_output_path(const std::string & source_path, const std::string & ext) const
{
	const auto info = QFileInfo(QString::fromStdString(source_path));
	const auto base_name = info.completeBaseName().toStdString();
	const auto timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString();
	const auto dir = get_output_dir();

	return dir + base_name + "-" + timestamp + "." + ext;
}

std::string operation_executor_t::get_output_dir() const
{
	QString dir_path;

	if (!output_dir_.empty())
		dir_path = QString::fromStdString(output_dir_);
	else
		dir_path = QCoreApplication::applicationDirPath() + "/workspace/";

	QDir dir(dir_path);
	if (!dir.exists())
		dir.mkpath(".");

	auto result = dir_path.toStdString();
	if (!result.empty() && result.back() != '/' && result.back() != '\\')
		result += '/';

	return result;
}

operation_executor_t::result_t operation_executor_t::make_dict(const std::string & plugin_path, tools_t::encoding_t encoding)
{
	tools_t::reset_log();

	dict_creator_t creator(plugin_path);

	if (tools_t::has_error())
		return {false, tools_t::get_log(), ""};

	const auto output_path = make_output_path(plugin_path, "json");
	dict_writer_t::write(creator.get_dict(), output_path);

	return {!tools_t::has_error(), tools_t::get_log(), output_path};
}

operation_executor_t::result_t operation_executor_t::make_dict_with_base(const std::string & plugin_path, const tools_t::dict_t & base_dict, tools_t::encoding_t encoding)
{
	tools_t::reset_log();

	dict_creator_t creator(plugin_path, &base_dict);

	if (tools_t::has_error())
		return {false, tools_t::get_log(), ""};

	const auto output_path = make_output_path(plugin_path, "json");
	dict_writer_t::write(creator.get_dict(), output_path);

	return {!tools_t::has_error(), tools_t::get_log(), output_path};
}

operation_executor_t::result_t operation_executor_t::make_base(const std::string & foreign_path, const std::string & native_path,
    const std::string & foreign_lang, const std::string & native_lang)
{
	tools_t::reset_log();

	dict_creator_t creator(foreign_path, native_path);

	if (tools_t::has_error())
		return {false, tools_t::get_log(), ""};

	const auto foreign_info = QFileInfo(QString::fromStdString(foreign_path));
	const auto native_info = QFileInfo(QString::fromStdString(native_path));
	const auto foreign_name = foreign_info.completeBaseName().toStdString();
	const auto native_name = native_info.completeBaseName().toStdString();

	auto fl = foreign_lang.empty() ? "XX" : foreign_lang;
	auto nl = native_lang.empty() ? "XX" : native_lang;

	std::string output_name = foreign_name;
	if (foreign_name != native_name)
		output_name += "+" + native_name;

	const auto timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString();
	output_name += "_BASE_" + fl + "-" + nl + "-" + timestamp;

	const auto output_path = get_output_dir() + output_name + ".json";
	dict_writer_t::write(creator.get_dict(), output_path);

	return {!tools_t::has_error(), tools_t::get_log(), output_path};
}

operation_executor_t::result_t operation_executor_t::convert(const std::string & plugin_path, const std::vector<std::string> & dict_paths, tools_t::encoding_t encoding)
{
	tools_t::reset_log();

	auto reversed = dict_paths;
	std::reverse(reversed.begin(), reversed.end());
	dict_merger_t merger(reversed);

	esm_converter_t converter(plugin_path, merger, false, "", encoding, false);

	if (!converter.is_loaded())
		return {false, tools_t::get_log(), ""};

	const auto info = QFileInfo(QString::fromStdString(plugin_path));
	const auto ext = info.suffix().toStdString();
	const auto output_path = make_output_path(plugin_path, ext);

	tools_t::write_file(converter.get_records(), output_path);

	return {true, tools_t::get_log(), output_path};
}

operation_executor_t::result_t operation_executor_t::create_plugin(const std::string & plugin_path, const std::vector<std::string> & dict_paths, tools_t::encoding_t encoding)
{
	tools_t::reset_log();

	auto reversed = dict_paths;
	std::reverse(reversed.begin(), reversed.end());
	dict_merger_t merger(reversed);

	esm_converter_t converter(plugin_path, merger, false, "", encoding, true);

	if (!converter.is_loaded())
		return {false, tools_t::get_log(), ""};

	const auto info = QFileInfo(QString::fromStdString(plugin_path));
	const auto ext = info.suffix().toStdString();
	const auto output_path = make_output_path(plugin_path, ext);

	tools_t::create_file(converter.get_records(), output_path);

	return {true, tools_t::get_log(), output_path};
}
