
#include "bwexp.h"

using namespace DOS16M;

void BWFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

bool BWFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool BWFormat::FormatIs16bit() const
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

void BWFormat::AbstractSegment::WriteHeader(Linker::Writer& wr, BWFormat& bw)
{
	uint32_t size = GetSize(bw);
	if(size == 0)
		flags = (flag_type)(flags | FLAG_EMPTY);
	else
		flags = (flag_type)(flags & ~FLAG_EMPTY);
	wr.WriteWord(2, size > 0 ? size - 1 : 0);
	wr.WriteWord(3, address);
	wr.WriteWord(1, access);
	wr.WriteWord(2, (::AlignTo(total_length, 0x10) >> 4) | flags);
}

void BWFormat::Segment::SetTotalSize(uint32_t new_value)
{
	AbstractSegment::SetTotalSize(new_value);
	image->SetEndAddress(GetTotalSize());
}

uint32_t BWFormat::Segment::GetSize(BWFormat& bw)
{
	return image ? image->data_size : 0;
}

void BWFormat::Segment::WriteContent(Linker::Writer& wr, BWFormat& bw)
{
	if(image)
	{
		image->WriteFile(wr);
	}
}

void BWFormat::DummySegment::SetTotalSize(uint32_t new_value)
{
	AbstractSegment::SetTotalSize(new_value);
}

uint32_t BWFormat::DummySegment::GetSize(BWFormat& bw)
{
	return 0;
}

void BWFormat::DummySegment::WriteContent(Linker::Writer& wr, BWFormat& bw)
{
}

void BWFormat::RelocationSegment::SetTotalSize(uint32_t new_value)
{
	Linker::FatalError("Internal error: relocation segment's size is predetermined");
}

uint32_t BWFormat::RelocationSegment::GetSize(BWFormat& bw)
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
			else if((offset_t)(index + 1) * 0x10000 < size)
				return 0x10000;
			else
				return size - index * 0x10000;
		}
	default:
		Linker::FatalError("Internal error: Impossible relocation mode");
	}
}

void BWFormat::RelocationSegment::WriteContent(Linker::Writer& wr, BWFormat& bw)
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

offset_t BWFormat::MeasureRelocations()
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

void BWFormat::SetOptions(std::map<std::string, std::string>& options)
{
	stub_file = FetchOption(options, "stub", "");
}

void BWFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	if(segment->name == ".data")
		default_data = segments.size();

	std::unique_ptr<Segment> bw_segment = std::make_unique<Segment>(segment, segment->sections[0]->IsExecable() ? Segment::TYPE_CODE : Segment::TYPE_DATA);
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
		return LinkerManager::GetScript(module);
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
			if(resolution.target != resolution.reference)
			{
				Linker::Error << "Error: intersegment differences impossible in protected mode, ignoring" << std::endl;
				continue;
			}
			rel.WriteWord(resolution.value);
		}
	}
}

void BWFormat::CalculateValues()
{
	file_offset = stub_file != "" ? GetStubImageSize() : 0;;
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
}

void BWFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	if(stub_file != "")
	{
		WriteStubImage(wr);
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
	wr.WriteWord(2, first_selector + (segments.size() - 1) * 8);
	wr.WriteWord(2, (private_xm + 0x3FF) >> 10);
	wr.WriteWord(2, ext_reserve);
	wr.Skip(6);
	wr.WriteWord(2, options);
	wr.WriteWord(2, transparent_stack);
	wr.WriteWord(2, exp_flags);
	wr.WriteWord(2, (program_size + 0xF) >> 4);
	wr.WriteWord(2, ::AlignTo(first_selector + segments.size() * 8, 0x10) - 1);
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
}

std::string BWFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	if(stub_file != "")
	{
		return filename + ".exe";
	}
	else
	{
		return filename + ".exp";
	}
}

