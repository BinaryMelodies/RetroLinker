
#include "mzexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Microsoft;

// MZFormat

offset_t MZFormat::ImageSize() const
{
	return (offset_t(file_size_blocks) << 9) - (-offset_t(last_block_size) & 0x1FF);
}

MZFormat::Relocation MZFormat::Relocation::FromLinear(uint32_t address)
{
	return Relocation(address >> 4, address & 0xF);
}

uint32_t MZFormat::Relocation::GetOffset() const
{
	return (uint32_t(segment) << 4) + offset;
}

bool MZFormat::Relocation::operator ==(const Relocation& other) const
{
	return segment == other.segment && offset == other.offset;
}

bool MZFormat::Relocation::operator <(const Relocation& other) const
{
	return GetOffset() < other.GetOffset() || (GetOffset() == other.GetOffset() && segment < other.segment);
}

void MZFormat::PIF::SetDefaults()
{
	maximum_extra_paragraphs = 0xFFC0;
	minimum_extra_paragraphs = 0x1040;

	flags = 0;

	lowest_used_interrupt = 0x08;
	highest_used_interrupt = 0x2F;

	com_port_usage = 0;
	lpt_port_usage = 0;
	screen_usage = 0;
}

void MZFormat::PIF::ReadFile(Linker::Reader& rd)
{
	maximum_extra_paragraphs = rd.ReadUnsigned(2, LittleEndian);
	minimum_extra_paragraphs = rd.ReadUnsigned(2, LittleEndian);
	flags = rd.ReadUnsigned(1);
	rd.Skip(1);
	lowest_used_interrupt = rd.ReadUnsigned(1);
	highest_used_interrupt = rd.ReadUnsigned(1);
	com_port_usage = rd.ReadUnsigned(1);
	lpt_port_usage = rd.ReadUnsigned(1);
	screen_usage = rd.ReadUnsigned(1);
}

void MZFormat::PIF::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, MAGIC_BEGIN, LittleEndian);

	wr.WriteWord(2, maximum_extra_paragraphs, LittleEndian);
	wr.WriteWord(2, minimum_extra_paragraphs, LittleEndian);
	wr.WriteWord(1, flags);
	wr.WriteWord(1, 0);
	wr.WriteWord(1, lowest_used_interrupt);
	wr.WriteWord(1, highest_used_interrupt);
	wr.WriteWord(1, com_port_usage);
	wr.WriteWord(1, lpt_port_usage);
	wr.WriteWord(1, screen_usage);

	wr.WriteWord(4, MAGIC_END, LittleEndian);
}

void MZFormat::PIF::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	Dumper::Region pif_region("Program information", file_offset, SIZE, 5);

	pif_region.AddField("Minimum", Dumper::HexDisplay::Make(6), offset_t(uint32_t(minimum_extra_paragraphs) << 4));
	pif_region.AddField("Maximum", Dumper::HexDisplay::Make(6), offset_t(uint32_t(maximum_extra_paragraphs) << 4));

	pif_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("Requires 8087 coprocessor"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("May run in banked memory"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("Runs only in the foreground"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("Waits in idle loop"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("Requires aligned memory"), true),
		offset_t(flags));
	pif_region.AddField("Lowest used interrupt", Dumper::HexDisplay::Make(2), offset_t(lowest_used_interrupt));
	pif_region.AddField("Highest used interrupt", Dumper::HexDisplay::Make(2), offset_t(highest_used_interrupt));
	pif_region.AddField("COM port usage",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("Direct access to COM1"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("Direct access to COM2"), true),
		offset_t(com_port_usage));
	pif_region.AddField("LPT port usage",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("Direct access to LPT1"), true)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("Direct access to LPT2"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("Direct access to LPT3"), true),
		offset_t(lpt_port_usage));
	pif_region.AddField("Screen usage",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("Uses 25 lines", "Uses 24 lines"), false)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("Uses ANSI escape sequences"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("Uses ROS calls"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("Direct video access"), true),
		offset_t(screen_usage));

	pif_region.Display(dump);
}

MZFormat::magic_type MZFormat::GetSignature() const
{
	if(signature[0] == 'M' && signature[1] == 'Z')
		return MAGIC_MZ;
	else if(signature[0] == 'Z' && signature[1] == 'M')
		return MAGIC_ZM;
	else if(signature[0] == 'D' && signature[1] == 'L')
		return MAGIC_DL;
	else
		return magic_type(0); // invalid
}

