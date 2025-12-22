
#include "pharlap.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace PharLap;

// MPFormat

void MPFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	std::array<char, 2> signature;
	file_offset = Microsoft::FindActualSignature(rd, signature, "MP", "MQ", true);
	has_relocations = signature[1] == 'Q';
	image_size = rd.ReadUnsigned(2);
	image_size = (uint32_t(rd.ReadUnsigned(2)) << 9) - (-image_size & 0x1FF);
	relocation_count = rd.ReadUnsigned(2);
	header_size = uint32_t(rd.ReadUnsigned(2)) << 4;
	min_extra_pages = rd.ReadUnsigned(2);
	max_extra_pages = rd.ReadUnsigned(2);
	esp = rd.ReadUnsigned(4);
	checksum = rd.ReadUnsigned(2);
	eip = rd.ReadUnsigned(4);
	relocation_offset = rd.ReadUnsigned(2);

	rd.Seek(file_offset + relocation_offset);
	if(has_relocations && relocation_count != 0)
	{
		for(uint16_t relocation_index = 0; relocation_index < relocation_count; relocation_index++)
		{
			relocations.push_back(Relocation(rd.ReadUnsigned(4)));
		}
	}

	rd.Seek(file_offset + header_size);
	image = Linker::Buffer::ReadFromFile(rd, image_size - header_size);
}

unsigned MPFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		return Linker::Section::Stack;
	}
	else
	{
		return 0;
	}
}

bool MPFormat::Relocation::operator ==(const Relocation& other) const
{
	return offset == other.offset && rel32 == other.rel32;
}

bool MPFormat::Relocation::operator <(const Relocation& other) const
{
	return offset < other.offset || (offset == other.offset && rel32 < other.rel32);
}

std::shared_ptr<Linker::OptionCollector> MPFormat::GetOptions()
{
	return std::make_shared<MPOptionCollector>();
}

void MPFormat::SetOptions(std::map<std::string, std::string>& options)
{
	MPOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
	stack_size = collector.stack();
}

void MPFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		image = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

std::unique_ptr<Script::List> MPFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	all not write align 4;
	align 4;

	all not zero align 4;
	align 4;

	all align 4;
	align 4
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void MPFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void MPFormat::ProcessModule(Linker::Module& module)
{
	module.AllocateStack(stack_size);

	Link(module);

	std::shared_ptr<Linker::Segment> segment = std::static_pointer_cast<Linker::Segment>(image);

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		esp = stack_top.GetPosition().address;
	}
	else
	{
		Linker::Warning << "Warning: no stack found" << std::endl;
		if(module.FindSection(".stack") == nullptr)
		{
			segment->zero_fill += 0x1000;
		}
		esp = segment->TotalSize();
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		eip = entry.GetPosition().address;
	}
	else
	{
		Linker::Warning << "Warning: no entry found, using 0" << std::endl;
		eip = 0;
	}

	std::set<Relocation> relocations_set;
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::Direct)
		{
			rel.WriteWord(resolution.value);
			if(resolution.target != nullptr && resolution.reference == nullptr)
			{
				Linker::Position source = rel.source.GetPosition();
				if(rel.size == 4)
					relocations_set.insert(Relocation(source.address, 1));
				else
					relocations_set.insert(Relocation(source.address, 0));
			}
		}
		else if(rel.kind == Linker::Relocation::SelectorIndex)
		{
			Linker::Error << "Error: segment relocations impossible in flat memory model, ignoring" << std::endl;
			continue;
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}

	relocations.clear();
	for(auto relocation : relocations_set)
	{
		relocations.push_back(relocation);
	}
}

void MPFormat::CalculateValues()
{
	std::shared_ptr<Linker::Segment> segment = std::static_pointer_cast<Linker::Segment>(image);
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;
	relocation_offset = 0x1E;
	header_size = ::AlignTo(relocation_offset + (has_relocations ? 4 * relocations.size() : 0), 0x10);
	image_size = header_size + segment->data_size;
	min_extra_pages = (segment->zero_fill + 0x3FFF) >> 12;
	max_extra_pages = min_extra_pages + ((segment->optional_extra + 0x3FFF) >> 12); /* TODO */
	relocation_count = has_relocations ? relocations.size() : 0;
}

offset_t MPFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	if(stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}
	wr.Seek(file_offset);
	wr.WriteData(2, has_relocations ? "MQ" : "MP");
	wr.WriteWord(2, image_size & 0x1FF);
	wr.WriteWord(2, (image_size + 0x1FF) >> 9);
	wr.WriteWord(2, relocation_count);
	wr.WriteWord(2, (header_size + 0xF) >> 4);
	wr.WriteWord(2, min_extra_pages);
	wr.WriteWord(2, max_extra_pages);
	wr.WriteWord(4, esp);
	wr.WriteWord(2, checksum); /* TODO */
	wr.WriteWord(4, eip);
	wr.WriteWord(2, has_relocations ? relocation_offset : 0);
	wr.WriteWord(2, 1);

	if(has_relocations && relocations.size() != 0)
	{
		wr.Seek(file_offset + relocation_offset);
		for(Relocation rel : relocations)
		{
			wr.WriteWord(4, rel.value);
		}
	}

	wr.Seek(file_offset + ::AlignTo(header_size, 0x10));
	image->WriteFile(wr);

	return offset_t(-1);
}

void MPFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("MP/MQ format");
	Dumper::Region file_region("File", file_offset, image_size, 8);
	file_region.Display(dump);

	Dumper::Region header_region("Header", file_offset, header_size, 8);
	header_region.AddField("Signature", Dumper::StringDisplay::Make("'"), std::string(has_relocations ? "MQ" : "MP"));
	header_region.AddField("Relocation count", Dumper::DecDisplay::Make(), offset_t(relocation_count));
	header_region.AddField("Relocation offset", Dumper::DecDisplay::Make(), offset_t(relocation_count));
	header_region.AddField("Minimum extra memory", Dumper::HexDisplay::Make(8), offset_t(min_extra_pages) << 12);
	header_region.AddField("Maximum extra memory", Dumper::HexDisplay::Make(8), offset_t(max_extra_pages) << 12);
	header_region.AddField("Entry (EIP)", Dumper::HexDisplay::Make(8), offset_t(eip));
	header_region.AddField("Initial stack (ESP)", Dumper::HexDisplay::Make(8), offset_t(esp));
	header_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(4), offset_t(checksum));
	header_region.Display(dump);

	if(relocation_offset != 0 || relocation_count != 0)
	{
		Dumper::Region relocation_region("Relocations", file_offset + relocation_offset, 4 * relocation_count, 8);
		header_region.AddField("Count", Dumper::DecDisplay::Make(), offset_t(relocation_count));
		relocation_region.Display(dump);

		uint32_t relocation_index = 0;
		for(auto relocation : relocations)
		{
			Dumper::Entry relocation_entry("Relocation", relocation_index + 1, file_offset + relocation_offset + 4 * relocation_index, 8);
			relocation_entry.AddField("Value", Dumper::BitFieldDisplay::Make(8)
				->AddBitField(0, 31, "Offset", Dumper::HexDisplay::Make(8))
				->AddBitField(31, 1, "Size", Dumper::ChoiceDisplay::Make("4", "2")),
				offset_t(relocation.value));
			relocation_entry.Display(dump);
			relocation_index++;
		}
	}

	Dumper::Block image_block("Image", file_offset + header_size, image->AsImage(), 0, 8);
	for(auto relocation : relocations)
	{
		image_block.AddSignal(relocation.offset, relocation.rel32 ? 4 : 2);
	}
	image_block.Display(dump);
}

