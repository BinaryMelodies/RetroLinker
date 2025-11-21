
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "common.h"
#include "formats.h"
#include "linker/format.h"
#include "linker/module.h"
#include "linker/module_collector.h"
#include "linker/options.h"
#include "linker/reader.h"
#include "linker/reference.h"
#include "format/binary.h"

using namespace Linker;

using namespace Binary;

static void usage_format(format_specification * fmtspec)
{
	std::cerr <<"\t\t" << fmtspec->documentation << std::endl;
	std::shared_ptr<Linker::OutputFormat> fmt = std::dynamic_pointer_cast<Linker::OutputFormat>(fmtspec->produce());
	if(fmt)
	{
		std::shared_ptr<Linker::OptionCollector> opts = fmt->GetOptions();
		for(auto option : opts->option_list)
		{
			std::cerr << "\t\t\t-S" << option->name << "=<" << option->type_name() << ">\t" << option->description << std::endl;
		}
		for(auto& model : fmt->GetMemoryModelNames())
		{
			std::cerr << "\t\t\t-M" << model.name << "\t" << model.description << std::endl;
		}
		for(auto parameter : fmt->GetLinkerScriptParameterNames())
		{
			std::cerr << "\t\t\t-P" << parameter->name << "=<" << parameter->type_name() << ">\t" << parameter->description << std::endl;
		}
		for(auto& symbol : fmt->GetSpecialSymbolNames())
		{
			std::cerr << "\t\t\t-d" << symbol.name << "\t" << symbol.description << std::endl;
		}
	}
}

void usage(char * argv0)
{
	std::cerr << "Usage: " << argv0 << "[options] <input files>" << std::endl;
	std::cerr << "\t-h" << std::endl << "\t\tDisplay this help page" << std::endl;
	std::cerr << "\t-F<format>" << std::endl << "\t\tSelect output format" << std::endl;
	std::cerr << "\t-o<output file>" << std::endl << "\t\tSpecify output binary file name" << std::endl;
	std::cerr << "\t-M<memory model>" << std::endl << "\t\tSelect memory model, format dependent" << std::endl;
	std::cerr << "\t-T<linker script>" << std::endl;
	std::cerr << "\t-P<parameter>=<value>" << std::endl << "\t\tSet parameter for linker script (syntax: ?parameter?)" << std::endl;
	std::cerr << "\t-S<setting>, -S<setting>=<value>" << std::endl << "\t\tSet parameter, format dependent" << std::endl;
	std::cerr << "\t-d<symbol>, -d<symbol>=<value> -d<symbol>=<segment>:<offset>" << std::endl << "\t\tDefine symbol, including special names such as .entry" << std::endl;
	std::cerr << "\t-$=<char>, -$ <char>" << std::endl << "\t\tSet special character (default: '$')" << std::endl;
	std::cerr << "\t--display-debug-messages" << std::endl << "\t\tPrint information only relevant for linker development" << std::endl;
	std::cerr << "\t--suppress-warnings" << std::endl << "\t\tSuppress printing warnings" << std::endl;

	std::cerr << "List of supported output formats:" << std::endl;
	format_specification * last = nullptr;

	size_t i = 0;
	do
	{
		if(formats[i].documentation != "")
		{
			if(last != nullptr)
			{
				std::cerr << std::endl;
				usage_format(last);
			}
			last = &formats[i];
		}
		std::cerr << "\t" << formats[i].format;
		i += 1;
	}
	while(i < formats_size);
	std::cerr << std::endl;
	usage_format(last);
}

class null_buffer : public std::streambuf
{
protected:
	int overflow(int ch = EOF) override { return ch; }
public:
	static null_buffer the_null_buffer;
};

null_buffer null_buffer::the_null_buffer;

/**
 * @brief The main entry to the linker
 */