void MZFormat::SetSignature(magic_type magic)
{
	switch(magic)
	{
	case MAGIC_MZ:
		signature[0] = 'M';
		signature[1] = 'Z';
		break;
	case MAGIC_ZM:
		signature[0] = 'Z';
		signature[1] = 'M';
		break;
	case MAGIC_DL:
		signature[0] = 'D';
		signature[1] = 'L';
		break;
	}
}

void MZFormat::Clear()
{
	/* format fields */
	relocations.clear();
	pif = nullptr;
	image = nullptr;
	/* writer fields */
	ClearSegmentManager();
}

void MZFormat::SetFileSize(uint32_t size)
{
	last_block_size = size & 0x1FF;
	file_size_blocks = (size + 0x1FF) >> 9;
}

uint32_t MZFormat::GetHeaderSize() const
{
	return uint32_t(header_size_paras) << 4;
}

uint32_t MZFormat::GetPifOffset() const
{
	return relocation_offset == 0 ? 0x1E : relocation_offset + relocation_count * 4;
}

void MZFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	offset_t file_end = rd.GetImageEnd();
	file_offset = rd.Tell();

	rd.endiantype = ::LittleEndian;

	rd.ReadData(2, signature);
	if(GetSignature() == magic_type(0))
	{
		Linker::FatalError("Invalid magic number");
	}
	last_block_size = rd.ReadUnsigned(2);
	file_size_blocks = rd.ReadUnsigned(2);
	relocation_count = rd.ReadUnsigned(2);
	header_size_paras = rd.ReadUnsigned(2);
	min_extra_paras = rd.ReadUnsigned(2);
	max_extra_paras = rd.ReadUnsigned(2);
	ss = rd.ReadUnsigned(2);
	sp = rd.ReadUnsigned(2);
	checksum = rd.ReadUnsigned(2);
	ip = rd.ReadUnsigned(2);
	cs = rd.ReadUnsigned(2);
	relocation_offset = rd.ReadUnsigned(2);
	if(GetSignature() != MAGIC_DL)
	{
		overlay_number = rd.ReadUnsigned(2);
		data_segment = 0;
	}
	else
	{
		overlay_number = 0;
		data_segment = rd.ReadUnsigned(2);
	}
	relocations.clear();
	rd.Seek(relocation_offset);
	if(relocation_count != 0)
	{
		for(size_t i = 0; i < relocation_count; i++)
		{
			uint16_t offset = rd.ReadUnsigned(2);
			uint16_t segment = rd.ReadUnsigned(2);
			relocations.push_back(Relocation(segment, offset));
		}
	}
	if(GetPifOffset() + 19 <= file_end)
	{
		rd.Seek(GetPifOffset());
		if(rd.ReadUnsigned(4) == PIF::MAGIC_BEGIN)
		{
			pif = std::make_unique<PIF>();
			pif->ReadFile(rd);
			if(rd.ReadUnsigned(4) != PIF::MAGIC_END)
			{
				/* failed */
				pif = nullptr;
			}
		}
	}
	rd.Seek(uint32_t(header_size_paras) << 4);
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(".text");
	image = buffer;
	buffer->ReadFile(rd, ImageSize() - GetHeaderSize());
}

offset_t MZFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(2, signature);
	wr.WriteWord(2, last_block_size);
	wr.WriteWord(2, file_size_blocks);
	wr.WriteWord(2, relocation_count);
	wr.WriteWord(2, header_size_paras);
	wr.WriteWord(2, min_extra_paras);
	wr.WriteWord(2, max_extra_paras);
	wr.WriteWord(2, ss);
	wr.WriteWord(2, sp);
	wr.WriteWord(2, 0); /* TODO: checksum */
	wr.WriteWord(2, ip);
	wr.WriteWord(2, cs);
	wr.WriteWord(2, relocation_offset);
	wr.WriteWord(2, GetSignature() != MAGIC_DL ? overlay_number : data_segment);
	wr.Seek(relocation_offset);
	for(auto& rel : relocations)
	{
		wr.WriteWord(2, rel.offset);
		wr.WriteWord(2, rel.segment);
	}
	if(pif)
	{
		wr.Seek(GetPifOffset());
		pif->WriteFile(wr);
	}
	wr.Seek(uint32_t(header_size_paras) << 4);
	if(image)
		image->WriteFile(wr);

	wr.FillTo(ImageSize());

	return ImageSize();
}

void MZFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("MZ format");
	offset_t magic_field = GetSignature();
	static const std::map<offset_t, std::string> magic_field_descriptions =
	{
		{ MAGIC_MZ, "executable" },
		{ MAGIC_ZM, "old-style executable" },
		{ MAGIC_DL, "HP 100LX/200LX System Manager compliant module" }
	};
	Dumper::Region file_region("File", file_offset, ImageSize(), 6);
	file_region.AddField("Signature", Dumper::StringDisplay::Make(2, "'"), std::string(signature, 2));
	file_region.AddField("Type", Dumper::ChoiceDisplay::Make(magic_field_descriptions), offset_t(magic_field));
	file_region.AddOptionalField("Overlay", Dumper::DecDisplay::Make(), magic_field != MAGIC_DL ? overlay_number : offset_t(0));
	file_region.AddOptionalField("Data segment", Dumper::HexDisplay::Make(), magic_field != MAGIC_DL ? 0 : offset_t(uint32_t(data_segment) << 4));
	file_region.Display(dump);

	Dumper::Region header_region("Header", file_offset, GetHeaderSize(), 6);
	header_region.AddField("SS:SP", Dumper::SegmentedDisplay::Make(), offset_t(ss), offset_t(sp));
	header_region.AddField("CS:IP", Dumper::SegmentedDisplay::Make(), offset_t(cs), offset_t(ip));
	header_region.AddField("Minimum", Dumper::HexDisplay::Make(), offset_t(ImageSize() - GetHeaderSize() + (uint32_t(min_extra_paras) << 4)));
	header_region.AddField("Maximum", Dumper::HexDisplay::Make(), offset_t(ImageSize() - GetHeaderSize() + (uint32_t(max_extra_paras) << 4)));
	header_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(4), offset_t(checksum));
	header_region.Display(dump);

	Dumper::Region relocations_region("Relocations", file_offset + relocation_offset, relocation_count * 4, 8);
	relocations_region.AddField("Count", Dumper::DecDisplay::Make(), offset_t(relocation_count));
	relocations_region.Display(dump);

	if(pif)
	{
		pif->Dump(dump, file_offset + relocation_offset + relocation_count * 4);
	}

	Dumper::Block image_block("Image", file_offset + GetHeaderSize(), image->AsImage(), 0, 6);

	size_t i = 0;
	for(auto relocation : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, file_offset + relocation_offset + i * 4, 6);
		relocation_entry.AddField("Source", Dumper::SegmentedDisplay::Make(), offset_t(relocation.segment), offset_t(relocation.offset));
		relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(4), image->AsImage()->ReadUnsigned(2, (offset_t(relocation.segment) << 4) | relocation.offset));
		relocation_entry.Display(dump);
		image_block.AddSignal(relocation.GetOffset(), 2);
		i++;
	}

	image_block.Display(dump);
}

void MZFormat::CalculateValues()
{
	if(relocations.size() >= 0x10000)
	{
		Linker::FatalError("Fatal error: Too many relocations");
	}
	relocation_count = relocations.size();
	uint32_t header_size = uint32_t(header_size_paras) << 4;
	uint32_t minimum_header_size;
	if(pif != nullptr && relocation_offset == 0 && relocation_count == 0)
	{
		minimum_header_size = 0x1E + PIF::SIZE; /* PIFED places the program information here */
	}
	else
	{
		if(relocation_offset < 0x1C && (relocation_count != 0 || pif != nullptr))
		{
			/* Watcom default */
			relocation_offset = 0x20;
		}

		minimum_header_size = relocation_offset + uint32_t(relocation_count) * 4;
		if(pif != nullptr)
		{
			minimum_header_size += PIF::SIZE;
		}
	}
	if(header_size < minimum_header_size)
	{
		header_size = minimum_header_size;
	}
	if(header_size < 0x20)
	{
		header_size = 0x20;
	}
	header_size = ::AlignTo(header_size, option_header_align);
	if(header_size >= 0x10000)
	{
		Linker::FatalError("Fatal error: Header too large");
	}
	header_size_paras = header_size >> 4;

	uint32_t file_size = GetHeaderSize() + GetDataSize();
	file_size = ::AlignTo(file_size, option_file_align); /* TODO: test out */

	SetFileSize(file_size);
	max_extra_paras = min_extra_paras + extra_paras; /* TODO */
}

/* * * Writer members * * */