std::string MPFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	if(stub.filename != "")
	{
		return filename + ".exe";
	}
	else if(has_relocations)
	{
		return filename + ".rex";
	}
	else
	{
		return filename + ".exp";
	}
}

// P3Format

void P3Format::ReadFile(Linker::Reader& rd)
{
	//file_offset = Microsoft::FindActualSignature(rd, signature, "P3", "P2");
	/* TODO */
}

bool P3Format::FormatSupportsSegmentation() const
{
	return is_multisegmented; /* TODO: this probably does not actually support this? */
}

bool P3Format::FormatIs16bit() const
{
	return !is_32bit;
}

bool P3Format::FormatIsProtectedMode() const
{
	return true;
}

#if 0
bool P3Format::FormatSupportsStackSection() const
{
	return true;
}
#endif

void P3Format::RunTimeParameterBlock::ReadFile(Linker::Reader& rd)
{
	min_realmode_param = rd.ReadUnsigned(2);
	max_realmode_param = rd.ReadUnsigned(2);
	min_int_buffer_size_kb = rd.ReadUnsigned(2);
	max_int_buffer_size_kb = rd.ReadUnsigned(2);
	int_stack_count = rd.ReadUnsigned(2);
	int_stack_size_kb = rd.ReadUnsigned(2);
	realmode_area_end = rd.ReadUnsigned(4);
	call_buffer_size_kb = rd.ReadUnsigned(2);
	flags = rd.ReadUnsigned(2);
	ring = rd.ReadUnsigned(2);
}

void P3Format::RunTimeParameterBlock::CalculateValues()
{
	/* set up Watcom parameter */
	min_realmode_param = 0;
	max_realmode_param = 0;
	min_int_buffer_size_kb = 0x0001;
	max_int_buffer_size_kb = 0x0004;
	int_stack_count = 4;
	int_stack_size_kb = 0x0001;
	realmode_area_end = 0;
	call_buffer_size_kb = 0;
	flags = 0;
	ring = 0;
}

void P3Format::RunTimeParameterBlock::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, min_realmode_param);
	wr.WriteWord(2, max_realmode_param);
	wr.WriteWord(2, min_int_buffer_size_kb);
	wr.WriteWord(2, max_int_buffer_size_kb);
	wr.WriteWord(2, int_stack_count);
	wr.WriteWord(2, int_stack_size_kb);
	wr.WriteWord(4, realmode_area_end);
	wr.WriteWord(2, call_buffer_size_kb);
	wr.WriteWord(2, flags);
	wr.WriteWord(2, ring);
}

std::shared_ptr<Linker::OptionCollector> P3Format::GetOptions()
{
	return std::make_shared<P3OptionCollector>();
}

void P3Format::SetOptions(std::map<std::string, std::string>& options)
{
	P3OptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
	stack_size = collector.stack();
}

std::string P3Format::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	if(stub.filename != "")
	{
		return filename + ".exe";
	}
	else
	{
		return filename + ".exp";
	}
}

offset_t P3Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	if(stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}
	wr.Seek(file_offset);
	wr.WriteData(2, is_32bit ? "P3" : "P2");
	wr.WriteWord(2, is_multisegmented ? 2 : 1);
	wr.WriteWord(2, header_size);
	wr.WriteWord(4, file_size);
	wr.WriteWord(2, checksum16); /* TODO */
	wr.WriteWord(4, runtime_parameters_offset);
	wr.WriteWord(4, runtime_parameters_size);
	wr.WriteWord(4, relocation_table_offset);
	wr.WriteWord(4, relocation_table_size);
	wr.WriteWord(4, segment_information_table_offset);
	wr.WriteWord(4, segment_information_table_size);
	wr.WriteWord(2, segment_information_table_entry_size);
	wr.WriteWord(4, load_image_offset);
	wr.WriteWord(4, load_image_size);
	wr.WriteWord(4, symbol_table_offset);
	wr.WriteWord(4, symbol_table_size);
	wr.WriteWord(4, gdt_address);
	wr.WriteWord(4, gdt_size);
	wr.WriteWord(4, ldt_address);
	wr.WriteWord(4, ldt_size);
	wr.WriteWord(4, idt_address);
	wr.WriteWord(4, idt_size);
	wr.WriteWord(4, tss_address);
	wr.WriteWord(4, tss_size);
	wr.WriteWord(4, minimum_extra);
	wr.WriteWord(4, maximum_extra);
	wr.WriteWord(4, base_load_offset);
	wr.WriteWord(4, esp);
	wr.WriteWord(2, ss);
	wr.WriteWord(4, eip);
	wr.WriteWord(2, cs);
	wr.WriteWord(2, ldtr);
	wr.WriteWord(2, tr);
	wr.WriteWord(2, flags);
	wr.WriteWord(4, memory_requirements);
	wr.WriteWord(4, checksum32); /* TODO */
	wr.WriteWord(4, stack_size);

	return offset_t(-1);
}

