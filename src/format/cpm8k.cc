
#include "cpm8k.h"
#include "../linker/buffer.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"

using namespace DigitalResearch;

void CPM8KFormat::Segment::Clear()
{
	image = nullptr;
}

bool CPM8KFormat::Segment::IsPresent() const
{
	return type != BSS && type != STACK;
}

CPM8KFormat::Relocation CPM8KFormat::Relocation::ReadFile(Linker::Reader& rd)
{
	Relocation relocation;
	relocation.segment = rd.ReadUnsigned(1);
	relocation.type = Relocation::relocation_type(rd.ReadUnsigned(1));
	relocation.offset = rd.ReadUnsigned(2);
	relocation.target = rd.ReadUnsigned(2);
	return relocation;
}

void CPM8KFormat::Relocation::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(1, segment);
	wr.WriteWord(1, type);
	wr.WriteWord(2, offset);
	wr.WriteWord(2, target);
}

CPM8KFormat::magic_type CPM8KFormat::GetSignature() const
{
	if((signature[0] & 0xFF) == 0xEE)
	{
		switch(signature[1] & 0xFF)
		{
		case MAGIC_SEGMENTED_OBJECT & 0xFF:
			return MAGIC_SEGMENTED_OBJECT;
		case MAGIC_SEGMENTED & 0xFF:
			return MAGIC_SEGMENTED;
		case MAGIC_NONSHARED_OBJECT & 0xFF:
			return MAGIC_NONSHARED_OBJECT;
		case MAGIC_NONSHARED & 0xFF:
			return MAGIC_NONSHARED;
		case MAGIC_SHARED_OBJECT & 0xFF:
			return MAGIC_SHARED_OBJECT;
		case MAGIC_SHARED & 0xFF:
			return MAGIC_SHARED;
		case MAGIC_SPLIT_OBJECT & 0xFF:
			return MAGIC_SPLIT_OBJECT;
		case MAGIC_SPLIT & 0xFF:
			return MAGIC_SPLIT;
		default:
			return magic_type(0); // invalid
		}
	}
	return magic_type(0); // invalid
}

void CPM8KFormat::SetSignature(magic_type magic)
{
	signature[0] = magic >> 8;
	signature[1] = magic & 0xFF;
}

void CPM8KFormat::Clear()
{
	/* format fields */
	for(auto& segment : segments)
		segment.Clear();
	segments.clear();
	relocations.clear();
	symbols.clear();
}

void CPM8KFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::BigEndian;
	rd.ReadData(2, signature);
	segment_count = rd.ReadUnsigned(2);
	total_size = rd.ReadUnsigned(4);
	relocation_size = rd.ReadUnsigned(4);
	symbol_table_size = rd.ReadUnsigned(4);

	for(size_t i = 0; i < segment_count; i++)
	{
		Segment segment;
		segment.number = rd.ReadUnsigned(1);
		segment.type = Segment::segment_type(rd.ReadUnsigned(1));
		segment.length = rd.ReadUnsigned(2);
		segments.push_back(segment);
	}

	for(auto& segment : segments)
	{
		if(!segment.IsPresent())
			continue;

		segment.image = Linker::Buffer::ReadFromFile(rd, segment.length);
	}

	rd.Seek(0x10 + 4 * segments.size() + total_size);

	for(uint32_t relocation_offset = 0; relocation_offset < relocation_size; relocation_offset += 6)
	{
		relocations.push_back(Relocation::ReadFile(rd));
	}

	// TODO: symbols
}

offset_t CPM8KFormat::ImageSize() const
{
	offset_t size = 16 + segments.size() * 4;

	for(auto& segment : segments)
	{
		if(segment.IsPresent())
		{
			size += segment.image->ImageSize();
		}
	}

	size += relocation_size;

	// TODO: symbols

	return size;
}