bool MZFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool MZFormat::FormatIs16bit() const
{
	return true;
}

bool MZFormat::FormatIsProtectedMode() const
{
	return false;
}

unsigned MZFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	unsigned flags;
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		flags = Linker::Section::Stack;
	}
	else
	{
		flags = 0;
	}
	if(section_name == ".opt" || ends_with(section_name, ".opt"))
	{
		flags |= Linker::Section::Optional;
	}
	return flags;
}

std::vector<Linker::OptionDescription<void>> MZFormat::MemoryModelNames =
{
	Linker::OptionDescription<void>("default/tiny", "Tiny model, all symbols have the same preferred segment"),
	Linker::OptionDescription<void>("small", "Small model, symbols in .code have a separate preferred segment"),
	Linker::OptionDescription<void>("compact", "Compact model, symbols in .data, .bss and .comm have a common preferred segment, all other sections are treated as separate segments"),
//	Linker::OptionDescription<void>("large", "Large model, ???"), // TODO
};

std::vector<Linker::OptionDescription<void>> MZFormat::GetMemoryModelNames()
{
	return MemoryModelNames;
}

void MZFormat::SetModel(std::string model)
{
	if(model == "")
	{
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "tiny")
	{
		memory_model = MODEL_TINY;
	}
	else if(model == "small")
	{
		memory_model = MODEL_SMALL;
	}
	else if(model == "compact")
	{
		memory_model = MODEL_COMPACT;
	}
	else if(model == "large")
	{
		memory_model = MODEL_LARGE;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_SMALL;
	}
}

std::shared_ptr<Linker::OptionCollector> MZFormat::GetOptions()
{
	return std::make_shared<MZOptionCollector>();
}

void MZFormat::SetOptions(std::map<std::string, std::string>& options)
{
	MZOptionCollector collector;
	collector.ConsiderOptions(options);
	if(auto header_align = collector.header_align())
	{
		option_header_align = header_align.value();
	}
	if(auto file_align = collector.file_align())
	{
		option_file_align = file_align.value();
	}
	stack_size = collector.stack();
}

void MZFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		if(image != nullptr)
		{
			Linker::Error << "Error: duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		image = segment;
		/* TODO: parametrize extra_paras */
		extra_paras = (segment->optional_extra + 0xF) >> 4;
		zero_fill = segment->zero_fill;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, ignoring" << std::endl;
	}
}

void MZFormat::CreateDefaultSegments()
{
	if(image == nullptr)
	{
		image = std::make_shared<Linker::Segment>(".code");
	}
}

std::unique_ptr<Script::List> MZFormat::GetScript(Linker::Module& module)
{
	static const char * TinyScript = R"(
".code"
{
	all not zero;
	all not ".stack";
	align 16;
	all;
	align 16;
};
)";

	static const char * SmallScript = R"(
".code"
{
	all exec;
	align 16;
	base here;
	all not zero;
	all not ".stack";
	align 16;
	all;
	align 16;
};
)";

	static const char * CompactScript = R"(
".code"
{
	all exec;
	all not zero and not ".data" and not ".rodata"
		align 16 base here;
	align 16;
	base here;
	all ".data" or ".rodata";
	align 16;
	all ".bss" or ".comm";
	all not ".stack" align 16 base here;
	all base here;
};
)";

	if(linker_script != "")
	{
		if(memory_model != MODEL_DEFAULT)
		{
			Linker::Warning << "Warning: linker script provided, overriding memory model" << std::endl;
		}
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(memory_model)
		{
		case MODEL_TINY:
			/* single segment/address space for code and data */
			return Script::parse_string(TinyScript);
		case MODEL_DEFAULT:
		case MODEL_SMALL:
			/* separate segment/address space for code and data, one for each */
			return Script::parse_string(SmallScript);
		case MODEL_COMPACT:
			/* separate address space for a single code segment and multiple data segments */
			return Script::parse_string(CompactScript);
		case MODEL_LARGE:
			/* multiple separate segments for code and data */
			/* TODO */
			//return Script::parse_string(LargeScript);
			;
		default:
			Linker::FatalError("Internal error: invalid memory model");
		}
	}
}

void MZFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();

#if 0
	for(Linker::Section * section : module.Sections())
	{
		Linker::Debug << "Debug: Section " << section->name;
		if(section->segment)
			Linker::Debug << " in " << section->segment->name;
		Linker::Debug << " bias " << section->bias << std::endl;
	}