void P3Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("P2/P3 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

P3Format::AbstractSegment::~AbstractSegment()
{
}

P3Format::Descriptor P3Format::Descriptor::FromSegment(uint32_t access, std::weak_ptr<AbstractSegment> image)
{
	Descriptor descriptor;
	descriptor.image = image;
	descriptor.access = access;
	return descriptor;
}

P3Format::Descriptor P3Format::Descriptor::ReadEntry(Linker::Image& image, offset_t offset)
{
	Descriptor descriptor;
	descriptor.access = image.ReadUnsigned(4, offset + 4, ::LittleEndian);
	if(descriptor.IsGate())
	{
		descriptor.offset = image.ReadUnsigned(2, offset, ::LittleEndian);
		descriptor.offset |= descriptor.access & 0xFFFF0000;
		descriptor.selector = image.ReadUnsigned(3, offset + 2, ::LittleEndian);
		descriptor.access &= 0x0000FF00;
	}
	else
	{
		descriptor.limit = image.ReadUnsigned(2, offset, ::LittleEndian);
		descriptor.limit |= descriptor.access & 0x000F0000;
		descriptor.base = image.ReadUnsigned(3, offset + 2, ::LittleEndian);
		descriptor.base |= descriptor.access & 0xFF000000;
		if((descriptor.access & DESC_G) != 0)
			descriptor.limit <<= 12;
		descriptor.access &= 0x00F0FF00;
	}
	return descriptor;
}

bool P3Format::Descriptor::IsSegment() const
{
	if((access & DESC_S) != 0)
		return true;
	else switch(access & 0x00000700)
	{
	case 0x100:
	case 0x200:
	case 0x300:
		return true;
	default:
		return false;
	}
}

bool P3Format::Descriptor::IsGate() const
{
	if((access & DESC_S) != 0)
		return false;
	else switch(access & 0x00000700)
	{
	case 0x400:
	case 0x500:
	case 0x600:
	case 0x700:
		return true;
	default:
		return false;
	}
}

void P3Format::Descriptor::CalculateValues()
{
	std::shared_ptr<AbstractSegment> segment = image.lock();
	uint32_t size = segment ? segment->GetLoadedSize() : 0;
	if(size != 0)
		size -= 1;
	if(size > 0xFFFFF)
	{
		access |= DESC_G;
		limit >>= 12;
	}
	else
	{
		access &= ~DESC_G;
	}
	limit = size;
	base = segment ? segment->address : 0;
}

void P3Format::Descriptor::WriteEntry(Linker::Writer& wr) const
{
	if(IsGate())
	{
		wr.WriteWord(2, offset & 0xFFFF);
		wr.WriteWord(2, selector);
		wr.WriteWord(2, access & 0xFFFF);
		wr.WriteWord(2, offset >> 16);
	}
	else
	{
		wr.WriteWord(2, limit & 0xFFFF);
		wr.WriteWord(3, base & 0xFFFFFF);
		wr.WriteWord(2, (access | (limit & 0xF0000)) >> 8);
		wr.WriteWord(1, base >> 24);
	}
}

void P3Format::Descriptor::FillEntry(Dumper::Entry& entry) const
{
	static const std::map<offset_t, std::string> type_description =
	{
		{ 1, "16-bit available TSS" },
		{ 2, "LDT" },
		{ 3, "16-bit busy TSS" },
		{ 4, "16-bit call gate" },
		{ 5, "task gate" },
		{ 6, "16-bit interrupt gate" },
		{ 7, "16-bit trap gate" },
		{ 9, "32-bit available TSS" },
		{ 11, "32-bit busy TSS" },
		{ 12, "32-bit call gate" },
		{ 14, "32-bit interrupt gate" },
		{ 15, "32-bit trap gate" },
	};

	if(IsGate())
	{
		entry.AddField("Gate selector", Dumper::HexDisplay::Make(4), offset_t(selector));
		entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(offset));
		entry.AddField("Access", Dumper::BitFieldDisplay::Make(8)
			->AddBitField(0, 5, "parameter count", Dumper::DecDisplay::Make(), true)
			->AddBitField(8, 4, Dumper::ChoiceDisplay::Make(type_description), false)
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("segment", "system segment"), false)
			->AddBitField(13, 2, "DPL", Dumper::DecDisplay::Make(), false)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("present", "absent"), false),
			offset_t(access));
	}
	else
	{
		entry.AddField("Base", Dumper::HexDisplay::Make(8), offset_t(base));
		entry.AddField("Limit", Dumper::HexDisplay::Make(8), offset_t(limit));
		if((access & DESC_S) == 0)
		{
			entry.AddField("Access", Dumper::BitFieldDisplay::Make(8)
				->AddBitField(8, 4, Dumper::ChoiceDisplay::Make(type_description), false)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("segment", "system segment"), false)
				->AddBitField(13, 2, "DPL", Dumper::DecDisplay::Make(), false)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("present", "absent"), false)
				->AddBitField(20, 1, Dumper::ChoiceDisplay::Make("available flag"), true)
				->AddBitField(23, 1, Dumper::ChoiceDisplay::Make("granularity"), true),
				offset_t(access));
		}
		else
		{
			entry.AddField("Access", Dumper::BitFieldDisplay::Make(8)
				->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("accessed"), true)
				->AddBitField(9, 1, Dumper::ChoiceDisplay::Make(
					access & DESC_X ? "readable" : "writable",
					access & DESC_X ? "execute-only" : "read-only"), false)
				->AddBitField(10, 1, Dumper::ChoiceDisplay::Make(
					access & DESC_X ? "conforming" : "expand-down"), true)
				->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("code", "data"), false)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("segment", "system segment"), false)
				->AddBitField(13, 2, "DPL", Dumper::DecDisplay::Make(), false)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("present", "absent"), false)
				->AddBitField(20, 1, Dumper::ChoiceDisplay::Make("available flag"), true)
				->AddBitField(22, 1, Dumper::ChoiceDisplay::Make("32-bit", "16-bit"), false)
				->AddBitField(23, 1, Dumper::ChoiceDisplay::Make("granularity"), true),
				offset_t(access));

			if(auto sit_entry = std::dynamic_pointer_cast<SITEntry>(image.lock()))
			{
				entry.AddOptionalField("SIT flags", Dumper::HexDisplay::Make(4), offset_t(sit_entry->flags));
				entry.AddOptionalField("Base offset", Dumper::HexDisplay::Make(4), offset_t(sit_entry->base_offset));
				entry.AddOptionalField("Zero fill", Dumper::HexDisplay::Make(4), offset_t(sit_entry->zero_fill));
			}
		}
	}
}

uint32_t P3Format::DescriptorTable::GetStoredSize() const
{
	return descriptors.size() * 8;
}

uint32_t P3Format::DescriptorTable::GetLoadedSize() const
{
	return descriptors.size() * 8;
}

void P3Format::DescriptorTable::WriteFile(Linker::Writer& wr) const
{
	for(auto& descriptor : descriptors)
	{
		descriptor.WriteEntry(wr);
	}
}

void P3Format::DescriptorTable::CalculateValues()
{
	for(auto& descriptor : descriptors)
	{
		descriptor.CalculateValues();
	}
}

uint32_t P3Format::TaskStateSegment::GetStoredSize() const
{
	return is_32bit ? 0x68 : 0x2C;
}

uint32_t P3Format::TaskStateSegment::GetLoadedSize() const
{
	return is_32bit ? 0x68 : 0x2C;
}

void P3Format::TaskStateSegment::WriteFile(Linker::Writer& wr) const
{
	if(is_32bit)
	{
		wr.WriteWord(4, link);
		wr.WriteWord(4, esp0);
		wr.WriteWord(4, ss0);
		wr.WriteWord(4, esp1);
		wr.WriteWord(4, ss1);
		wr.WriteWord(4, esp2);
		wr.WriteWord(4, ss2);
		wr.WriteWord(4, cr3);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, eflags);
		wr.WriteWord(4, eax);
		wr.WriteWord(4, ecx);
		wr.WriteWord(4, edx);
		wr.WriteWord(4, ebx);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, ebp);
		wr.WriteWord(4, esi);
		wr.WriteWord(4, edi);
		wr.WriteWord(4, es);
		wr.WriteWord(4, cs);
		wr.WriteWord(4, ss);
		wr.WriteWord(4, ds);
		wr.WriteWord(4, fs);
		wr.WriteWord(4, gs);
		wr.WriteWord(4, ldtr);
		wr.WriteWord(4, uint32_t(iopb) << 16);
	}
	else
	{
		wr.WriteWord(2, link);
		wr.WriteWord(2, esp0);
		wr.WriteWord(2, ss0);
		wr.WriteWord(2, esp1);
		wr.WriteWord(2, ss1);
		wr.WriteWord(2, esp2);
		wr.WriteWord(2, ss2);
		wr.WriteWord(2, eip);
		wr.WriteWord(2, eflags);
		wr.WriteWord(2, eax);
		wr.WriteWord(2, ecx);
		wr.WriteWord(2, edx);
		wr.WriteWord(2, ebx);
		wr.WriteWord(2, esp);
		wr.WriteWord(2, ebp);
		wr.WriteWord(2, esi);
		wr.WriteWord(2, edi);
		wr.WriteWord(2, es);
		wr.WriteWord(2, cs);
		wr.WriteWord(2, ss);
		wr.WriteWord(2, ds);
		wr.WriteWord(2, ldtr);
	}
}

