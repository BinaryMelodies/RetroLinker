
#include "emxaout.h"

using namespace EMX;

void EMXAOutFormat::ReadFile(Linker::Reader& rd)
{
	char signature[2];
	rd.Seek(0);
	rd.ReadData(sizeof(signature), signature);
	if(signature[0] == 'M' && signature[0] == 'Z')
		/* TODO: read it as a bound executable */;
	else
		/* TODO: read it as an unbound executable */;
}

offset_t EMXAOutFormat::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t EMXAOutFormat::WriteFile(Linker::Writer& wr) const
{
	if(stub.filename == "")
	{
		AOutFormat::WriteFile(wr);
	}
	else
	{
		LEFormat::WriteFile(wr);
		// TODO: update EMX information in stub
		// TODO: write a.out header inside LX image
	}
	return ImageSize();
}

void EMXAOutFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("EMX a.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

/* * * Reader members * * */

void EMXAOutFormat::GenerateModule(Linker::Module& module) const
{
	AOutFormat::GenerateModule(module);
}

/* * * Writer members * * */

std::vector<Linker::OptionDescription<void> *> EMXAOutFormat::ParameterNames =
{
};

std::vector<Linker::OptionDescription<void> *> EMXAOutFormat::GetLinkerScriptParameterNames()
{
	return ParameterNames;
}

std::shared_ptr<Linker::OptionCollector> EMXAOutFormat::GetOptions()
{
	// TODO
	return OutputFormat::GetOptions();
}

void EMXAOutFormat::SetOptions(std::map<std::string, std::string>& options)
{
	// TODO
}

void EMXAOutFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	AOutFormat::OnNewSegment(segment);
}

void EMXAOutFormat::ProcessModule(Linker::Module& module)
{
	AOutFormat::ProcessModule(module);
}

void EMXAOutFormat::CalculateValues()
{
	AOutFormat::CalculateValues();

	if(stub.filename != "")
	{
		// TODO: fill in LX fields

		LEFormat::CalculateValues();
	}
}

void EMXAOutFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::I386)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 80386 binaries");
	}

	// a.out fields
	AOutFormat::cpu = AOutFormat::I386;
	AOutFormat::mid_value = MID_PC386;
	AOutFormat::flags = 0;
	AOutFormat::word_size = AOutFormat::GetWordSize();

	// LE fields
	LEFormat::program_name = filename;
	size_t ix = filename.rfind('.');
	if(ix != std::string::npos)
		LEFormat::module_name = filename.substr(0, ix);
	else
		LEFormat::module_name = filename;

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string EMXAOutFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	if(stub.filename == "")
		return filename;
	else
		return filename + ".exe";
}

std::string EMXAOutFormat::GetDefaultExtension(Linker::Module& module) const
{
	if(stub.filename == "")
		return "a.out";
	else
		return "a.exe";
}