offset_t CPM8KFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData(2, signature);
	wr.WriteWord(2, segment_count);
	wr.WriteWord(4, total_size);
	wr.WriteWord(4, relocation_size);
	wr.WriteWord(4, symbol_table_size);

	for(auto& segment : segments)
	{
		wr.WriteWord(1, segment.number);
		wr.WriteWord(1, segment.type);
		wr.WriteWord(2, segment.length);
	}

	for(auto& segment : segments)
	{
		if(segment.IsPresent())
		{
			segment.image->WriteFile(wr);
		}
	}

	wr.Seek(0x10 + 4 * segments.size() + total_size);

	for(auto& relocation : relocations)
	{
		relocation.WriteFile(wr);
	}

	// TODO: symbols

	return ImageSize();
}

void CPM8KFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	static const std::map<offset_t, std::string> signature_description =
	{
		{ 0xEE00, "segmented, non-executable" },
		{ 0xEE01, "segmented, executable" },
		{ 0xEE02, "non-segmented, non-executable" },
		{ 0xEE03, "non-segmented, executable, non-shared instruction and data space" },
		{ 0xEE06, "non-segmented, non-executable, shared instruction and data space" },
		{ 0xEE07, "non-segmented, executable, shared instruction and data space" },
		{ 0xEE0A, "non-segmented, non-executable, split instruction and data space" },
		{ 0xEE0B, "non-segmented, executable, split instruction and data space" },
	};

	dump.SetTitle("CP/M-8000 format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 6);
	file_region.AddField("Signature", Dumper::ChoiceDisplay::Make(signature_description, Dumper::HexDisplay::Make(4)),
		offset_t((uint8_t(signature[0]) << 8) | uint8_t(signature[1])));
	file_region.AddField("Segment count", Dumper::DecDisplay::Make(), offset_t(segment_count));
	file_region.AddField("Total size", Dumper::HexDisplay::Make(8), offset_t(total_size));
	file_region.Display(dump);

	static const std::map<offset_t, std::string> type_descriptions =
	{
		{ Segment::BSS, "BSS (uninitialized data)" },
		{ Segment::STACK, "Stack (not present)" },
		{ Segment::CODE, "Code/text (executable)" },
		{ Segment::RODATA, "Constant pool (read-only data)" },
		{ Segment::DATA, "Initialized data" },
		{ Segment::MIXED, "Mixed code and data, not write protectable" },
		{ Segment::MIXED_PROTECTABLE, "Mixed code and data, write protectable" },
	};

	uint16_t segment_index = 0;
	uint32_t segment_offset = file_offset + 0x40;
	for(auto& segment : segments)
	{
		if(segment.IsPresent())
		{
			Dumper::Block segment_block("Segment", segment_offset, segment.image->AsImage(), 0 /* TODO */, 8);
			segment_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(segment_index + 1));
			segment_block.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(2)), offset_t(segment.type));

			for(auto& relocation : relocations)
			{
				if(relocation.segment != segment_index)
					continue;
				segment_block.AddSignal(relocation.offset, relocation.GetRelocationSize());
			}

			segment_block.Display(dump);
			segment_offset += segment.length;
		}
		else
		{
			Dumper::Region segment_region("Segment", segment_offset, segment.length, 8);
			segment_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(segment_index + 1));
			segment_region.AddField("Address", Dumper::HexDisplay::Make(8), offset_t(0 /* TODO */));
			segment_region.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions, Dumper::HexDisplay::Make(2)), offset_t(segment.type));
			segment_region.Display(dump);
		}
		segment_index ++;
	}

	if(relocation_size != 0)
	{
		Dumper::Region relocation_region("Relocations", file_offset + 0x40 + 4 * segments.size() + total_size, relocation_size, 8);
		relocation_region.Display(dump);

		static const std::map<offset_t, std::string> relocation_descriptions =
		{
			{ Relocation::SEG_OFFSET, "Offset to segment" },
			{ Relocation::SEG_SHORT_SEGMENTED, "16-bit segmented address to segment" },
			{ Relocation::SEG_LONG_SEGMENTED, "32-bit segmented address to segment" },
			{ Relocation::EXT_OFFSET, "Offset to symbol" },
			{ Relocation::EXT_SHORT_SEGMENTED, "16-bit segmented address to symbol" },
			{ Relocation::EXT_LONG_SEGMENTED, "32-bit segmented address to symbol" },
		};

		uint32_t relocation_index = 0;
		for(auto& relocation : relocations)
		{
			Dumper::Entry relocation_entry("Relocation", relocation_index + 1, file_offset + 0x40 + 4 * segments.size() + total_size + 6 * relocation_index, 8);
			if(relocation.segment < segments.size())
				relocation_entry.AddField("Source segment type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(segments[relocation.segment].type));
			relocation_entry.AddField("Source", Dumper::SegmentedDisplay::Make(4), offset_t(relocation.segment), offset_t(relocation.offset));
			relocation_entry.AddField("Relocation type", Dumper::ChoiceDisplay::Make(relocation_descriptions, Dumper::HexDisplay::Make(2)), offset_t(relocation.type));
			relocation_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation.GetRelocationSize()));
			if(relocation.IsExternal())
			{
				relocation_entry.AddField("Symbol number", Dumper::HexDisplay::Make(4), offset_t(relocation.target));
			}
			else
			{
				relocation_entry.AddField("Segment number", Dumper::HexDisplay::Make(4), offset_t(relocation.target));
				if(relocation.target < segments.size())
					relocation_entry.AddField("Segment type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(segments[relocation.target].type));
			}
			relocation_entry.Display(dump);
			relocation_index++;
		}
	}

	if(symbol_table_size != 0)
	{
		Dumper::Region symbol_table_region("Symbol table", file_offset + 0x40 + 4 * segments.size() + total_size + relocation_size, symbol_table_size, 8);
		// TODO
		symbol_table_region.Display(dump);
	}
}