void P3Format::TaskStateSegment::ReadImage(Linker::Image& image, offset_t offset)
{
	if(is_32bit)
	{
		link = image.ReadUnsigned(4, offset + 0x00, ::LittleEndian);
		esp0 = image.ReadUnsigned(4, offset + 0x04, ::LittleEndian);
		ss0 = image.ReadUnsigned(4, offset + 0x08, ::LittleEndian);
		esp1 = image.ReadUnsigned(4, offset + 0x0C, ::LittleEndian);
		ss1 = image.ReadUnsigned(4, offset + 0x10, ::LittleEndian);
		esp2 = image.ReadUnsigned(4, offset + 0x14, ::LittleEndian);
		ss2 = image.ReadUnsigned(4, offset + 0x18, ::LittleEndian);
		cr3 = image.ReadUnsigned(4, offset + 0x1C, ::LittleEndian);
		eip = image.ReadUnsigned(4, offset + 0x20, ::LittleEndian);
		eflags = image.ReadUnsigned(4, offset + 0x24, ::LittleEndian);
		eax = image.ReadUnsigned(4, offset + 0x28, ::LittleEndian);
		ecx = image.ReadUnsigned(4, offset + 0x2C, ::LittleEndian);
		edx = image.ReadUnsigned(4, offset + 0x30, ::LittleEndian);
		ebx = image.ReadUnsigned(4, offset + 0x34, ::LittleEndian);
		esp = image.ReadUnsigned(4, offset + 0x38, ::LittleEndian);
		ebp = image.ReadUnsigned(4, offset + 0x3C, ::LittleEndian);
		esi = image.ReadUnsigned(4, offset + 0x40, ::LittleEndian);
		edi = image.ReadUnsigned(4, offset + 0x44, ::LittleEndian);
		es = image.ReadUnsigned(4, offset + 0x48, ::LittleEndian);
		cs = image.ReadUnsigned(4, offset + 0x4C, ::LittleEndian);
		ss = image.ReadUnsigned(4, offset + 0x50, ::LittleEndian);
		ds = image.ReadUnsigned(4, offset + 0x54, ::LittleEndian);
		fs = image.ReadUnsigned(4, offset + 0x58, ::LittleEndian);
		gs = image.ReadUnsigned(4, offset + 0x5C, ::LittleEndian);
		ldtr = image.ReadUnsigned(4, offset + 0x60, ::LittleEndian);
		iopb = image.ReadUnsigned(2, offset + 0x66, ::LittleEndian);
	}
	else
	{
		link = image.ReadUnsigned(2, offset + 0x00, ::LittleEndian);
		esp0 = image.ReadUnsigned(2, offset + 0x02, ::LittleEndian);
		ss0 = image.ReadUnsigned(2, offset + 0x04, ::LittleEndian);
		esp1 = image.ReadUnsigned(2, offset + 0x06, ::LittleEndian);
		ss1 = image.ReadUnsigned(2, offset + 0x08, ::LittleEndian);
		esp2 = image.ReadUnsigned(2, offset + 0x0A, ::LittleEndian);
		ss2 = image.ReadUnsigned(2, offset + 0x0C, ::LittleEndian);
		eip = image.ReadUnsigned(2, offset + 0x0E, ::LittleEndian);
		eflags = image.ReadUnsigned(2, offset + 0x10, ::LittleEndian);
		eax = image.ReadUnsigned(2, offset + 0x12, ::LittleEndian);
		ecx = image.ReadUnsigned(2, offset + 0x14, ::LittleEndian);
		edx = image.ReadUnsigned(2, offset + 0x16, ::LittleEndian);
		ebx = image.ReadUnsigned(2, offset + 0x18, ::LittleEndian);
		esp = image.ReadUnsigned(2, offset + 0x1A, ::LittleEndian);
		ebp = image.ReadUnsigned(2, offset + 0x1C, ::LittleEndian);
		esi = image.ReadUnsigned(2, offset + 0x1E, ::LittleEndian);
		edi = image.ReadUnsigned(2, offset + 0x20, ::LittleEndian);
		es = image.ReadUnsigned(2, offset + 0x22, ::LittleEndian);
		cs = image.ReadUnsigned(2, offset + 0x24, ::LittleEndian);
		ss = image.ReadUnsigned(2, offset + 0x26, ::LittleEndian);
		ds = image.ReadUnsigned(2, offset + 0x28, ::LittleEndian);
		ldtr = image.ReadUnsigned(2, offset + 0x2A, ::LittleEndian);
	}
}

void P3Format::TaskStateSegment::FillEntries(Dumper::Region& region) const
{
	region.AddOptionalField("Link", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(link));
	region.AddOptionalField(is_32bit ? "SS0:ESP0" : "SS0:SP0", Dumper::SegmentedDisplay::Make(is_32bit ? 8 : 4), offset_t(ss0), offset_t(esp0));
	region.AddOptionalField(is_32bit ? "SS1:ESP1" : "SS1:SP1", Dumper::SegmentedDisplay::Make(is_32bit ? 8 : 4), offset_t(ss1), offset_t(esp1));
	region.AddOptionalField(is_32bit ? "SS2:ESP2" : "SS2:SP2", Dumper::SegmentedDisplay::Make(is_32bit ? 8 : 4), offset_t(ss2), offset_t(esp2));
	if(is_32bit)
		region.AddOptionalField("CR3", Dumper::HexDisplay::Make(8), offset_t(cr3));
	region.AddOptionalField(is_32bit ? "EIP" : "IP", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(eip));
	region.AddOptionalField(is_32bit ? "EFLAGS" : "FLAGS", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(eflags));
	region.AddOptionalField(is_32bit ? "EAX" : "AX", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(eax));
	region.AddOptionalField(is_32bit ? "ECX" : "CX", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(ecx));
	region.AddOptionalField(is_32bit ? "EDX" : "DX", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(edx));
	region.AddOptionalField(is_32bit ? "EBX" : "BX", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(ebx));
	region.AddOptionalField(is_32bit ? "ESP" : "SP", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(esp));
	region.AddOptionalField(is_32bit ? "EBP" : "BP", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(ebp));
	region.AddOptionalField(is_32bit ? "ESI" : "SI", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(esi));
	region.AddOptionalField(is_32bit ? "EDI" : "DI", Dumper::HexDisplay::Make(is_32bit ? 8 : 4), offset_t(edi));
	region.AddOptionalField("ES", Dumper::HexDisplay::Make(4), offset_t(es));
	region.AddOptionalField("CS", Dumper::HexDisplay::Make(4), offset_t(cs));
	region.AddOptionalField("SS", Dumper::HexDisplay::Make(4), offset_t(ss));
	region.AddOptionalField("DS", Dumper::HexDisplay::Make(4), offset_t(ds));
	if(is_32bit)
	{
		region.AddOptionalField("FS", Dumper::HexDisplay::Make(4), offset_t(fs));
		region.AddOptionalField("GS", Dumper::HexDisplay::Make(4), offset_t(gs));
	}
	region.AddOptionalField("LDTR", Dumper::HexDisplay::Make(4), offset_t(ldtr));
	if(is_32bit)
		region.AddOptionalField("IOPB", Dumper::HexDisplay::Make(4), offset_t(iopb));
}

uint32_t P3Format::SITEntry::GetStoredSize() const
{
	// TODO
	return 0;
}

uint32_t P3Format::SITEntry::GetZeroSize() const
{
	return zero_fill;
}

uint32_t P3Format::SITEntry::GetLoadedSize() const
{
	return GetStoredSize() + GetZeroSize();
}

void P3Format::SITEntry::WriteSITEntry(Linker::Writer& wr) const
{
	wr.WriteWord(2, selector);
	wr.WriteWord(2, flags);
	wr.WriteWord(4, base_offset);
	wr.WriteWord(4, GetZeroSize());
}

void P3Format::SITEntry::WriteFile(Linker::Writer& wr) const
{
	// TODO
}

