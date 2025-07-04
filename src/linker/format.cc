
#include <fstream>
#include "format.h"
#include "module_collector.h"
#include "options.h"
#include "writer.h"

using namespace Linker;

void Format::Dump(Dumper::Dumper& dump) const
{
	Linker::Warning << "Warning: Dumping of format not implemented" << std::endl;
}

void Format::Clear()
{
}

offset_t Format::ImageSize() const
{
	Linker::FatalError("Internal error: format does not provide image size");
}

offset_t Format::WriteFile(Writer& wr, offset_t count, offset_t offset) const
{
	if(offset != 0)
	{
		Linker::FatalError("Internal error: format cannot generate partial data");
	}
	// count is ignored
	return WriteFile(wr);
}

bool OutputFormat::AddSupplementaryOutputFormat(std::string subformat)
{
	return false;
}

std::vector<OptionDescription<void>> OutputFormat::GetMemoryModelNames()
{
	return std::vector<OptionDescription<void>>();
}

std::vector<OptionDescription<void> *> OutputFormat::GetLinkerScriptParameterNames()
{
	return std::vector<OptionDescription<void> *>();
}

static std::vector<OptionDescription<void>> SpecialSymbolNames =
{
	OptionDescription<void>(".entry", "The location where execution is expected to start (PC, CS:IP, CS:EIP, RIP)"),
	OptionDescription<void>(".stack_top", "The location where the stack should start being pushed to (SP, SS:SP, SS:ESP, RSP, S)"),
};

std::vector<OptionDescription<void>> OutputFormat::GetSpecialSymbolNames()
{
	//return std::vector<OptionDescription<void>>();
	return SpecialSymbolNames;
}

std::shared_ptr<OptionCollector> OutputFormat::GetOptions()
{
	return std::make_shared<OptionCollector>();
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

std::string OutputFormat::GetDefaultExtension(::Linker::Module& module, std::string filename) const
{
	return filename;
}

std::string OutputFormat::GetDefaultExtension(::Linker::Module& module) const
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

bool OutputFormat::FormatIsProtectedMode() const
{
	// most 16-bit x86 formats are for real mode, so this default makes sense
	// 32-bit x86 must return true
	return !FormatIs16bit();
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

void InputFormat::SetupOptions(std::shared_ptr<OutputFormat> format)
{
}

void InputFormat::ProduceModule(ModuleCollector& linker, Reader& rd, std::string file_name)
{
	ReadFile(rd);
	GenerateModule(linker, file_name);
}

void InputFormat::ProduceModule(Module& module, Reader& rd)
{
	ReadFile(rd);
	GenerateModule(module);
}

void InputFormat::GenerateModule(ModuleCollector& linker, std::string file_name, bool is_library) const
{
	std::shared_ptr<Module> module = linker.CreateModule(shared_from_this(), file_name);
	GenerateModule(*module);
	if(is_library)
		linker.AddLibraryModule(module);
	else
		linker.AddModule(module);
}

void InputFormat::GenerateModule(Module& module) const
{
	Linker::FatalError("Internal error: processing input format not implemented");
}

bool InputFormat::FormatProvidesSegmentation() const
{
	return false;
}

bool InputFormat::FormatRequiresDataStreamFix() const
{
	return false;
}

bool InputFormat::FormatProvidesResources() const
{
	return false;
}

bool InputFormat::FormatProvidesLibraries() const
{
	return false;
}

