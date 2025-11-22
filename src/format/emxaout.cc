
#include "emxaout.h"

using namespace EMX;

void EMXAOutFormat::ReadFile(Linker::Reader& rd)
{
	char signature[2];
	rd.Seek(0);
	rd.ReadData(sizeof(signature), signature);
	if(signature[0] == 'M' && signature[0] == 'Z')
	{
		/* read it as a bound executable */
		LEFormat::ReadFile(rd);

		rd.endiantype = ::LittleEndian;
		rd.Seek(8);
		uint16_t header_paragraphs = rd.ReadUnsigned(2);

		rd.Seek((header_paragraphs << 4) + 18);
		aout_header_offset = rd.ReadUnsigned(4);
		rd.ReadData(dos_options);
		rd.Seek(aout_header_offset);
		AOutFormat::magic = AOutFormat::magic_type(rd.ReadUnsigned(2));
		AOutFormat::mid_value = rd.ReadUnsigned(1);
		AOutFormat::flags = rd.ReadUnsigned(1);
		AOutFormat::ReadHeader(rd);
	}
	else
	{
		/* read it as an unbound executable */
		AOutFormat::ReadFile(rd);
	}

	auto data_image = data->AsImage();
	text_base  = data_image->ReadUnsigned(4, 0, ::LittleEndian);
	text_end   = data_image->ReadUnsigned(4, 4, ::LittleEndian);
	data_base  = data_image->ReadUnsigned(4, 8, ::LittleEndian);
	data_end   = data_image->ReadUnsigned(4, 12, ::LittleEndian);
	bss_base   = data_image->ReadUnsigned(4, 16, ::LittleEndian);
	bss_end    = data_image->ReadUnsigned(4, 20, ::LittleEndian);
	heap_base  = data_image->ReadUnsigned(4, 24, ::LittleEndian);
	heap_end   = data_image->ReadUnsigned(4, 28, ::LittleEndian);
	heap_brk   = data_image->ReadUnsigned(4, 32, ::LittleEndian);
	heap_off   = data_image->ReadUnsigned(4, 36, ::LittleEndian);
	os2_dll    = data_image->ReadUnsigned(4, 40, ::LittleEndian);
	stack_base = data_image->ReadUnsigned(4, 44, ::LittleEndian);
	stack_end  = data_image->ReadUnsigned(4, 48, ::LittleEndian);
	flags      = data_image->ReadUnsigned(4, 52, ::LittleEndian);
	data_image->ReadData(64, 60, os2_options.data());
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

		if(stub.original_file_size - stub.original_header_size < 86)
		{
			Linker::FatalError("Fatal error: Stub is too short (needs at least 86 bytes in the image");
		}

		wr.endiantype = ::LittleEndian;

		// update EMX information in stub
		wr.Seek(stub.stub_header_size + 16);
		wr.WriteWord(1, 0xFF); // bind flag
		wr.Skip(1);
		wr.WriteWord(4, aout_header_offset);
		wr.WriteData(dos_options);

		wr.Seek(aout_header_offset);
		AOutFormat::WriteHeader(wr);
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
	LEFormat::Dump(dump);
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
	auto data_segment = GetDataSegment();
	if(data_segment->sections.size() == 0)
	{
		Linker::FatalError("Fatal error: data segment must not be empty");
	}
	auto data_section = data_segment->sections[0];
	if(data_section->Size() < 128)
	{
		Linker::FatalError("Fatal error: first data section must be at least 128 bytes long");
	}
	AOutFormat::CalculateValues();

	// calculate the patch area entries
	text_base = AOutFormat::GetTextAddress();
	text_end = text_base + GetCodeSegment()->TotalSize();
	data_base = AlignTo(text_base, AOutFormat::GetDataAddressAlign());
	data_end = data_base + GetDataSegment()->TotalSize();
	bss_base = data_end;
	bss_end = bss_base + GetBssSegment()->TotalSize();
	// TODO: calculate the remaining values

	data_section->WriteWord(4, 0, text_base, ::LittleEndian);
	data_section->WriteWord(4, 4, text_end, ::LittleEndian);
	data_section->WriteWord(4, 8, data_base, ::LittleEndian);
	data_section->WriteWord(4, 12, data_end, ::LittleEndian);
	data_section->WriteWord(4, 16, bss_base, ::LittleEndian);
	data_section->WriteWord(4, 20, bss_end, ::LittleEndian);
	data_section->WriteWord(4, 24, heap_base, ::LittleEndian);
	data_section->WriteWord(4, 28, heap_end, ::LittleEndian);
	data_section->WriteWord(4, 32, heap_brk, ::LittleEndian);
	data_section->WriteWord(4, 36, heap_off, ::LittleEndian);
	data_section->WriteWord(4, 40, os2_dll, ::LittleEndian);
	data_section->WriteWord(4, 44, stack_base, ::LittleEndian);
	data_section->WriteWord(4, 48, stack_end, ::LittleEndian);
	data_section->WriteWord(4, 52, flags, ::LittleEndian);
	data_section->WriteData(64, 60, os2_options.data());

	if(stub.filename != "")
	{
		// TODO: fill in LX fields

		LEFormat::CalculateValues();

		//aout_header_offset
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