void CPM8KFormat::CalculateValues()
{
	segment_count = segments.size();
	total_size = 0;
	for(auto& segment : segments)
	{
		if(segment.IsPresent())
		{
			segment.length = segment.image->ImageSize();
			total_size += segment.length;
		}
	}
	relocation_size = 6 * relocations.size();
	symbol_table_size = 12 * symbols.size();
}

/* * * Writer members * * */

bool CPM8KFormat::FormatSupportsSegmentation() const
{
	magic_type magic = GetSignature();
	return magic == MAGIC_SEGMENTED || magic == MAGIC_SEGMENTED_OBJECT;
}

std::vector<std::shared_ptr<Linker::Segment>>& CPM8KFormat::Segments()
{
	return segment_vector;
}

unsigned CPM8KFormat::GetSegmentNumber(std::shared_ptr<Linker::Segment>segment)
{
	for(size_t i = 0; i < segments.size(); i++)
	{
		if(segments[i].image == std::dynamic_pointer_cast<Linker::Contents>(segment))
			return segments[i].number;
	}
	return unsigned(-1);
}

void CPM8KFormat::SetOptions(std::map<std::string, std::string>& options)
{
}

void CPM8KFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	Segment cpm_segment;
	if(segment->name == ".code")
	{
		cpm_segment.type = Segment::CODE;
		if(FormatSupportsSegmentation())
			cpm_segment.number = 0x09; // TODO: parametrize
	}
	else if(segment->name == ".rodata")
	{
		if(segment->IsMissing())
			return;
		cpm_segment.type = Segment::RODATA;
		if(FormatSupportsSegmentation())
			cpm_segment.number = 0x0a; // TODO: parametrize
	}
	else if(segment->name == ".data")
	{
		if(segment->IsMissing())
			return;
		cpm_segment.type = Segment::DATA;
		if(FormatSupportsSegmentation())
			cpm_segment.number = 0x0a; // TODO: parametrize
	}
	else if(segment->name == ".bss")
	{
		if(segment->IsMissing())
			return;
		cpm_segment.type = Segment::BSS;
		if(FormatSupportsSegmentation())
			cpm_segment.number = 0x0a; // TODO: parametrize
	}
