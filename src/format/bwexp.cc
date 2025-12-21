
#include <array>
#include "bwexp.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace DOS16M;

void BWFormat::ReadFile(Linker::Reader& rd)
{
	std::array<char, 2> signature;
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	rd.ReadData(signature);
	if(signature[0] != 'B' || signature[1] != 'W')
	{
		// TODO: try to find real file offset
	}
	file_size = rd.ReadUnsigned(2);
	file_size += uint32_t(rd.ReadUnsigned(2)) << 9;
	rd.Skip(4); // reserved
	min_extra = uint32_t(rd.ReadUnsigned(2)) << 10;
	max_extra = uint32_t(rd.ReadUnsigned(2)) << 10;
	ss = rd.ReadUnsigned(2);
	sp = rd.ReadUnsigned(2);
	relocsel = rd.ReadUnsigned(2);
	ip = rd.ReadUnsigned(2);
	cs = rd.ReadUnsigned(2);
	runtime_gdt_length = rd.ReadUnsigned(2);
	version = rd.ReadUnsigned(2);
	next_header_offset = rd.ReadUnsigned(4); // TODO: read sequence of spliced images
	debug_info_offset = rd.ReadUnsigned(4);
	last_used_selector = rd.ReadUnsigned(2);
	private_xm = uint32_t(rd.ReadUnsigned(2)) << 10;
	ext_reserve = rd.ReadUnsigned(2);
	rd.Skip(6); // reserved
	options = option_type(rd.ReadUnsigned(2));
	transparent_stack = rd.ReadUnsigned(2);
	exp_flags = exp_flag_type(rd.ReadUnsigned(2));
	program_size = uint32_t(rd.ReadUnsigned(2)) << 4;
	gdt_size = rd.ReadUnsigned(2);
	first_selector = rd.ReadUnsigned(2);
	default_memory_strategy = rd.ReadUnsigned(1);
	rd.Skip(1); // reserved
	transfer_buffer_size = rd.ReadUnsigned(2);
	rd.Skip(48); // TODO
	exp_name = rd.ReadData(48); // TODO: trim

	uint16_t first_relocation_selector;
	if((options & OPTION_RELOCATIONS) == 0)
	{
		option_relocations = RelocationsNone;
		first_relocation_selector = uint16_t(-1);
	}
	else if(relocsel == 0)
	{
		option_relocations = RelocationsType1;
		first_relocation_selector = (last_used_selector & ~7) - 8;
	}
	else
	{
		option_relocations = RelocationsType2;
		first_relocation_selector = relocsel & ~7;
	}

	rd.Seek(file_offset + 0xB0);
	segments.clear();

	for(uint16_t selector = first_selector ? first_selector & ~7 : 0x80; selector <= (last_used_selector & ~7); selector += 8)
	{
		std::unique_ptr<AbstractSegment> segment;
		if(selector >= first_relocation_selector)
		{
			segment = std::make_unique<RelocationSegment>();
		}
		else
		{
			segment = std::make_unique<Segment>();
		}
		segment->ReadHeader(rd);
		segments.emplace_back(std::move(segment));
	}
	rd.Seek(file_offset + 48 + gdt_size + 1);
	for(auto& segment : segments)
	{
		rd.Seek(::AlignTo(rd.Tell(), 0x10));
		segment->ReadContent(rd, *this);
	}
}

bool BWFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool BWFormat::FormatIs16bit() const
{
	return true;
}

bool BWFormat::FormatIsProtectedMode() const
{
	return true;
}

unsigned BWFormat::FormatAdditionalSectionFlags(std::string section_name) const
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

BWFormat::AbstractSegment::~AbstractSegment()
{
}

uint32_t BWFormat::AbstractSegment::GetTotalSize()
{
	return total_length;
}

void BWFormat::AbstractSegment::SetTotalSize(uint32_t new_value)
{
	if(new_value > 0x10000)
	{
		new_value = 0x10000;
	}
	if(new_value > total_length)
	{
		total_length = new_value;
	}
}