#endif
}

#if 0
void MZFormat::LinkLarge(Linker::Module& module)
{
	static const char _code[] = ".code";
	Linker::AppendSections<Linker::IsCalled<_code>::predicate>(module, image);
	Linker::AppendSections<Linker::IsExecutable>(module, image, false);
	static const char _data[] = ".data";
	static const char _rodata[] = ".rodata";
	image.next_bias = 0;
	Linker::AppendSections<Linker::IsCalled<_data, _rodata>::predicate>(module, image);
	Linker::AppendSections<Linker::IsDataFilled>(module, image, false);
	static const char _bss[] = ".bss";
	static const char _comm[] = ".comm";
	image.next_bias = 0;
	Linker::AppendSections<Linker::IsCalled<_bss, _comm>::predicate>(module, image); /* TODO: .bss is in a separate segment */
	Linker::AppendSections<Linker::IsNonStack>(module, image, false); /* copy each section separately */
	Linker::AppendSections<>(module, image);
	/* TODO */
	assert(false);
}
#endif

void MZFormat::ProcessModule(Linker::Module& module)
{
	module.AllocateStack(stack_size);

	Link(module);

	std::set<uint32_t> relocation_offsets;
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::Direct)
		{
			rel.WriteWord(resolution.value);
		}
		else if(rel.kind == Linker::Relocation::ParagraphAddress)
		{
			if(resolution.target != nullptr)
			{
				relocation_offsets.insert(rel.source.GetPosition().address);
			}
			rel.WriteWord(resolution.value);
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}

	for(auto rel : relocation_offsets)
	{
		relocations.push_back(Relocation::FromLinear(rel));
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Debug << "Debug: " << stack_top << std::endl;
		ss = stack_top.GetPosition(true).address >> 4;
		sp = stack_top.GetPosition().address - (uint32_t(ss) << 4);
	}
	else
	{
		// use last section of image as stack top
		std::shared_ptr<Linker::Segment> image_segment = std::dynamic_pointer_cast<Linker::Segment>(image);
		std::shared_ptr<Linker::Section> stack = image_segment->sections.back();
		ss = stack->Base().address >> 4;
		sp = image_segment->TotalSize() - (uint32_t(ss) << 4);
		Linker::Debug << "Debug: End of memory: " << sp << std::endl;
		Linker::Debug << "Debug: Total size: " << (image->ImageSize() + zero_fill) << std::endl;
		Linker::Debug << "Debug: Stack base: " << ss << std::endl;
#if 0
		if(!(stack->GetFlags() & Linker::Section::Stack)) /* TODO: allocate stack instead */
		{
			Linker::Warning << "Warning: no stack top specified, using end of .bss segment" << std::endl;
		}
#endif
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		cs = entry.GetPosition(true).address >> 4;
		ip = entry.GetPosition().address - (uint32_t(cs) << 4);
	}
	else
	{
		ip = 0;
		cs = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}

	if(GetSignature() == MAGIC_DL)
	{
		/* TODO: untested */
		offset_t code_size = 0;
		for(auto& section : std::dynamic_pointer_cast<Linker::Segment>(image)->sections)
		{
			if(!section->IsExecutable())
				break;
			assert(section->GetStartAddress() == code_size);
			code_size += section->Size();
		}
		data_segment = (code_size + 0xF) >> 4;
	}

	min_extra_paras = (zero_fill + 0xF) >> 4;
}

uint32_t MZFormat::GetDataSize() const
{
	return image ? image->ImageSize() : 0;
}

void MZFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::I86)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 8086 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string MZFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + (GetSignature() != MAGIC_DL ? ".exe" : ".exm");
}

// MZSimpleStubWriter