//			else if(segment->name == ".stack")
//			{
//				if(segment->IsMissing())
//					return;
//				cpm_segment.type = Segment::STACK;
//				if(FormatSupportsSegmentation())
//					cpm_segment.number = 2;
//			}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected one of `.code`, `.rodata', `.data`, `.bss`, ignoring" << std::endl;
		return;
	}

	if(!FormatSupportsSegmentation())
		cpm_segment.number = segments.size();
	cpm_segment.length = segment->TotalSize();
	cpm_segment.image = segment;
	segments.push_back(cpm_segment);
}

bool CPM8KFormat::IsCombined() const
{
	magic_type magic = GetSignature();
	return magic == MAGIC_NONSHARED || magic == MAGIC_NONSHARED_OBJECT;
}

std::unique_ptr<Script::List> CPM8KFormat::GetScript(Linker::Module& module)
{
	static const char * CombinedScript = R"(
".code"
{
	all exec align 2;
	align 2;
};

".rodata"
{
	base 0;
	all not write align 2;
	align 2;
};

".data"
{
	base 0;
	all not zero align 2;
	align 2;
};

".bss"
{
	base 0;
	all not ".stack" align 2;
	align 2;
};
)";

#if 0
".stack"
{
	base 0;
	all align 2;
	align 2;
};
#endif

	static const char * SplitScript = R"(
".code"
{
	all exec align 2;
	align 2;
};

".rodata"
{
	at 0;
	all not write align 2;
	align 2;
};

".data"
{
	base 0;
	all not zero align 2;
	align 2;
};

".bss"
{
	base 0;
	all not ".stack" align 2;
	align 2;
};
)";

#if 0
".stack"
{
	base 0;
	all align 2;
	align 2;
};
#endif

	static const char * SegmentedScript = R"(
".code"
{
	all exec align 2;
	align 2;
};

".rodata"
{
	at 0;
	all not write align 2;
	align 2;
};

".data"
{
	base 0;
	all not zero align 2;
	align 2;
};

".bss"
{
	base 0;
	all not ".stack" align 2;
	align 2;
};
)";

#if 0
".stack"
{
	at 0;
	all align 2;
	align 2;
};
#endif
	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(GetSignature())
		{
		case MAGIC_NONSHARED:
		case MAGIC_NONSHARED_OBJECT:
			return Script::parse_string(CombinedScript);

		case MAGIC_SPLIT_OBJECT:
		case MAGIC_SPLIT:
			return Script::parse_string(SplitScript);

		case MAGIC_SEGMENTED_OBJECT:
		case MAGIC_SEGMENTED:
			return Script::parse_string(SegmentedScript);

		default:
			Linker::FatalError("Fatal error: unknown output format");
		}
	}
}

void CPM8KFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);
	ProcessScript(script, module);
}

void CPM8KFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::SegmentIndex)
		{
			if(resolution.reference != nullptr)
			{
				Linker::Error << "Error: Invalid segment offset, ignoring reference" << std::endl;
			}
			unsigned number = GetSegmentNumber(resolution.target);
			if(number == unsigned(-1))
			{
				Linker::Error << "Error: Invalid segment, ignoring" << std::endl;
			}
			rel.WriteWord(number);
		}
		else if(rel.kind == Linker::Relocation::Direct)
		{
			rel.WriteWord(resolution.value);
		}
		else
		{
			rel.WriteWord(resolution.value); // TODO

			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}
	// TODO: generate relocations?

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		if(position.segment != Segments()[0] || position.address != 0)
		{
			// TODO: this just assumes the first segment is code, is this actually expected?
			Linker::Error << "Error: entry point must be beginning of .code segment, ignoring" << std::endl;
		}
	}
}

void CPM8KFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::Z8K)
	{
		Linker::FatalError("Fatal error: Format only supports Zilog Z8000 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string CPM8KFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".z8k";
}