int main(int argc, char * argv[])
{
	Module module;
	std::shared_ptr<OutputFormat> format = nullptr;
	bool display_debug_messages = false;
	bool suppress_warnings = false;

	std::vector<std::string> inputs;
	std::string output;
	std::string model;
	std::string linker_script; /* TODO: not actually used */
	std::map<std::string, std::string> options;
	std::map<std::string, std::string> parameters;
	std::map<std::string, Reference> defines;
	char special_char = '$';

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'h')
			{
				/* help */
				usage(argv[0]);
				exit(0);
			}
			else if(argv[i][1] == 'F')
			{
				/* format type */
				format = std::dynamic_pointer_cast<OutputFormat>(FetchFormat(argv[i][2] ? &argv[i][2] : argv[++i]));
				if(format == nullptr)
				{
					Linker::Error << "Error: Unknown output format " << (argv[i][2] ? &argv[i][2] : argv[++i]) << std::endl;
				}
			}
			else if(argv[i][1] == 'o')
			{
				/* output file */
				if(output != "")
				{
					Linker::Error << "Error: Multiple output files provided, using first one" << std::endl;
				}
				else
				{
					output = argv[i][2] ? &argv[i][2] : argv[++i];
				}
			}
			else if(argv[i][1] == 'M')
			{
				/* memory model */
				model = std::string(&argv[i][2]);
			}
			else if(argv[i][1] == 'T')
			{
				/* linker script */
				linker_script = std::string(&argv[i][2]);
			}
			else if(argv[i][1] == 'S')
			{
				/* linker options */
				std::string setting = std::string(&argv[i][2]);
				size_t ix = setting.find('=');
				if(ix == std::string::npos)
					options[setting] = "";
				else
					options[setting.substr(0, ix)] = setting.substr(ix + 1);
			}
			else if(argv[i][1] == 'P')
			{
				/* linker script parameters */
				std::string setting = std::string(&argv[i][2]);
				size_t ix = setting.find('=');
				if(ix == std::string::npos)
					parameters[setting] = "";
				else
					parameters[setting.substr(0, ix)] = setting.substr(ix + 1);
			}
			else if(argv[i][1] == 'd')
			{
				/* define symbol */
				try
				{
					std::string name = std::string(&argv[i][2]);
					size_t ix = name.find('=');
					Reference ref;
					if(ix == std::string::npos)
					{
						ref.offset = offset_t(1);
					}
					else
					{
						size_t ix2 = name.find(':');
						std::string value;
						if(ix2 == std::string::npos)
						{
							value = name.substr(ix + 1);
						}
						else
						{
							ref.segment = name.substr(ix + 1, ix2 - ix - 1);
							value = name.substr(ix2 + 1);
						}
						name = name.substr(0, ix);
						if(value.size() >= 1 && '0' <= value[0] && value[0] <= '9')
						{
//							Linker::Debug << "Debug: ?" << value << std::endl;
							ref.offset = offset_t(stoll(value, nullptr, 0));
//							Linker::Debug << "Debug: ?" << std::get<offset_t>(ref.offset) << std::endl;
						}
						else
						{
							ref.offset = value;
						}
					}
//					Linker::Debug << "Debug: Define " << name << " as " << ref << std::endl;
					defines[name] = ref;
				}
				catch(std::invalid_argument& a)
				{
					Linker::Error << "Error: Unable to parse definition argument " << &argv[i][2] << ", ignoring" << std::endl;
				}
			}
			else if(argv[i][1] == '$')
			{
				/* assembler escape symbol, to extend input object file feature set (for example, segmentation or automatic imports) */
				special_char = argv[i][2] == '=' ? argv[i][3] : argv[i++][0];
			}
			else if(strcmp(argv[i], "--display-debug-messages") == 0)
			{
				display_debug_messages = true;
			}
			else if(strcmp(argv[i], "--suppress-warnings") == 0)
			{
				suppress_warnings = true;
			}
			else
			{
				std::ostringstream message;
				message << "Fatal error: Unknown option `" << argv[i] << "'";
				Linker::FatalError(message.str());
			}
			/* TODO: define new symbols and parameters */
		}
		else
		{
			inputs.push_back(argv[i]);
		}
	}

	if(!display_debug_messages)
	{
		Linker::Debug.rdbuf(&null_buffer::the_null_buffer);
	}

	if(suppress_warnings)
	{
		Linker::Warning.rdbuf(&null_buffer::the_null_buffer);
	}

	if(inputs.size() == 0)
	{
		usage(argv[0]);
		exit(0);
	}

	if(format == nullptr)
	{
		Linker::Warning << "Warning: Unspecified output format, using flat binary" << std::endl;
		format = std::make_shared<BinaryFormat>(0, "");
	}

	ModuleCollector linker;

	linker.SetupOptions(special_char, format);

	for(auto input : inputs)
	{
		std::ifstream in;
		in.open(input, std::ios_base::in | std::ios_base::binary);
		if(!in.is_open())
		{
			std::ostringstream message;
			message << "Fatal error: Unable to open file " << input;
			Linker::FatalError(message.str());
		}
		Reader rd (LittleEndian, &in);

		std::vector<format_description> file_formats;
		DetermineFormat(file_formats, rd);

		std::shared_ptr<InputFormat> input_format = nullptr;

		for(auto& file_format : file_formats)
		{
			input_format = std::dynamic_pointer_cast<InputFormat>(CreateFormat(rd, file_format, ReadLibraryFile));
			if(input_format != nullptr)
			{
				input_format->file_offset = file_format.offset;
				rd.Seek(file_format.offset);
				break; /* already processed */
			}
		}
		if(!input_format)
		{
			std::ostringstream message;
			message << "Fatal error: Unable to process input file " << input << ", file format: ";
			if(file_formats.size() == 0)
			{
				message << "unknown";
			}
			else
			{
				bool not_first = false;
				for(auto& file_format : file_formats)
				{
					if(not_first)
						message << ", ";
					message << file_format.magic.description;
					not_first = true;
				}
			}
			Linker::FatalError(message.str());
		}

		input_format->SetupOptions(format);
		input_format->ProduceModule(linker, rd, input);
		in.close();
	}

	linker.CombineModulesInto(module);