bool MZSimpleStubWriter::OpenAndCheckValidFile()
{
	if(filename != "" && valid)
	{
		if(!stream.is_open())
		{
			stream.open(filename, std::ios_base::in | std::ios_base::binary);
		}
		if(!stream.good())
		{
			Linker::Error << "Error: unable to open stub file " << filename << ", generating dummy stub" << std::endl;
			valid = false;
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

offset_t MZSimpleStubWriter::GetStubImageSize()
{
	if(OpenAndCheckValidFile())
	{
		Linker::Reader reader(::LittleEndian);
		reader.in = &stream;
		reader.Seek(2);
		uint32_t file_size = reader.ReadUnsigned(2);
		return size = (reader.ReadUnsigned(2) << 9) - ((-file_size) & 0x1FF);
	}
	else
	{
		/* minimal stub */
		return size = 0x20;
	}
}

void MZSimpleStubWriter::WriteStubImage(std::ostream& out)
{
	Linker::Writer wr(::LittleEndian, &out);
	WriteStubImage(wr);
}

void MZSimpleStubWriter::WriteStubImage(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	if(OpenAndCheckValidFile())
	{
		if(size == offset_t(-1))
		{
			GetStubImageSize();
		}
		stream.seekg(0);
		wr.WriteData(size, stream);
	}
	else
	{
		/* minimal stub */
		wr.WriteData(2, "MZ");
		wr.WriteWord(2, 0x20);
		wr.WriteWord(2, 1);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 2);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0x20);
		wr.WriteWord(2, 0);
		wr.Seek(0x20);
	}

	if(stream.is_open())
	{
		stream.close();
	}
}

// MZStubWriter

bool MZStubWriter::OpenAndCheckValidFile()
{
	if(filename != "" && valid)
	{
		if(!stream.is_open())
		{
			stream.open(filename, std::ios_base::in | std::ios_base::binary);
		}
		if(!stream.good())
		{
			Linker::Error << "Error: unable to open stub file " << filename << ", generating dummy stub" << std::endl;
			valid = false;
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

offset_t MZStubWriter::GetStubImageSize()
{
	if(OpenAndCheckValidFile())
	{
		Linker::Reader reader(::LittleEndian);
		reader.in = &stream;
		reader.Seek(2);
		original_file_size = reader.ReadUnsigned(2);
		original_file_size = (reader.ReadUnsigned(2) << 9) - ((-original_file_size) & 0x1FF);
		stub_reloc_count = reader.ReadUnsigned(2);
		original_header_size = uint32_t(reader.ReadUnsigned(2)) << 4;
		reader.Seek(0x18);
		stub_reloc_offset = original_reloc_offset = reader.ReadUnsigned(2);

		stub_header_size = 0x40;
		if(stub_reloc_offset < 0x40)
		{
			stub_reloc_offset = 0x40;
			stub_header_size = ::AlignTo(0x40 + 4 * stub_reloc_count, 0x10);
		}
		else if(stub_header_size < original_header_size)
		{
			stub_header_size = original_header_size;
		}
		return stub_file_size = stub_header_size + (original_file_size - original_header_size);
	}
	else
	{
		/* minimal stub */
		stub_file_size = 0x40;
		stub_reloc_count = 0;
		stub_header_size = 0x40;
		stub_reloc_offset = 0x40;
		return 0x40;
	}
}

void MZStubWriter::WriteStubImage(std::ostream& out)
{
	Linker::Writer wr(::LittleEndian, &out);
	WriteStubImage(wr);
}

void MZStubWriter::WriteStubImage(Linker::Writer& wr)
{
	if(OpenAndCheckValidFile())
	{
		if(original_file_size == uint32_t(-1))
		{
			GetStubImageSize();
		}

		if(stub_reloc_offset != original_reloc_offset || stub_header_size != original_header_size)
		{
			wr.WriteData(2, "MZ");
			wr.WriteWord(2, stub_file_size & 0x1FF);
			wr.WriteWord(2, (stub_file_size + 0x1FF) >> 9);
			wr.WriteWord(2, stub_reloc_count);
			wr.WriteWord(2, stub_header_size >> 4);
			stream.seekg(0x0A);
			wr.WriteData(0x0E, stream);
			wr.WriteWord(2, stub_reloc_offset);
			wr.Seek(0x3C);
			wr.WriteWord(4, stub_file_size);
			stream.seekg(original_reloc_offset);
			wr.WriteData(4 * stub_reloc_count, stream);
			wr.Seek(stub_header_size);
			stream.seekg(original_header_size);
			wr.WriteData(original_file_size - original_header_size, stream);
		}
		else
		{
			stream.seekg(0);
			wr.WriteData(original_file_size, stream);
		}
	}
	else
	{
		/* minimal stub */
		wr.WriteData(2, "MZ");
		wr.WriteWord(2, 0x40);
		wr.WriteWord(2, 1);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 4);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0);
		wr.WriteWord(2, 0x40);
		wr.WriteWord(2, 0);
		wr.Seek(0x3C);
		wr.WriteWord(4, 0x40);
	}

	if(stream.is_open())
	{
		stream.close();
	}
}

