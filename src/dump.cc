
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "common.h"
#include "formats.h"
#include "linker/format.h"
#include "linker/reader.h"
#include "dumper/dumper.h"

using namespace Linker;

void usage(char * argv0)
{
	std::cerr << "Usage: " << argv0 << "[options] <input file>" << std::endl;
	std::cerr << "\t-h" << std::endl << "\t\tDisplay this help page" << std::endl;
	std::cerr << "\t-F<format>" << std::endl << "\t\tSelect output format" << std::endl;

	std::cerr << "List of supported formats:" << std::endl;
	format_specification * last = nullptr;

	size_t i = 0;
	do
	{
		if(formats[i].documentation != "")
		{
			if(last != nullptr)
			{
				std::cerr << std::endl << "\t\t" << last->documentation << std::endl;
			}
			last = &formats[i];
		}
		std::cerr << "\t" << formats[i].format;
		i += 1;
	}
	while(i < formats_size);
	std::cerr << std::endl << "\t\t" << last->documentation << std::endl;
}

/**
 * @brief The main entry to the dumper
 */
int main(int argc, char * argv[])
{
	std::string input = "";
	std::shared_ptr<Format> format = nullptr;
	int show_options = 0;
	int hide_options = 0;

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'h')
			{
				usage(argv[0]);
				exit(0);
			}
			else if(argv[i][1] == 'F')
			{
				/* TODO: FetchFormat with another table for input formats, enable setting system type */
				format = FetchFormat(argv[i][2] ? &argv[i][2] : argv[++i]);
				/* TODO: enable selecting a format within the determined formats, or force parsing a format at a specified address */
			}
			else if(memcmp(argv[i], "--on-overflow=", 14) == 0 || strcmp(argv[i], "--on-overflow") == 0)
			{
				char * option;
				if(strlen(argv[i]) < 14)
				{
					if(++i < argc)
					{
						option = argv[i];
					}
					else
					{
						Linker::FatalError("Fatal error: --on-overflow expects parameter");
					}
				}
				else
				{
					option = &argv[i][14];
				}

				if(strcmp(option, "default") == 0)
				{
					Linker::Debug << "Debug: Read overflow behavior set to `default'" << std::endl;
					Linker::Reader::global_overflow_behavior = Linker::Reader::OverflowHandlingRequest::Default;
				}
				else if(strcmp(option, "force") == 0)
				{
					Linker::Debug << "Debug: Read overflow behavior set to `force', all instances of overflow will be ignored" << std::endl;
					Linker::Reader::global_overflow_behavior = Linker::Reader::OverflowHandlingRequest::Force;
				}
				else if(strcmp(option, "report") == 0)
				{
					Linker::Debug << "Debug: Read overflow behavior set to `report', all instances of overflow will terminate execution" << std::endl;
					Linker::Reader::global_overflow_behavior = Linker::Reader::OverflowHandlingRequest::Report;
				}
				else
				{
					std::ostringstream oss;
					oss << "Fatal error: Unknown action on read overflow: `" << option << "'";
					Linker::FatalError(oss.str());
				}
			}
			else if(memcmp(argv[i], "--show-", 7) == 0 || memcmp(argv[i], "--hide-", 7) == 0)
			{
				bool show = argv[i][2] != 'h';
				char * flag = &argv[i][7];
				int option = 0;
				if(strcmp(flag, "all") == 0)
				{
					option = Dumper::All;
				}
				else if(strcmp(flag, "data") == 0 || strcmp(flag, "image") == 0)
				{
					option = Dumper::Image;
				}
				else if(strcmp(flag, "header") == 0 || strcmp(flag, "headers") == 0)
				{
					option = Dumper::Header;
				}
				else if(strcmp(flag, "symbol") == 0 || strcmp(flag, "symbols") == 0)
				{
					option = Dumper::Symbol;
				}
				else if(strcmp(flag, "reloc") == 0 || strcmp(flag, "relocs") == 0 || strcmp(flag, "relocation") == 0 || strcmp(flag, "relocations") == 0)
				{
					option = Dumper::Relocation;
				}
				else if(strcmp(flag, "import") == 0 || strcmp(flag, "imports") == 0)
				{
					option = Dumper::Import;
				}
				else if(strcmp(flag, "export") == 0 || strcmp(flag, "exports") == 0)
				{
					option = Dumper::Export;
				}
				else if(strcmp(flag, "control") == 0)
				{
					option = Dumper::Control;
				}
				else if(strcmp(flag, "string") == 0)
				{
					option = Dumper::String;
				}
				else if(strcmp(flag, "debug") == 0)
				{
					option = Dumper::Debug;
				}
				else if(strcmp(flag, "res") == 0 || strcmp(flag, "rsrc") == 0 || strcmp(flag, "resource") == 0 || strcmp(flag, "resources") == 0)
				{
					option = Dumper::Resource;
				}
				else if(strcmp(flag, "dynamic") == 0)
				{
					option = Dumper::Dynamic;
				}
				else if(strcmp(flag, "redundant") == 0)
				{
					option = Dumper::Redundant;
				}
				else if(strcmp(flag, "generated") == 0)
				{
					option = Dumper::Generated;
				}
				else if(strcmp(flag, "misc") == 0)
				{
					option = Dumper::Miscellaneous;
				}
				else
				{
					Linker::Error << "Error: unknown option flag `" << argv[i] << "'" << std::endl;
				}

				if(show)
				{
					show_options |= option;
				}
				else
				{
					hide_options |= option;
				}
			}
			/* TODO: select text encoding */
			else
			{
				std::ostringstream message;
				message << "Fatal error: Unknown option `" << argv[i] << "'";
				Linker::FatalError(message.str());
			}
		}
		else
		{
			if(input != "")
			{
				Linker::Error << "Error: Multiple input files provided, ignoring" << std::endl;
			}
			input = argv[i];
		}
	}

	if(input == "")
	{
		usage(argv[0]);
		exit(0);
	}

	std::ifstream in;
	in.open(input, std::ios_base::in | std::ios_base::binary);
	if(!in.is_open())
	{
		std::ostringstream message;
		message << "Fatal error: Unable to open file " << input;
		Linker::FatalError(message.str());
	}
	Reader rd (LittleEndian, &in);
	int status = 0;

	if(format == nullptr)
	{
		std::vector<format_description> file_formats;
		DetermineFormat(file_formats, rd);

		if(file_formats.size() == 0)
		{
			Linker::FatalError("Fatal error: Unable to determine file format");
		}

		format_priority highest_priority = PRIORITY_NONE;

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority > highest_priority)
				highest_priority = file_format.magic.priority;
		}

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority != highest_priority)
				continue;

			Linker::Debug << "Debug: Reading as " << file_format.magic.description << std::endl;
			format = CreateFormat(rd, file_format);
			if(!format)
			{
				Linker::Error << "Error: Unable to parse file, unimplemented format " << file_format.magic.description << std::endl;
				status = 1;
				continue;
			}
			rd.Seek(file_format.offset);

			try
			{
				format->ReadFile(rd);
				Dumper::Dumper dump(std::cout);
				dump.hide_options = hide_options;
				if(show_options != 0)
				{
					dump.show_options = show_options;
				}
				format->Dump(dump);
			}
			catch(Linker::Exception&)
			{
			}
		}

		for(auto& file_format : file_formats)
		{
			if(file_format.magic.priority == highest_priority)
				continue;

			Linker::Debug << "Debug: Other possible format: " << file_format.magic.description << std::endl;
		}
	}
	else
	{
		format->ReadFile(rd);
		Dumper::Dumper dump(std::cout);
		dump.hide_options = hide_options;
		if(show_options != 0)
		{
			dump.show_options = show_options;
		}
		format->Dump(dump);
	}

	return status;
}