void BWFormat::AbstractSegment::Prepare(BWFormat& bw)
{
	uint32_t size = GetSize(bw);
	if(size == 0)
		flags = flag_type(flags | FLAG_EMPTY);
	else
		flags = flag_type(flags & ~FLAG_EMPTY);
}

void BWFormat::AbstractSegment::ReadHeader(Linker::Reader& rd)
{
	size = rd.ReadUnsigned(2);
	if(size != 0)
		size += 1;
	address = rd.ReadUnsigned(3);
	access = access_type(rd.ReadUnsigned(1));
	total_length = rd.ReadUnsigned(2);
	flags = flag_type(total_length & 0xF000);
	total_length <<= 4;
}

void BWFormat::AbstractSegment::WriteHeader(Linker::Writer& wr, const BWFormat& bw) const
{
	uint32_t size = GetSize(bw);
	wr.WriteWord(2, size > 0 ? size - 1 : 0);
	wr.WriteWord(3, address);
	wr.WriteWord(1, access);
	wr.WriteWord(2, (::AlignTo(total_length, 0x10) >> 4) | flags);
}

void BWFormat::Segment::SetTotalSize(uint32_t new_value)
{
	AbstractSegment::SetTotalSize(new_value);
	std::dynamic_pointer_cast<Linker::Segment>(image)->SetEndAddress(GetTotalSize());
}

uint32_t BWFormat::Segment::GetSize(const BWFormat& bw) const
{
	return image ? image->ImageSize() : 0;
}

void BWFormat::Segment::ReadContent(Linker::Reader& rd, BWFormat& bw)
{
	if(size != 0)
	{
		image = Linker::Buffer::ReadFromFile(rd, size);
	}
}

void BWFormat::Segment::WriteContent(Linker::Writer& wr, const BWFormat& bw) const
{
	if(image)
	{
		image->WriteFile(wr);
	}
}

void BWFormat::Segment::Dump(Dumper::Dumper& dump, const BWFormat& bw, offset_t file_offset, uint16_t selector_offset) const
{
	Dumper::Block segment_block("Segment", file_offset, image, address, 6);
	segment_block.InsertField(0, "Selector", Dumper::HexDisplay::Make(4), offset_t(selector_offset));
	segment_block.AddField("Memory size", Dumper::HexDisplay::Make(6), offset_t(total_length + 1));
	segment_block.AddField("Access", Dumper::HexDisplay::Make(2), offset_t(access)); // TODO: bitfield
	segment_block.AddOptionalField("Flags", Dumper::HexDisplay::Make(1), offset_t(flags >> 12)); // TODO: bitfield
	// TODO: relocation signals
	segment_block.Display(dump);
}

void BWFormat::DummySegment::SetTotalSize(uint32_t new_value)
{
	AbstractSegment::SetTotalSize(new_value);
}

uint32_t BWFormat::DummySegment::GetSize(const BWFormat& bw) const
{
	return 0;
}

void BWFormat::DummySegment::ReadContent(Linker::Reader& rd, BWFormat& bw)
{
}

void BWFormat::DummySegment::WriteContent(Linker::Writer& wr, const BWFormat& bw) const
{
}

void BWFormat::DummySegment::Dump(Dumper::Dumper& dump, const BWFormat& bw, offset_t file_offset, uint16_t selector_offset) const
{
	// TODO
}

void BWFormat::RelocationSegment::SetTotalSize(uint32_t new_value)
{
	Linker::FatalError("Internal error: relocation segment's size is predetermined");
}

uint32_t BWFormat::RelocationSegment::GetSize(const BWFormat& bw) const
{
	switch(bw.option_relocations)
	{
	case RelocationsType1:
		return bw.MeasureRelocations();
	case RelocationsType2:
		{
			offset_t size = bw.MeasureRelocations();
			if(size == 0)
				return 4; /* create dummy relocation segment */
			else if(offset_t((index + 1) * 0x10000) < size)
				return 0x10000;
			else
				return size - index * 0x10000;
		}
	default:
		Linker::FatalError("Internal error: Impossible relocation mode");
	}
}

