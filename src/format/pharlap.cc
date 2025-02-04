
#include "pharlap.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace PharLap;

// MPFormat

void MPFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
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

void MPFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub.filename = FetchOption(options, "stub", "");
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
	Link(module);

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
			image->zero_fill += 0x1000; /* TODO: parametrize */
		}
		esp = image->TotalSize();
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

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}
		if(rel.segment_of)
		{
			Linker::Error << "Error: segment relocations impossible in flat memory model, ignoring" << std::endl;
			continue;
		}
		else
		{
			rel.WriteWord(resolution.value);
			if(resolution.target != nullptr && resolution.reference == nullptr)
			{
				Linker::Position source = rel.source.GetPosition();
				if(rel.size == 4)
					relocations.insert(Relocation(source.address, 1));
				else
					relocations.insert(Relocation(source.address, 0));
			}
		}
	}
}

void MPFormat::CalculateValues()
{
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;
	relocation_offset = 0x1E;
	header_size = ::AlignTo(relocation_offset + (has_relocations ? 4 * relocations.size() : 0), 0x10);
	image_size = header_size + image->data_size;
	bss_pages = (image->zero_fill + 0x3FFF) >> 12;
	extra_pages = (image->optional_extra + 0x3FFF) >> 12; /* TODO */
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
	wr.WriteWord(2, has_relocations ? relocations.size() : 0);
	wr.WriteWord(2, (header_size + 0xF) >> 4);
	wr.WriteWord(2, bss_pages);
	wr.WriteWord(2, bss_pages + extra_pages);
	wr.WriteWord(4, esp);
	wr.WriteWord(2, 0); /* TODO: checksum */
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
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
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
	/* TODO */
}

bool P3Format::FormatSupportsSegmentation() const
{
	return is_multisegmented; /* TODO: this probably does not actually support this? */
}

#if 0
bool P3Format::FormatSupportsStackSection() const
{
	return true;
}
#endif

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

void P3Format::SetOptions(std::map<std::string, std::string>& options)
{
	stub.filename = FetchOption(options, "stub", "");
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
	wr.WriteWord(2, 0); /* TODO: checksum */
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
	wr.WriteWord(4, 0); /* TODO: checksum */
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
	Link(module);

	minimum_extra = image->zero_fill;
	maximum_extra = image->optional_extra;
	if(maximum_extra == 0)
	{
		maximum_extra = -1;
	}

	if(std::shared_ptr<Linker::Section> stack_section = module.FindSection(".stack"))
	{
		stack_size = stack_section->Size();
	}
	else
	{
		stack_size = 0;
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		esp = stack_top.GetPosition().address;
	}
	else
	{
		Linker::Warning << "Warning: no stack found" << std::endl;
		if(stack_size == 0)
		{
			stack_size = 0x1000;
			minimum_extra += stack_size;
		}
		esp = image->TotalSize() + stack_size;
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

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}
		if(rel.segment_of)
		{
			Linker::Error << "Error: segment relocations impossible in flat memory model, ignoring" << std::endl;
			continue;
		}
		else
		{
			rel.WriteWord(resolution.value);
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
	load_image_size = image->data_size;
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

P3Format::MultiSegmented::AbstractSegment::~AbstractSegment()
{
}

void P3Format::MultiSegmented::Descriptor::CalculateValues()
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

void P3Format::MultiSegmented::Descriptor::WriteEntry(Linker::Writer& wr) const
{
	wr.WriteWord(2, limit & 0xFFFF);
	wr.WriteWord(3, base & 0xFFFFFF);
	wr.WriteWord(2, (access | (limit & 0xF0000)) >> 8);
	wr.WriteWord(1, base >> 24);
}

uint32_t P3Format::MultiSegmented::DescriptorTable::GetStoredSize() const
{
	return descriptors.size() * 8;
}

uint32_t P3Format::MultiSegmented::DescriptorTable::GetLoadedSize() const
{
	return descriptors.size() * 8;
}

void P3Format::MultiSegmented::DescriptorTable::WriteFile(Linker::Writer& wr) const
{
	for(auto& descriptor : descriptors)
	{
		descriptor.WriteEntry(wr);
	}
}

void P3Format::MultiSegmented::DescriptorTable::CalculateValues()
{
	for(auto& descriptor : descriptors)
	{
		descriptor.CalculateValues();
	}
}

uint32_t P3Format::MultiSegmented::TaskStateSegment::GetStoredSize() const
{
	return is_32bit ? 0x68 : 0x2C;
}

uint32_t P3Format::MultiSegmented::TaskStateSegment::GetLoadedSize() const
{
	return is_32bit ? 0x68 : 0x2C;
}

void P3Format::MultiSegmented::TaskStateSegment::WriteFile(Linker::Writer& wr) const
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

uint32_t P3Format::MultiSegmented::Segment::GetStoredSize() const
{
	return segment->data_size;
}

uint32_t P3Format::MultiSegmented::Segment::GetLoadedSize() const
{
	return segment->data_size + segment->zero_fill;
}

void P3Format::MultiSegmented::Segment::WriteSITEntry(Linker::Writer& wr) const
{
	wr.WriteWord(2, selector);
	wr.WriteWord(2, flags);
	wr.WriteWord(4, base_offset);
	wr.WriteWord(4, segment->zero_fill);
}

void P3Format::MultiSegmented::Segment::WriteFile(Linker::Writer& wr) const
{
	segment->WriteFile(wr);
}

bool P3Format::MultiSegmented::Relocation::operator ==(const Relocation& other) const
{
	return segment->selector == other.segment->selector && offset == other.offset;
}

bool P3Format::MultiSegmented::Relocation::operator <(const Relocation& other) const
{
	return segment->selector < other.segment->selector || (segment->selector == other.segment->selector && offset < other.offset);
}

void P3Format::MultiSegmented::Relocation::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, offset);
	wr.WriteWord(2, segment->selector);
}