#if DISPLAY_LOGS
	for(auto section : module.Sections())
	{
		Linker::Debug << "Debug: " << section << std::endl;
	}
#endif
#if DISPLAY_LOGS
	for(auto pairs : module.symbols)
	{
		Linker::Debug << "Debug: " << pairs.first << ":" << pairs.second << std::endl;
	}
#endif
#if DISPLAY_LOGS
	for(auto relocation : module.relocations)
	{
		Linker::Debug << "Debug: " << relocation << std::endl;
	}
#endif

	for(auto it : defines)
	{
		module.AddGlobalSymbol(it.first, it.second.ToLocation(module));
//		Location loc;
//		Linker::Debug << "Debug: " << module.FindGlobalSymbol(it.first, loc);
//		Linker::Debug << it.first << " = " << loc << std::endl;
	}

	module.AllocateSymbols();
	format->SetOptions(options); /* extension might depend on options */

	if(output == "")
	{
		if(inputs.size() == 1)
		{
#if 0
			size_t pos = inputs[0].find_last_of(".");
			if(pos == std::string::npos || inputs[0].find_first_of("/", pos) != std::string::npos) /* TODO: UNIX specific */
			{
				output = inputs[0];
			}
			else
			{
				output = inputs[0].substr(0, pos);
			}
#endif
			output = std::filesystem::path(inputs[0]).replace_extension().string();
			output = format->GetDefaultExtension(module, output);
		}
		else
		{
			output = format->GetDefaultExtension(module);
		}
	}

	format->SetModel(model);
	format->SetLinkScript(linker_script, parameters);

	Linker::Debug << "Debug: Generating " << output << std::endl;
	format->GenerateFile(output, module);

	return 0;
}