void BWFormat::RelocationSegment::ReadContent(Linker::Reader& rd, BWFormat& bw)
{
	// TODO
}

void BWFormat::RelocationSegment::WriteContent(Linker::Writer& wr, const BWFormat& bw) const
{
	switch(bw.option_relocations)
	{
	case RelocationsType1:
		if(index == 0)
		{
			/* write both segments */
			for(auto& it : bw.relocations)
			{
				for(unsigned i = 0; i < it.second.size(); i++)
				{
					wr.WriteWord(2, it.first);
				}
			}
			wr.AlignTo(0x10);
			for(auto& it : bw.relocations)
			{
				for(auto& it2 : it.second)
				{
					wr.WriteWord(2, it2);
				}
			}
		}
		break;
	case RelocationsType2:
		if(index == 0)
		{
			/* write all segments */
			size_t count = 0;
			if(bw.relocations.empty())
			{
				/* if no relocations are present, create dummy relocation sequence so file appears relocatable */
				wr.WriteWord(2, 0x0002); /* end of relocations */
				wr.WriteWord(2, 0x0000); /* 0 relocations */
			}
			for(auto& it : bw.relocations)
			{
				if(count == bw.relocations.size() - 1)
					wr.WriteWord(2, it.first | 2);
				else
					wr.WriteWord(2, it.first);
				wr.WriteWord(2, it.second.size());
				for(auto& it2 : it.second)
				{
					wr.WriteWord(2, it2);
				}
				count++;
			}
		}
		break;
	default:
		Linker::FatalError("Internal error: Impossible relocation mode");
	}
}

offset_t BWFormat::MeasureRelocations() const
{
	switch(option_relocations)
	{
	case RelocationsNone:
		return 0;
	case RelocationsType1:
		{
			offset_t count = 0;
			for(auto& it : relocations)
			{
				count += 2 * it.second.size();
			}
			return count;
		}
	case RelocationsType2:
		{
			offset_t count = 0;
			for(auto& it : relocations)
			{
				count += 4 + 2 * it.second.size();
			}
			return count;
		}
	default:
		Linker::FatalError("Internal error: Impossible relocation type");
	}
}

void BWFormat::RelocationSegment::Dump(Dumper::Dumper& dump, const BWFormat& bw, offset_t file_offset, uint16_t selector_offset) const
{
	// TODO
}

std::shared_ptr<Linker::OptionCollector> BWFormat::GetOptions()
{
	return std::make_shared<BWOptionCollector>();
}

void BWFormat::SetOptions(std::map<std::string, std::string>& options)
{
	BWOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
}

void BWFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	if(segment->name == ".data")
		default_data = segments.size();

	std::unique_ptr<Segment> bw_segment = std::make_unique<Segment>(segment, segment->sections[0]->IsExecutable() ? Segment::TYPE_CODE : Segment::TYPE_DATA);
	segment_indices[segment] = segments.size();
	segments.push_back(std::move(bw_segment));
}