void P3Format::MultiSegmented::OnNewSegment(std::shared_ptr<Linker::Segment> linker_segment)
{
	std::shared_ptr<Segment> segment = std::make_shared<Segment>(linker_segment,
		/* TODO: check properly read, write, 32-bit (TODO) flags */
		linker_segment->sections[0]->IsExecutable() ? Descriptor::Code32 : Descriptor::Data32,
		ldt->descriptors.size() * 8 + 4);

	segments.push_back(segment);
	segment_associations[linker_segment] = segment;
	ldt->descriptors.push_back(Descriptor(segment->access, segment));

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
	gdt = std::make_shared<DescriptorTable>();
	idt = std::make_shared<DescriptorTable>();
	ldt = std::make_shared<DescriptorTable>();
	tss = std::make_shared<TaskStateSegment>();

	gdt->descriptors.push_back(Descriptor(0));
	tr = gdt->descriptors.size() * 8;
	gdt->descriptors.push_back(Descriptor(Descriptor::TSS32, tss));
	gdt->descriptors.push_back(Descriptor(Descriptor::Data16, tss));
	gdt->descriptors.push_back(Descriptor(Descriptor::Data16, gdt));
	gdt->descriptors.push_back(Descriptor(Descriptor::Data16, idt));
	tss->ldtr = ldtr = gdt->descriptors.size() * 8;
	gdt->descriptors.push_back(Descriptor(Descriptor::LDT, ldt));
	gdt->descriptors.push_back(Descriptor(Descriptor::Data16, ldt));

	idt->descriptors.push_back(Descriptor(0));

	ldt->descriptors.push_back(Descriptor(0));

	segments.push_back(tss);
	segments.push_back(gdt);
	segments.push_back(idt);
	segments.push_back(ldt);

	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void P3Format::MultiSegmented::ProcessModule(Linker::Module& module)
{
	Link(module);

	std::shared_ptr<Linker::Segment> stack_segment;
	if(std::shared_ptr<Linker::Section> stack_section = module.FindSection(".stack"))
	{
		stack_size = stack_section->Size();
		stack_segment = stack_section->segment.lock();
	}
	else
	{
		stack_size = 0;
		stack_segment = nullptr;
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		esp = position.address;
		ss = segment_associations[position.segment]->selector;
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
			std::shared_ptr<Segment> segment = segment_associations[stack_segment];
			esp = segment->GetLoadedSize();
			ss = segment->selector;
		}
		else if(data != nullptr)
		{
			Linker::Warning << "Warning: no stack found, using data segment" << std::endl;
			stack_size = 0x1000; /* TODO: parametrize */
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
		cs = segment_associations[position.segment]->selector;
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

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}
		if(rel.segment_of)
		{
			if(resolution.target == nullptr || resolution.reference != nullptr || resolution.value != 0)
			{
				Linker::Error << "Error: intersegment differences impossible in protected mode, ignoring" << std::endl;
				continue;
			}
			Linker::Position source = rel.source.GetPosition();
			relocations.insert(Relocation(segment_associations[source.segment], source.address));
		}
		else
		{
			rel.WriteWord(resolution.value);
		}
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

void P3FormatContainer::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	offset_t file_offset = rd.Tell();
	rd.Skip(2);
	uint16_t level = rd.ReadUnsigned(2);
	switch(level)
	{
	case 1:
		contents = std::make_unique<P3Format::Flat>();
		break;
	case 2:
		contents = std::make_unique<P3Format::MultiSegmented>();
		break;
	default:
		Linker::Error << "Error: invalid P2/P3 format level " << level << std::endl;
		contents = std::make_unique<P3Format::MultiSegmented>();
		break;
	}
	rd.Seek(file_offset);
	contents->ReadFile(rd);
}

bool P3FormatContainer::FormatSupportsSegmentation() const
{
	return contents ? contents->FormatSupportsSegmentation() : true;
}

void P3FormatContainer::SetOptions(std::map<std::string, std::string>& options)
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

std::string P3FormatContainer::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

void P3FormatContainer::ProcessModule(Linker::Module& module)
{
	Linker::FatalError("Internal error: P2/P3 file level not provided");
}

void P3FormatContainer::CalculateValues()
{
	if(contents != nullptr)
	{
		contents->CalculateValues();
	}
}

offset_t P3FormatContainer::WriteFile(Linker::Writer& wr) const
{
	if(contents != nullptr)
	{
		return contents->WriteFile(wr);
	}
	else
	{
		return 0;
	}
}

void P3FormatContainer::Dump(Dumper::Dumper& dump) const
{
	if(contents != nullptr)
	{
		contents->Dump(dump);
	}
	else
	{
		Linker::Error << "Internal error: empty P3 container" << std::endl;
	}
}