std::shared_ptr<P3Format::SITEntry> P3Format::SITEntry::ReadSITEntry(Linker::Reader& rd)
{
	std::shared_ptr<SITEntry> segment = std::make_shared<SITEntry>();
	segment->selector = rd.ReadUnsigned(2);
	segment->flags = rd.ReadUnsigned(2);
	segment->base_offset = rd.ReadUnsigned(4);
	segment->zero_fill = rd.ReadUnsigned(4);
	return segment;
}

uint32_t P3Format::Segment::GetStoredSize() const
{
	return segment->data_size;
}

uint32_t P3Format::Segment::GetZeroSize() const
{
	return segment->zero_fill;
}

void P3Format::Segment::WriteFile(Linker::Writer& wr) const
{
	segment->WriteFile(wr);
}

bool P3Format::Relocation::operator ==(const Relocation& other) const
{
	return selector == other.selector && offset == other.offset;
}

bool P3Format::Relocation::operator <(const Relocation& other) const
{
	return selector < other.selector || (selector == other.selector && offset < other.offset);
}

void P3Format::Relocation::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, offset);
	wr.WriteWord(2, selector);
}

std::shared_ptr<Linker::Segment> P3Format::Flat::GetSegment()
{
	return std::static_pointer_cast<Linker::Segment>(image);
}

std::shared_ptr<const Linker::Segment> P3Format::Flat::GetSegment() const
{
	return std::static_pointer_cast<const Linker::Segment>(image);
}

void P3Format::Flat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		image = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

std::unique_ptr<Script::List> P3Format::Flat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	all not write align 4;
	align 4;

	all not zero align 4;
	align 4;

	all align 4;
	align 4
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void P3Format::Flat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void P3Format::Flat::ProcessModule(Linker::Module& module)
{
	module.AllocateStack(stack_size);

	Link(module);

	minimum_extra = GetSegment()->zero_fill;
	maximum_extra = GetSegment()->optional_extra;
	if(maximum_extra == 0)
	{
		maximum_extra = -1;
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		esp = stack_top.GetPosition().address;
		if(stack_size == 0)
		{
			// make the entire section data until .stack_top part of the stack
			stack_size = stack_top.offset;
		}

		// TODO: if stack only had a default value, completely override it
	}
	else
	{
		auto stack_section = module.FindSection(".stack");
		if(stack_section != nullptr && stack_section->Size() != 0)
		{
			// if .stack exists, use it as the stack area
			esp = GetSegment()->TotalSize();
			if(stack_section->Size() > stack_size)
			{
				stack_size = stack_section->Size();
			}
		}
		else
		{
			Linker::Warning << "Warning: no stack found" << std::endl;
			stack_size = 0x1000;
			minimum_extra += stack_size;
			esp = GetSegment()->TotalSize() + stack_size;
		}
	}
	ss = 0;

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		eip = entry.GetPosition().address;
	}
	else
	{
		Linker::Warning << "Warning: no entry found, using 0" << std::endl;
		eip = 0;
	}
	cs = 0;

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::Direct)
		{
			rel.WriteWord(resolution.value);
		}
		else if(rel.kind == Linker::Relocation::SelectorIndex)
		{
			Linker::Error << "Error: segment relocations impossible in flat memory model, ignoring" << std::endl;
			continue;
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}
}

void P3Format::Flat::CalculateValues()
{
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;

	header_size = 0x180;

	segment_information_table_offset = 0;
	segment_information_table_entry_size = 0;
	segment_information_table_size = 0;

	relocation_table_offset = 0;
	relocation_table_size = 0;

	runtime_parameters_offset = header_size;
	runtime_parameters_size = 0x80;
	runtime_parameters.CalculateValues();

	load_image_offset = runtime_parameters_offset + runtime_parameters_size;
	load_image_size = GetSegment()->data_size;
	memory_requirements = load_image_size; /* TODO: ? */

	file_size = load_image_offset + load_image_size;

	tss_address = 0;
	tss_size = 0;

	gdt_address = 0;
	gdt_size = 0;

	idt_address = 0;
	idt_size = 0;

	ldt_address = 0;
	ldt_size = 0;

	base_load_offset = 0;

	flags = 0;

	/* TODO: position in file? */
	symbol_table_offset = 0;
	symbol_table_size = 0;
}

offset_t P3Format::Flat::WriteFile(Linker::Writer& wr) const
{
	P3Format::WriteFile(wr);

	wr.Seek(file_offset + runtime_parameters_offset);
	runtime_parameters.WriteFile(wr);

	wr.Seek(file_offset + load_image_offset);
	image->WriteFile(wr);

	return offset_t(-1);
}

void P3Format::Flat::Dump(Dumper::Dumper& dump) const
{
	P3Format::Dump(dump);
	// TODO
}

void P3Format::MultiSegmented::OnNewSegment(std::shared_ptr<Linker::Segment> linker_segment)
{
	std::shared_ptr<Segment> segment = std::make_shared<Segment>(linker_segment,
		/* TODO: check properly read, write, 32-bit (TODO) flags */
		linker_segment->sections[0]->IsExecutable() ? Descriptor::Code32 : Descriptor::Data32,
		ldt->descriptors.size() * 8 + 4);

	segments.push_back(segment);
	segment_associations[linker_segment] = segments.size() - 1;
	ldt->descriptors.push_back(Descriptor::FromSegment(segment->access, segment));

	if(linker_segment->name == ".code")
	{
		code = segment;
	}
	else if(linker_segment->name == ".data")
	{
		data = segment;
	}
}

std::unique_ptr<Script::List> P3Format::MultiSegmented::GetScript(Linker::Module& module)
{
	static const char * SegmentedScript = R"(
".code"
{
	base here;
	all exec;
	align 4;
};

".data"
{
	at 0;
	base here;
	all ".data" or ".rodata";
	align 4;
	all ".bss" or ".comm";
	align 4;
	all ".stack";
	align 4;
};

for any
{
	at 0;
	base here;
	all not zero;
	all zero;
	align 4;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SegmentedScript);
	}
}