std::unique_ptr<Script::List> BWFormat::GetScript(Linker::Module& module)
{
	static const char * SegmentedScript = R"(
".code"
{
	base here;
	all exec;
	align 16;
};

".data"
{
	at 0;
	base here;
	all ".data" or ".rodata";
	align 16;
	all ".bss" or ".comm";
	align 16;
	all ".stack";
	align 16;
};

for any
{
	at 0;
	base here;
	all not zero;
	all zero;
	align 16;
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

void BWFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

size_t BWFormat::GetDefaultDataIndex()
{
	if(default_data == -1)
	{
		std::unique_ptr<DummySegment> data = std::make_unique<DummySegment>(0, Segment::TYPE_DATA);
		default_data = segments.size();
		segments.push_back(std::move(data));
	}
	return default_data;
}

void BWFormat::ProcessModule(Linker::Module& module)
{
	default_data = -1;
	Link(module);

	//first_selector = 0x80; /* TODO: make dynamic */

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		ss = first_selector + segment_indices[position.segment] * 8;
		sp = position.address;
	}
	else
	{
		Linker::Warning << "Warning: no stack found, creating one" << std::endl;
		ss = first_selector + GetDefaultDataIndex() * 8;
		std::unique_ptr<AbstractSegment>& segment = segments[GetDefaultDataIndex()];
		segment->SetTotalSize(segment->GetTotalSize() + 0x1000);
		sp = segment->GetTotalSize();
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		cs = first_selector + segment_indices[position.segment] * 8;
		ip = position.address;
	}
	else
	{
		Linker::Warning << "Warning: no entry found, using 0:0" << std::endl;
		cs = first_selector;
		ip = 0;
	}

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
			if(resolution.target != resolution.reference)
			{
				Linker::Error << "Error: intersegment differences impossible in protected mode, ignoring" << std::endl;
				continue;
			}
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
			uint16_t source_selector = first_selector + segment_indices[source.segment] * 8;
			uint16_t target_selector = first_selector + segment_indices[resolution.target] * 8;
			rel.WriteWord(target_selector);
			auto it = relocations.find(source_selector);
			if(it == relocations.end())
			{
				relocations[source_selector] = std::set<uint16_t>();
			}
			relocations[source_selector].insert(source.address);
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}
}

void BWFormat::CalculateValues()
{
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;
	//Linker::Debug << "Debug: New header offset: " << std::hex << file_offset << std::endl;

	min_extra = 0; /* TODO: parametrize */
	max_extra = 0x3FFFC00; /* TODO: parametrize */
	version = 2000;

	if(option_relocations != RelocationsNone)
	{
		relocsel = first_selector + segments.size() * 8;
		options = option_type(options | OPTION_RELOCATIONS);
		exp_flags = exp_flag_type(exp_flags | EXP_FLAG_RELOCATABLE);
		switch(option_relocations)
		{
		case RelocationsNone:
			break;
		case RelocationsType1:
			segments.push_back(std::make_unique<RelocationSegment>(0));
			segments.push_back(std::make_unique<RelocationSegment>(1));
			break;
		case RelocationsType2:
			for(unsigned i = 0;
				i < ((MeasureRelocations() + 0xFFFF) >> 16) || (option_force_relocations && i == 0);
				i++)
			{
				segments.push_back(std::make_unique<RelocationSegment>(i));
			}
			break;
		}
	}

	file_size = ::AlignTo(48 + first_selector + segments.size() * 8, 0x10);
	for(auto& segment : segments)
	{
		file_size = ::AlignTo(file_size + segment->GetSize(*this), 0x10);
	}

	for(auto& segment : segments)
	{
		segment->Prepare(*this);
	}

	last_used_selector = first_selector + (segments.size() - 1) * 8;
	gdt_size = ::AlignTo(first_selector + segments.size() * 8, 0x10) - 1;
}

offset_t BWFormat::ImageSize() const
{
	return file_size;
}

offset_t BWFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	if(stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}
	wr.Seek(file_offset);
	wr.WriteData(2, "BW");
	wr.WriteWord(2, file_size & 0x1FF);
	wr.WriteWord(2, file_size >> 9);
	wr.Skip(4);
	wr.WriteWord(2, (min_extra + 0x3FF) >> 10);
	wr.WriteWord(2, (max_extra + 0x3FF) >> 10);
	wr.WriteWord(2, ss);
	wr.WriteWord(2, sp);
	wr.WriteWord(2, relocsel);
	wr.WriteWord(2, ip);
	wr.WriteWord(2, cs);
	wr.WriteWord(2, runtime_gdt_length);
	wr.WriteWord(2, version);
	wr.WriteWord(4, next_header_offset);
	wr.WriteWord(4, debug_info_offset);
	wr.WriteWord(2, last_used_selector);
	wr.WriteWord(2, (private_xm + 0x3FF) >> 10);
	wr.WriteWord(2, ext_reserve);
	wr.Skip(6);
	wr.WriteWord(2, options);
	wr.WriteWord(2, transparent_stack);
	wr.WriteWord(2, exp_flags);
	wr.WriteWord(2, (program_size + 0xF) >> 4);
	wr.WriteWord(2, gdt_size);
	wr.WriteWord(2, first_selector);
	wr.WriteWord(1, default_memory_strategy);
	wr.Skip(1);
	wr.WriteWord(2, transfer_buffer_size);
	wr.Skip(48);
	wr.WriteData(48, exp_name);
	/* TODO */

	wr.Seek(file_offset + 48 + first_selector);
	for(auto& segment : segments)
	{
		segment->WriteHeader(wr, *this);
	}
	wr.AlignTo(0x10);
	for(auto& segment : segments)
	{
		segment->WriteContent(wr, *this);
		wr.AlignTo(0x10);
	}

	return ImageSize();
}

void BWFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("BW format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.AddField(".EXP file name", Dumper::StringDisplay::Make("'"), exp_name);
	file_region.AddOptionalField("Program size", Dumper::HexDisplay::Make(8), offset_t(program_size));
	file_region.AddField("Tool version", Dumper::DecDisplay::Make(), offset_t(version));
	file_region.AddOptionalField("Next header offset", Dumper::HexDisplay::Make(8), offset_t(next_header_offset));
	file_region.AddField("Minimum additional memory", Dumper::HexDisplay::Make(8), offset_t(min_extra));
	file_region.AddField("Maximum additional memory", Dumper::HexDisplay::Make(8), offset_t(max_extra));
	file_region.AddField("Private memory allocation", Dumper::HexDisplay::Make(8), offset_t(private_xm));
	file_region.AddField("Allocation increment", Dumper::HexDisplay::Make(8), offset_t(ext_reserve));
	file_region.AddField("Entry (CS:IP)", Dumper::SegmentedDisplay::Make(4), offset_t(cs), offset_t(ip));
	file_region.AddField("Initial stack (SS:SP)", Dumper::SegmentedDisplay::Make(4), offset_t(ss), offset_t(sp));
	file_region.AddOptionalField("First relocation selector", Dumper::HexDisplay::Make(4), offset_t(relocsel));
	file_region.AddOptionalField("Debug information offset", Dumper::HexDisplay::Make(8), offset_t(debug_info_offset));
	file_region.AddOptionalField("Runtime options", Dumper::BitFieldDisplay::Make(4)
		/* TODO */,
		offset_t(options));
	file_region.AddOptionalField("Module flags", Dumper::BitFieldDisplay::Make(4)
		/* TODO */,
		offset_t(exp_flags));
	file_region.AddOptionalField("Transparent stack selector", Dumper::HexDisplay::Make(4), offset_t(transparent_stack));
	file_region.AddOptionalField("Default memory strategy", Dumper::HexDisplay::Make(2), offset_t(default_memory_strategy)); // TODO: make choice
	file_region.AddField("Transfer buffer size (0x2000 if 0)", Dumper::HexDisplay::Make(4), offset_t(transfer_buffer_size));
	file_region.Display(dump);

	Dumper::Region gdt_region("GDT", file_offset + 48, gdt_size != 0 ? gdt_size + 1 : (last_used_selector + 8) & ~7, 8);
	gdt_region.AddField("File size", Dumper::HexDisplay::Make(4), offset_t(gdt_size));
	gdt_region.AddField("Runtime size", Dumper::HexDisplay::Make(4), offset_t(runtime_gdt_length));
	gdt_region.AddField("First selector", Dumper::HexDisplay::Make(4), offset_t(first_selector));
	gdt_region.AddField("Last used selector", Dumper::HexDisplay::Make(4), offset_t(last_used_selector));
	gdt_region.Display(dump);

	uint16_t selector_offset = first_selector & ~7;
	for(auto& segment : segments)
	{
		segment->Dump(dump, *this, 0 /* TODO: file offset */, selector_offset);
		selector_offset += 8;
	}
}

std::string BWFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
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

