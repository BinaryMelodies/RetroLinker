
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
		format->Dump(dump);
	}

	return status;
}