void P3Format::MultiSegmented::Link(Linker::Module& module)
{
	/* WATCOM set up */
	gdt->descriptors.push_back(Descriptor::FromSegment(0));
	tr = gdt->descriptors.size() * 8;
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::TSS32, tss));
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::Data16, tss));
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::Data16, gdt));
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::Data16, idt));
	tss->ldtr = ldtr = gdt->descriptors.size() * 8;
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::LDT, ldt));
	gdt->descriptors.push_back(Descriptor::FromSegment(Descriptor::Data16, ldt));

	idt->descriptors.push_back(Descriptor::FromSegment(0));

	ldt->descriptors.push_back(Descriptor::FromSegment(0));

	segments.push_back(tss);
	segments.push_back(gdt);
	segments.push_back(idt);
	segments.push_back(ldt);

	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void P3Format::MultiSegmented::ProcessModule(Linker::Module& module)
{
	// TODO: the stack generation code update has not been tested

	module.AllocateStack(stack_size);

	Link(module);

	std::shared_ptr<Linker::Segment> stack_segment;
	std::shared_ptr<Linker::Section> stack_section;
	if((stack_section = module.FindSection(".stack")))
	{
		stack_segment = stack_section->segment.lock();
	}
	else
	{
		stack_segment = nullptr;
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		esp = position.address;
		ss = std::static_pointer_cast<Segment>(segments[segment_associations[position.segment]])->selector;
		if(stack_size == 0)
		{
			// make the entire section data until .stack_top part of the stack
			stack_size = stack_top.offset;
		}

		// TODO: if stack only had a default value, completely override it

		if(stack_segment != nullptr && stack_segment != position.segment)
		{
			Linker::Warning << "Warning: stack_top not within .stack segment" << std::endl;
		}
	}
	else
	{
		if(stack_segment != nullptr)
		{
			/* Attempt to use .stack segment */
			std::shared_ptr<Segment> segment = std::static_pointer_cast<Segment>(segments[segment_associations[stack_segment]]);
			esp = segment->GetLoadedSize();
			ss = segment->selector;
			assert(stack_section != nullptr);
			if(stack_section->Size() > stack_size)
			{
				stack_size = stack_section->Size();
			}
		}
		else if(data != nullptr)
		{
			Linker::Warning << "Warning: no stack found, using data segment" << std::endl;
			stack_size = 0x1000;
			data->segment->zero_fill += stack_size;
			esp = data->segment->TotalSize();
			ss = data->selector;
		}
		else
		{
			Linker::Error << "Error: no stack or data segment found" << std::endl;
			esp = 0;
			ss = 0;
		}
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		eip = position.address;
		cs = std::static_pointer_cast<Segment>(segments[segment_associations[position.segment]])->selector;
	}
	else
	{
		if(code == nullptr)
		{
			Linker::Error << "Error: no entry or code segment found" << std::endl;
			eip = 0;
			cs = 0;
		}
		else
		{
			Linker::Warning << "Warning: no entry found, using 0" << std::endl;
			eip = 0;
			cs = code->selector;
		}
	}

	std::set<Relocation> relocations_set;

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::Direct)
		{
			rel.WriteWord(resolution.value);
		}
		else if(rel.kind == Linker::Relocation::SelectorIndex)
		{
			if(resolution.target == nullptr || resolution.reference != nullptr || resolution.value != 0 || rel.addend != 0)
			{
				Linker::Error << "Error: intersegment differences impossible in protected mode, ignoring" << std::endl;
				continue;
			}
			Linker::Position source = rel.source.GetPosition();
			relocations_set.insert(Relocation(std::static_pointer_cast<Segment>(segments[segment_associations[source.segment]])->selector, source.address));
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}

	relocations.clear();
	for(auto relocation : relocations_set)
	{
		relocations.push_back(relocation);
	}
}

void P3Format::MultiSegmented::CalculateValues()
{
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;

	header_size = 0x180;

	segment_information_table_offset = header_size;
	segment_information_table_entry_size = 0x0C;
	uint32_t sit_entry_count = 0;
	for(auto segment : segments)
	{
		if(std::dynamic_pointer_cast<Segment>(segment))
		{
			sit_entry_count ++;
		}
	}
	segment_information_table_size = sit_entry_count * segment_information_table_entry_size;

	relocation_table_offset = segment_information_table_offset + segment_information_table_size;
	relocation_table_size = relocations.size() * 6;

	runtime_parameters_offset = relocation_table_offset + relocation_table_size; /* TODO: header_size for flat */
	runtime_parameters_size = 0x80;
	runtime_parameters.CalculateValues();

	load_image_offset = runtime_parameters_offset + runtime_parameters_size;
	load_image_size = 0;
	for(auto segment : segments)
	{
		load_image_size += segment->GetStoredSize();
	}
	memory_requirements = load_image_size; /* TODO: ? */

	file_size = load_image_offset + load_image_size;

	tss_address = tss->address = 0;
	tss_size = tss->GetStoredSize();

	gdt_address = gdt->address = tss_address + tss_size;
	gdt_size = gdt->GetStoredSize();

	idt_address = idt->address = gdt_address + gdt_size;
	idt_size = idt->GetStoredSize();

	ldt_address = ldt->address = idt_address + idt_size;
	ldt_size = ldt->GetStoredSize();

	uint32_t offset = ldt_address + ldt_size;
	for(auto abstract_segment : segments)
	{
		if(auto segment = std::dynamic_pointer_cast<Segment>(abstract_segment))
		{
			segment->address = offset;
			offset += segment->GetStoredSize();
		}
	}

	gdt->CalculateValues();
	ldt->CalculateValues();
	idt->CalculateValues();

	minimum_extra = 0;
	maximum_extra = 0;

	base_load_offset = 0;

	tss->esp = esp;
	tss->ds = data ? data->selector : ss;
	tss->ss = ss;
	tss->cs = cs;

	flags = 0;

	/* TODO: position in file? */
	symbol_table_offset = 0;
	symbol_table_size = 0;
}

offset_t P3Format::MultiSegmented::WriteFile(Linker::Writer& wr) const
{
	P3Format::WriteFile(wr);

	wr.Seek(file_offset + segment_information_table_offset);
	for(auto abstract_segment : segments)
	{
		if(auto segment = std::dynamic_pointer_cast<Segment>(abstract_segment))
		{
			segment->WriteSITEntry(wr);
		}
	}

	wr.Seek(file_offset + relocation_table_offset);
	for(auto& relocation : relocations)
	{
		relocation.WriteFile(wr);
	}

	wr.Seek(file_offset + runtime_parameters_offset);
	runtime_parameters.WriteFile(wr);

	wr.Seek(file_offset + load_image_offset);
	for(auto segment : segments)
	{
		segment->WriteFile(wr);
	}

	return offset_t(-1);
}

void P3Format::MultiSegmented::Dump(Dumper::Dumper& dump) const
{
	P3Format::Dump(dump);
	// TODO
}

////

