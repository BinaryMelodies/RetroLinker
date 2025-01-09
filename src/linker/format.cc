
#include <fstream>
#include "format.h"

using namespace Linker;

Format::~Format()
{
}

void Format::Dump(Dumper::Dumper& dump)
{
	Linker::Warning << "Warning: Dumping of format not implemented" << std::endl;
}

void Format::Clear()
{
}

bool OutputFormat::AddSupplementaryOutputFormat(std::string subformat)
{
	return false;
}

void OutputFormat::SetOptions(std::map<std::string, std::string>& options)
{
}

std::optional<std::string> OutputFormat::FetchOption(std::map<std::string, std::string>& options, std::string name)
{
	auto entry = options.find(name);
	if(entry != options.end())
	{
		return std::optional<std::string>(entry->second);
	}
	else
	{
		return std::optional<std::string>();
	}
}

std::string OutputFormat::FetchOption(std::map<std::string, std::string>& options, std::string name, std::string default_value)
{
	auto entry = options.find(name);
	if(entry != options.end())
	{
		return entry->second;
	}
	else
	{
		return default_value;
	}
}

std::optional<offset_t> OutputFormat::FetchIntegerOption(std::map<std::string, std::string>& options, std::string name)
{
	if(auto option = FetchOption(options, name))
	{
		try
		{
			return std::optional<offset_t>(std::stoll(option.value(), nullptr, 0));
		}
		catch(std::invalid_argument& a)
		{
			Linker::Error << "Error: Unable to parse " << option.value() << ", ignoring" << std::endl;
		}
	}
	return std::optional<offset_t>();
}

void OutputFormat::SetModel(std::string model)
{
	if(model != "")
	{
		Linker::Error << "Error: unsupported memory model, ignoring" << std::endl;
	}
}

void OutputFormat::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	/* predefined in linker manager */
}

void OutputFormat::ProcessModule(::Linker::Module& object)
{
}

void OutputFormat::CalculateValues()
{
}

void OutputFormat::GenerateFile(std::string filename, ::Linker::Module& module)
{
	ProcessModule(module);
	CalculateValues();

	std::ofstream out;
	out.open(filename, std::ios_base::out | std::ios_base::binary);
	Writer wr(::LittleEndian, &out);
	WriteFile(wr);
	out.close();
}

std::string OutputFormat::GetDefaultExtension(::Linker::Module& module, std::string filename)
{
	return filename;
}

std::string OutputFormat::GetDefaultExtension(::Linker::Module& module)
{
	return GetDefaultExtension(module, "a");
}

bool OutputFormat::FormatSupportsSegmentation() const
{
	return false;
}

bool OutputFormat::FormatIs16bit() const
{
	return false;
}

bool OutputFormat::FormatIsLinear() const
{
	return !FormatIs16bit();
}

#if 0
bool OutputFormat::FormatSupportsStackSection() const
{
	return false;
}
#endif

bool OutputFormat::FormatSupportsResources() const
{
	return false;
}

bool OutputFormat::FormatSupportsLibraries() const
{
	return false;
}

unsigned OutputFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	return 0;
}

void InputFormat::SetupOptions(char special_char, std::shared_ptr<OutputFormat> format)
{
}

bool InputFormat::FormatProvidesLibraries() const
{
	return false;
}

