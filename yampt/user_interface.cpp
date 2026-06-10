#include "user_interface.hpp"
#include "dict_creator.hpp"
#include "dict_merger.hpp"
#include "dict_reader.hpp"
#include "dict_writer.hpp"
#include "esm_converter.hpp"
#include "translation_engine.hpp"

user_interface_t::user_interface_t(std::vector<std::string> & arg)
    : args(arg)
{
	parse_command_line();
	run_command();
}

void user_interface_t::parse_command_line()
{
	std::string command;
	std::vector<std::string> dict_path_reverse;
	if (args.size() > 1)
	{
		for (size_t i = 2; i < args.size(); ++i)
		{
			if (args[i] == "--windows-1250")
			{
				encoding = tools_t::encoding_t::windows_1250;
			}
			else if (args[i] == "-f")
			{
				command = "-f";
			}
			else if (args[i] == "-d")
			{
				command = "-d";
			}
			else if (args[i] == "-o")
			{
				command = "-o";
			}
			else if (args[i] == "-s")
			{
				command = "-s";
			}
			else if (args[i] == "--translate")
			{
				command = "--translate";
			}
			else
			{
				if (command == "-f")
				{
					file_paths.push_back(args[i]);
				}
				if (command == "-d")
				{
					dict_path_reverse.push_back(args[i]);
				}
				if (command == "-o")
				{
					output = args[i];
				}
				if (command == "-s")
				{
					suffix = args[i];
				}
				if (command == "--translate")
				{
					translate_model_path = args[i];
				}
			}
		}
	}

	dict_paths.insert(dict_paths.begin(), dict_path_reverse.rbegin(), dict_path_reverse.rend());
}

void user_interface_t::run_command()
{
	if (args.size() > 1)
	{
		if (args[1] == "--make" && file_paths.size() > 0)
		{
			make_dict_();
		}
		else if (args[1] == "--make-base" && file_paths.size() == 2)
		{
			make_dict_base();
		}
		else if (args[1] == "--merge" && dict_paths.size() > 0)
		{
			merge_dict();
		}
		else if (args[1] == "--convert" && file_paths.size() > 0 && dict_paths.size() > 0)
		{
			convert_esm();
		}
		else if (args[1] == "--create" && file_paths.size() > 0 && dict_paths.size() > 0)
		{
			create_esm();
		}
		else
		{
			tools_t::add_log("[error] syntax error\r\n");
		}
	}
	else
	{
		tools_t::add_log("yampt v0.25\r\n");
	}
}

void user_interface_t::make_dict_()
{
	tools_t::add_log("[info] making dictionaries\r\n");

	const tools_t::dict_t * base_dict = nullptr;
	tools_t::dict_t base_dict_storage;
	if (dict_paths.size() > 0)
	{
		dict_reader_t reader(dict_paths[0]);
		if (reader.is_loaded())
		{
			base_dict_storage = reader.get_dict();
			base_dict = &base_dict_storage;
		}
	}

	for (size_t i = 0; i < file_paths.size(); ++i)
	{
		dict_creator_t creator(file_paths[i], base_dict);

		std::string out_path;
		if (!output.empty())
		{
			out_path = output;
		}
		else
		{
			out_path = creator.get_name().name + ".json";
		}

		dict_writer_t::write(creator.get_dict(), out_path);
	}

	tools_t::add_log("[info] done\r\n");
}

void user_interface_t::make_dict_base()
{
	tools_t::add_log("[info] making \"BASE\" dictionary\r\n");

	translation_engine_t engine;
	translation_engine_t * engine_ptr = nullptr;

	if (!translate_model_path.empty())
	{
		if (engine.load(translate_model_path))
		{
			tools_t::add_log(
			    "[info] translation engine loaded: " + engine.source_language() + " -> " + engine.target_language() +
			    "\r\n");
			engine_ptr = &engine;
		}
		else
		{
			tools_t::add_log("[warning] failed to load translation model from \"" + translate_model_path + "\"\r\n");
		}
	}

	dict_creator_t creator(file_paths[0], file_paths[1], engine_ptr);
	dict_writer_t::write(creator.get_dict(), creator.get_name().name + ".BASE.json");
	tools_t::add_log("[info] done\r\n");
}

void user_interface_t::merge_dict()
{
	if (output.empty())
	{
		tools_t::add_log("[error] --merge requires -o <output_path>\r\n");
		return;
	}

	for (const auto & path : dict_paths)
	{
		const auto ext_pos = path.rfind('.');
		if (ext_pos != std::string::npos)
		{
			std::string ext = path.substr(ext_pos);
			for (auto & c : ext)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			if (ext == ".xml")
			{
				tools_t::add_log("[error] .xml dictionary files are no longer supported: " + path + "\r\n");
				return;
			}
		}
	}

	tools_t::add_log("[info] merging dictionaries\r\n");
	dict_merger_t merger(dict_paths);
	dict_writer_t::write(merger.get_dict(), output);
	tools_t::add_log("[info] done\r\n");
}

void user_interface_t::convert_esm()
{
	tools_t::add_log("[info] converting plugins\r\n");
	dict_merger_t merger(dict_paths);
	for (const auto & file_path : file_paths)
	{
		esm_converter_t converter(file_path, merger, false, suffix, encoding, false);
		if (converter.is_loaded())
		{
			const auto & name = converter.get_name().name + suffix + converter.get_name().ext;
			tools_t::write_file(converter.get_records(), name);
			std::filesystem::last_write_time(name, converter.get_time());
		}
	}
	tools_t::add_log("[info] done\r\n");
}

void user_interface_t::create_esm()
{
	tools_t::add_log("[info] creating plugins\r\n");
	dict_merger_t merger(dict_paths);
	for (const auto & file_path : file_paths)
	{
		esm_converter_t converter(file_path, merger, false, suffix, encoding, true);
		if (converter.is_loaded())
		{
			const auto & name = converter.get_name().name + ".CREATED" + converter.get_name().ext;
			tools_t::create_file(converter.get_records(), name);
			std::filesystem::last_write_time(name, converter.get_time() + std::chrono::seconds(1));
		}
	}
	tools_t::add_log("[info] done\r\n");
}