void P3Format::External::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	std::array<char, 2> signature;
	file_offset = Microsoft::FindActualSignature(rd, signature, "P3", "P2");
	if(signature[0] == 'P' && signature[1] == '3')
	{
		is_32bit = true;
	}
	else if(signature[0] == 'P' && signature[1] == '2')
	{
		is_32bit = false;
	}
	else
	{
		Linker::Error << "Error: invalid signature, assuming 32-bit" << std::endl;
	}
	uint16_t level = rd.ReadUnsigned(2);
	switch(level)
	{
	case 1:
		is_multisegmented = false;
		break;
	case 2:
		is_multisegmented = true;
		break;
	default:
		Linker::Error << "Error: invalid P2/P3 format level " << level << std::endl;
		is_multisegmented = true;
		break;
	}
	header_size = rd.ReadUnsigned(2);
	file_size = rd.ReadUnsigned(4);
	checksum16 = rd.ReadUnsigned(2);
	runtime_parameters_offset = rd.ReadUnsigned(4);
	runtime_parameters_size = rd.ReadUnsigned(4);
	relocation_table_offset = rd.ReadUnsigned(4);
	relocation_table_size = rd.ReadUnsigned(4);
	segment_information_table_offset = rd.ReadUnsigned(4);
	segment_information_table_size = rd.ReadUnsigned(4);
	segment_information_table_entry_size = rd.ReadUnsigned(2);
	load_image_offset = rd.ReadUnsigned(4);
	load_image_size = rd.ReadUnsigned(4);
	symbol_table_offset = rd.ReadUnsigned(4);
	symbol_table_size = rd.ReadUnsigned(4);
	gdt_address = rd.ReadUnsigned(4);
	gdt_size = rd.ReadUnsigned(4);
	ldt_address = rd.ReadUnsigned(4);
	ldt_size = rd.ReadUnsigned(4);
	idt_address = rd.ReadUnsigned(4);
	idt_size = rd.ReadUnsigned(4);
	tss_address = rd.ReadUnsigned(4);
	tss_size = rd.ReadUnsigned(4);
	minimum_extra = rd.ReadUnsigned(4);
	maximum_extra = rd.ReadUnsigned(4);
	base_load_offset = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	ss = rd.ReadUnsigned(2);
	eip = rd.ReadUnsigned(4);
	cs = rd.ReadUnsigned(2);
	ldtr = rd.ReadUnsigned(2);
	tr = rd.ReadUnsigned(2);
	flags = rd.ReadUnsigned(2);
	memory_requirements = rd.ReadUnsigned(4);
	checksum32 = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);

	/* Segment Information Table */

	if(segment_information_table_size != 0 && segment_information_table_entry_size != 0)
	{
		for(uint32_t sit_offset = 0; sit_offset + segment_information_table_entry_size <= segment_information_table_size; sit_offset += segment_information_table_entry_size)
		{
			rd.Seek(file_offset + segment_information_table_offset + sit_offset);
			segments.push_back(SITEntry::ReadSITEntry(rd));
		}
	}

	/* Relocation Table */

	if(relocation_table_size != 0)
	{
		rd.Seek(file_offset + relocation_table_offset);
		for(uint32_t relocation_offset = 0; relocation_offset < relocation_table_size; relocation_offset += (is_32bit ? 6 : 4))
		{
			Relocation relocation{0, 0};
			relocation.offset = rd.ReadUnsigned(is_32bit ? 4 : 2);
			relocation.selector = rd.ReadUnsigned(2);
			relocations.push_back(relocation);
		}
	}

	/* Runtime Parameters */

	if(runtime_parameters_size != 0)
	{
		rd.Seek(file_offset + runtime_parameters_offset);
		runtime_parameters.ReadFile(rd); // TODO: pass runtime_parameters_size as parameter
	}

	/* Symbol Table */

	// TODO

	/* Load Image */

	rd.Seek(file_offset + load_image_offset);
	image = Linker::Buffer::ReadFromFile(rd, file_size - load_image_offset);

	/* Task State Segment */

	tss->is_32bit = is_32bit;
	if(tss_size != 0)
	{
		tss->ReadImage(*image.get(), tss_address); // TODO: do not read more than tss_size
	}

	/* Global Descriptor Table */

	if(gdt_size != 0)
	{
		for(uint32_t gdt_offset = 0; gdt_offset < gdt_size; gdt_offset += 8)
		{
			gdt->descriptors.push_back(Descriptor::ReadEntry(*image.get(), gdt_address + gdt_offset));
		}
	}

	/* Interrupt Descriptor Table */

	if(idt_size != 0)
	{
		for(uint32_t idt_offset = 0; idt_offset < idt_size; idt_offset += 8)
		{
			idt->descriptors.push_back(Descriptor::ReadEntry(*image.get(), idt_address + idt_offset));
		}
	}

	/* Local Descriptor Table */

	if(ldt_size != 0)
	{
		for(uint32_t ldt_offset = 0; ldt_offset < ldt_size; ldt_offset += 8)
		{
			ldt->descriptors.push_back(Descriptor::ReadEntry(*image.get(), ldt_address + ldt_offset));
		}
	}

	// combine SIT and descriptor table information

	for(auto segment : segments)
	{
		std::shared_ptr<SITEntry> entry = std::static_pointer_cast<SITEntry>(segment);
		uint16_t index = entry->selector >> 3;
		Descriptor * descriptor;
		if((entry->selector & 4) == 0)
		{
			if(index >= gdt->descriptors.size())
			{
				Linker::Error << "Error: segment information table entry overflow" << std::endl;
				continue;
			}
			descriptor = &gdt->descriptors[index];
		}
		else
		{
			if(index >= ldt->descriptors.size())
			{
				Linker::Error << "Error: segment information table entry overflow" << std::endl;
				continue;
			}
			descriptor = &ldt->descriptors[index];
		}
		if(descriptor->IsGate())
			continue; // invalid
		if(descriptor->image.use_count() != 0)
		{
			Linker::Error << "Error: multiple segment information table entries reference same segment" << std::endl;
			continue;
		}
		descriptor->image = segment;
	}
}

bool P3Format::External::FormatSupportsSegmentation() const
{
	return is_multisegmented;
}

bool P3Format::External::FormatIs16bit() const
{
	return !is_32bit;
}

bool P3Format::External::FormatIsProtectedMode() const
{
	return true;
}

void P3Format::External::SetOptions(std::map<std::string, std::string>& options)
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

std::string P3Format::External::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

void P3Format::External::ProcessModule(Linker::Module& module)
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

void P3Format::External::CalculateValues()
{
	// TODO
}

offset_t P3Format::External::WriteFile(Linker::Writer& wr) const
{
	// TODO
	return 0;
}

void P3Format::External::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	static const std::map<offset_t, std::string> level_description =
	{
		{ 1, "flat" },
		{ 2, "multisegmented" },
	};

	dump.SetTitle("P2/P3 format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.AddField("Signature", Dumper::StringDisplay::Make(2, "'"), std::string(is_32bit ? "P3" : "P2"));
	file_region.AddField("Word size", Dumper::ChoiceDisplay::Make("32-bit", "16-bit"), offset_t(is_32bit));
	file_region.AddField("Level", Dumper::ChoiceDisplay::Make(level_description, 4), offset_t(is_multisegmented ? 2 : 1));
	file_region.AddOptionalField("Checksum (16-bit)", Dumper::HexDisplay::Make(4), offset_t(checksum16));
	file_region.AddOptionalField("Checksum (32-bit)", Dumper::HexDisplay::Make(8), offset_t(checksum32));
	if(!is_multisegmented || minimum_extra != 0)
	{
		file_region.AddField("Minimum extra size", Dumper::HexDisplay::Make(8), offset_t(minimum_extra));
	}
	if(!is_multisegmented || maximum_extra != 0)
	{
		file_region.AddField("Maximum extra size", Dumper::HexDisplay::Make(8), offset_t(maximum_extra));
	}
	if(!is_multisegmented || base_load_offset != 0)
	{
		file_region.AddField("Base address", Dumper::HexDisplay::Make(8), offset_t(base_load_offset));
	}
	if(is_multisegmented || cs != 0)
	{
		file_region.AddField("Entry (CS:EIP)", Dumper::SegmentedDisplay::Make(8), offset_t(cs), offset_t(eip));
	}
	else
	{
		file_region.AddField("Entry (EIP)", Dumper::HexDisplay::Make(8), offset_t(eip));
	}
	if(is_multisegmented || ss != 0)
	{
		file_region.AddField("Initial stack (SS:ESP)", Dumper::SegmentedDisplay::Make(8), offset_t(ss), offset_t(esp));
	}
	else
	{
		file_region.AddField("Initial stack (ESP)", Dumper::HexDisplay::Make(8), offset_t(esp));
	}
	if(is_multisegmented || ldtr != 0)
	{
		file_region.AddField("Initial LDTR (local description table register)", Dumper::HexDisplay::Make(4), offset_t(ldtr));
	}
	if(is_multisegmented || tr != 0)
	{
		file_region.AddField("Initial TR (task state segment register)", Dumper::HexDisplay::Make(4), offset_t(tr));
	}
	file_region.AddOptionalField("Flags", Dumper::BitFieldDisplay::Make(4)
		->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("compressed"), true)
		->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("32-bit checksum"), true)
		->AddBitField(2, 3, "relocation table type", Dumper::DecDisplay::Make(), true),
		offset_t(flags));
	file_region.AddField("Size of initial stack", Dumper::HexDisplay::Make(4), offset_t(stack_size));
	file_region.Display(dump);

	Dumper::Region header_region("Header", file_offset, header_size, 8);
	if(is_multisegmented || gdt_address != 0 || gdt_size != 0)
	{
		header_region.AddField("GDT address", Dumper::HexDisplay::Make(8), offset_t(gdt_address));
		header_region.AddField("GDT size", Dumper::HexDisplay::Make(8), offset_t(gdt_size));
	}
	if(is_multisegmented || ldt_address != 0 || ldt_size != 0)
	{
		header_region.AddField("LDT address", Dumper::HexDisplay::Make(8), offset_t(ldt_address));
		header_region.AddField("LDT size", Dumper::HexDisplay::Make(8), offset_t(ldt_size));
	}
	if(is_multisegmented || idt_address != 0 || idt_size != 0)
	{
		header_region.AddField("IDT address", Dumper::HexDisplay::Make(8), offset_t(idt_address));
		header_region.AddField("IDT size", Dumper::HexDisplay::Make(8), offset_t(idt_size));
	}
	if(is_multisegmented || tss_address != 0 || tss_size != 0)
	{
		header_region.AddField("TSS address", Dumper::HexDisplay::Make(8), offset_t(tss_address));
		header_region.AddField("TSS size", Dumper::HexDisplay::Make(8), offset_t(tss_size));
	}
	header_region.Display(dump);

	if(segment_information_table_offset != 0 || segment_information_table_size != 0)
	{
		Dumper::Region segment_information_table_region("Segment information table", file_offset + segment_information_table_offset, segment_information_table_size, 8);
		segment_information_table_region.AddField("Entry size", Dumper::HexDisplay::Make(4), offset_t(segment_information_table_entry_size));
		segment_information_table_region.Display(dump);

		uint32_t segment_index = 0;
		for(auto segment : segments)
		{
			if(auto sit_entry = std::dynamic_pointer_cast<SITEntry>(segment))
			{
				Dumper::Entry segment_entry("Segment", segment_index + 1, file_offset + segment_information_table_offset + segment_index * segment_information_table_entry_size, 8);
				segment_entry.AddField("Selector", Dumper::HexDisplay::Make(4), offset_t(sit_entry->selector));
				segment_entry.AddOptionalField("Flags", Dumper::HexDisplay::Make(4), offset_t(sit_entry->flags));
				segment_entry.AddOptionalField("Base offset", Dumper::HexDisplay::Make(8), offset_t(sit_entry->base_offset));
				segment_entry.AddOptionalField("Extra bytes", Dumper::HexDisplay::Make(8), offset_t(sit_entry->zero_fill));
				segment_entry.Display(dump);
				segment_index++;
			}
		}
	}

	if(relocation_table_offset != 0 || relocation_table_size != 0)
	{
		Dumper::Region relocation_table_region("Relocation table", file_offset + relocation_table_offset, relocation_table_size, 8);
		relocation_table_region.Display(dump);

		uint32_t relocation_index = 0;
		for(auto relocation : relocations)
		{
			Dumper::Entry relocation_entry("Relocation", relocation_index + 1, file_offset + relocation_table_offset + relocation_index * (is_32bit ? 6 : 4), 8);
			relocation_entry.AddField("Address", Dumper::SegmentedDisplay::Make(is_32bit ? 8 : 4), offset_t(relocation.selector), offset_t(relocation.offset));
			relocation_entry.Display(dump);
			relocation_index++;
		}
	}

	if(runtime_parameters_offset != 0 || runtime_parameters_size != 0)
	{
		Dumper::Region runtime_parameters_region("Runtime parameters", file_offset + runtime_parameters_offset, runtime_parameters_size, 8);
		runtime_parameters_region.Display(dump);
	}

	if(symbol_table_offset != 0 || symbol_table_size != 0)
	{
		Dumper::Region symbol_table_region("Symbol table", file_offset + symbol_table_offset, symbol_table_size, 8);
		// TODO
		symbol_table_region.Display(dump);
	}

	Dumper::Block load_image_block("Load image", file_offset + load_image_offset, image, is_multisegmented ? 0 : base_load_offset, 8);
	load_image_block.AddOptionalField("Size in memory", Dumper::HexDisplay::Make(4), offset_t(memory_requirements));
	for(auto relocation : relocations)
	{
		uint16_t index = relocation.selector >> 3;
		Descriptor * descriptor;
		if((relocation.selector & 4) == 0)
		{
			if(index >= gdt->descriptors.size())
				continue; // invalid
			descriptor = &gdt->descriptors[index];
		}
		else
		{
			if(index >= ldt->descriptors.size())
				continue; // invalid
			descriptor = &ldt->descriptors[index];
		}
		if(descriptor->IsGate())
			continue; // invalid
		load_image_block.AddSignal(descriptor->base + relocation.offset, 2);
	}
	load_image_block.Display(dump);

	/* Task State Segment */

	if(tss_address != 0 || tss_size != 0)
	{
		Dumper::Region tss_region("Task State Segment (TSS)", file_offset + load_image_offset + tss_address, tss_size, 8);
		tss->FillEntries(tss_region);
		tss_region.Display(dump);
	}

	/* Global Descriptor Table */

	if(gdt_address != 0 || gdt_size != 0)
	{
		Dumper::Region gdt_region("Global Descriptor Table (GDT)", file_offset + load_image_offset + gdt_address, gdt_size, 8);
		gdt_region.Display(dump);
	}

	uint32_t descriptor_index = 0;
	for(auto& descriptor : gdt->descriptors)
	{
		Dumper::Entry gdt_entry("GDT entry", descriptor_index + 1, file_offset + load_image_offset + gdt_address + 8 * descriptor_index, 8);
		gdt_entry.AddField("Selector", Dumper::HexDisplay::Make(4), offset_t(descriptor_index * 8));
		descriptor.FillEntry(gdt_entry);
		gdt_entry.Display(dump);
		descriptor_index ++;
	}

	/* Interrupt Descriptor Table */

	if(idt_address != 0 || idt_size != 0)
	{
		Dumper::Region idt_region("Interrupt Descriptor Table (IDT)", file_offset + load_image_offset + idt_address, idt_size, 8);
		idt_region.Display(dump);
	}

	descriptor_index = 0;
	for(auto& descriptor : idt->descriptors)
	{
		Dumper::Entry idt_entry("IDT entry", descriptor_index + 1, file_offset + load_image_offset + idt_address + 8 * descriptor_index, 8);
		idt_entry.AddField("Interrupt number", Dumper::HexDisplay::Make(2), offset_t(descriptor_index));
		descriptor.FillEntry(idt_entry);
		idt_entry.Display(dump);
		descriptor_index ++;
	}

	/* Local Descriptor Table */

	if(ldt_address != 0 || ldt_size != 0)
	{
		Dumper::Region ldt_region("Local Descriptor Table (LDT)", file_offset + load_image_offset + ldt_address, ldt_size, 8);
		ldt_region.Display(dump);
	}

	descriptor_index = 0;
	for(auto& descriptor : ldt->descriptors)
	{
		Dumper::Entry ldt_entry("LDT entry", descriptor_index + 1, file_offset + load_image_offset + ldt_address + 8 * descriptor_index, 8);
		ldt_entry.AddField("Selector", Dumper::HexDisplay::Make(4), offset_t(descriptor_index * 8 + 4));
		descriptor.FillEntry(ldt_entry);
		ldt_entry.Display(dump);
		descriptor_index ++;
	}
}

