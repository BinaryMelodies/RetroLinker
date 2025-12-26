
#include "xpexp.h"
#include "../linker/buffer.h"
#include "../linker/location.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

/* TODO: untested */

using namespace Ergo;

/* * * General members * * */

void XPFormat::CalculateValues()
{
	file_offset = stub.filename != "" ? stub.GetStubImageSize() : 0;;

	if(ldt_offset < 0x68)
	{
		ldt_offset = 0x68;
	}

	offset_t minimum_image_offset = ldt_offset + 8 * ldt.size();
	if(image_offset < minimum_image_offset)
	{
		image_offset = minimum_image_offset;
	}
}

void XPFormat::Clear()
{
	file_offset = 0;
	ldt_offset = 0;
	image_offset = 0;
	relocation_offset = 0; // TODO: unsure
	relocation_count = 0; // TODO: unsure
	minimum_extent = 0;
	maximum_extent = 0;
	unknown_field = 0;
	gs = fs = ds = ss = cs = es = edi = esi = ebp = esp = ebx = edx = ecx = eax = eflags = eip = 0;
	ldt.clear();
	image = nullptr;
	/* * * Writer members * * */
	wordsize = Unknown;
	section_groups.clear();
}

void XPFormat::ReadFile(Linker::Reader& rd)
{
	Clear();
	rd.endiantype = ::LittleEndian;
	std::array<char, 4> signature;
	file_offset = Microsoft::FindActualSignature(rd, signature, "XP\1\0", "XP\2\0");
	ldt_offset = rd.ReadUnsigned(4);
	uint32_t ldt_count = rd.ReadUnsigned(4);
	image_offset = rd.ReadUnsigned(4);
	uint32_t image_size = rd.ReadUnsigned(4);
	relocation_offset = rd.ReadUnsigned(4);
	relocation_count = rd.ReadUnsigned(4);
	minimum_extent = rd.ReadUnsigned(4);
	maximum_extent = rd.ReadUnsigned(4);
	rd.Skip(4);
	gs = rd.ReadUnsigned(4);
	fs = rd.ReadUnsigned(4);
	ds = rd.ReadUnsigned(4);
	ss = rd.ReadUnsigned(4);
	cs = rd.ReadUnsigned(4);
	es = rd.ReadUnsigned(4);
	edi = rd.ReadUnsigned(4);
	esi = rd.ReadUnsigned(4);
	ebp = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	ebx = rd.ReadUnsigned(4);
	edx = rd.ReadUnsigned(4);
	ecx = rd.ReadUnsigned(4);
	eax = rd.ReadUnsigned(4);
	eflags = rd.ReadUnsigned(4);
	eip = rd.ReadUnsigned(4);

	rd.Seek(file_offset + ldt_offset);
	for(uint32_t i = 0; i < ldt_count; i++)
	{
		ldt.push_back(Segment::ReadFile(rd));
	}

	if((cs >> 3) < ldt.size())
	{
		wordsize = ldt[cs >> 3].flags & Segment::FLAG_32BIT ? DWord : Word;
	}

	rd.Seek(image_offset);
	image = Linker::Buffer::ReadFromFile(rd, image_size);

	// TODO: relocations, format unknown
}

offset_t XPFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	if(stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}
	wr.Seek(file_offset);
	wr.WriteData(4, "XP\1\0");
	wr.WriteWord(4, ldt_offset);
	wr.WriteWord(4, ldt.size());
	wr.WriteWord(4, image_offset);
	wr.WriteWord(4, image->ImageSize());
	wr.WriteWord(4, relocation_offset);
	wr.WriteWord(4, relocation_count);
	wr.WriteWord(4, minimum_extent);
	wr.WriteWord(4, maximum_extent);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, gs);
	wr.WriteWord(4, fs);
	wr.WriteWord(4, ds);
	wr.WriteWord(4, ss);
	wr.WriteWord(4, cs);
	wr.WriteWord(4, es);
	wr.WriteWord(4, edi);
	wr.WriteWord(4, esi);
	wr.WriteWord(4, ebp);
	wr.WriteWord(4, esp);
	wr.WriteWord(4, ebx);
	wr.WriteWord(4, edx);
	wr.WriteWord(4, ecx);
	wr.WriteWord(4, eax);
	wr.WriteWord(4, eflags);
	wr.WriteWord(4, eip);

	wr.Seek(file_offset + ldt_offset);
	for(auto segment : ldt)
	{
		segment.WriteFile(wr);
	}

	wr.Seek(image_offset);
	image->WriteFile(wr);

	/* TODO */

	return offset_t(-1);
}

offset_t XPFormat::ImageSize() const
{
	return std::max(
		offset_t(ldt_offset + 8 * ldt.size()),
		image_offset + image->ImageSize());
}

void XPFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("XP format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);

	static const std::map<offset_t, std::string> wordsize_description =
	{
		{ Unknown, "unable to determine" },
		{ Word, "16-bit, for OS/286" },
		{ DWord, "32-bit, for OS/386" },
	};
	file_region.AddField("CPU mode", Dumper::ChoiceDisplay::Make(wordsize_description), offset_t(wordsize));
	file_region.AddField("Minimum extents", Dumper::HexDisplay::Make(8), offset_t(minimum_extent)); // TODO: meaning and unit unknown
	file_region.AddField("Maximum extents", Dumper::HexDisplay::Make(8), offset_t(maximum_extent)); // TODO: meaning and unit unknown
	file_region.AddField("Entry (CS:EIP)", Dumper::SegmentedDisplay::Make(8), offset_t(cs), offset_t(eip));
	file_region.AddField("Stack (SS:ESP)", Dumper::SegmentedDisplay::Make(8), offset_t(ss), offset_t(esp));
	file_region.AddOptionalField("EFLAGS", Dumper::HexDisplay::Make(8), offset_t(eflags)); // TODO: flags
	file_region.AddOptionalField("EAX", Dumper::HexDisplay::Make(8), offset_t(eax));
	file_region.AddOptionalField("ECX", Dumper::HexDisplay::Make(8), offset_t(ecx)); // apparently also LDT size in bytes
	file_region.AddOptionalField("EDX", Dumper::HexDisplay::Make(8), offset_t(edx));
	file_region.AddOptionalField("EBX", Dumper::HexDisplay::Make(8), offset_t(ebx));
	file_region.AddOptionalField("EBP", Dumper::HexDisplay::Make(8), offset_t(ebp));
	file_region.AddOptionalField("ESI", Dumper::HexDisplay::Make(8), offset_t(esi));
	file_region.AddOptionalField("EDI", Dumper::HexDisplay::Make(8), offset_t(edi));
	file_region.AddOptionalField("DS", Dumper::HexDisplay::Make(4), offset_t(ds));
	file_region.AddOptionalField("FS", Dumper::HexDisplay::Make(4), offset_t(fs));
	file_region.AddOptionalField("GS", Dumper::HexDisplay::Make(4), offset_t(gs));
	file_region.Display(dump);

	Dumper::Region ldt_region("Local Descriptor Table", file_offset + ldt_offset, 8 * ldt.size(), 8);
	ldt_region.Display(dump);
	unsigned i = 0;
	for(auto& segment : ldt)
	{
		segment.Dump(dump, *this, i);
		i++;
	}

	Dumper::Block image_block("Image", file_offset + image_offset, image->AsImage(), 0, 8);
	image_block.Display(dump);

	Dumper::Region relocation_region("Relocations", file_offset + relocation_offset, relocation_count /* TODO: unit of relocation */, 8); // TODO: nothing is known about relocations aside from their position
	relocation_region.Display(dump);
}

XPFormat::Segment XPFormat::Segment::ReadFile(Linker::Reader& rd)
{
	Segment segment;
	segment.limit = rd.ReadUnsigned(2);
	segment.base = rd.ReadUnsigned(3);
	segment.access = rd.ReadUnsigned(1);
	segment.flags = rd.ReadUnsigned(1);
	segment.limit |= uint32_t(segment.flags & 0x0F) << 16;
	segment.flags &= ~0x0F;
	segment.base |= uint32_t(rd.ReadUnsigned(1)) << 24;
	return segment;
}

void XPFormat::Segment::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, limit & 0xFFFF);
	wr.WriteWord(3, base & 0xFFFFFF);
	wr.WriteWord(1, access);
	wr.WriteWord(1,
		((limit >> 16) & 0x0F)
		| (flags & 0xF0));
	wr.WriteWord(1, base >> 24);
}

void XPFormat::Segment::Dump(Dumper::Dumper& dump, const XPFormat& xp, unsigned index) const
{
	Dumper::Entry descriptor_entry("Descriptor", index + 1, xp.ldt_offset + index * 8);
	descriptor_entry.AddField("Selector", Dumper::HexDisplay::Make(4), offset_t(index * 8 + 7));
	descriptor_entry.AddField("Base", Dumper::HexDisplay::Make(8), offset_t(base));
	descriptor_entry.AddField((flags & FLAG_PAGES) != 0 ? "Limit (in 4096 byte pages)" : "Limit (in bytes)", Dumper::HexDisplay::Make(5), offset_t(limit));
	if((access & ACCESS_S) != 0)
	{
		descriptor_entry.AddField("Access bits",
			Dumper::BitFieldDisplay::Make(2)
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("accessed"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make(
					(access & ACCESS_X) != 0 ? "readable" : "writable",
					(access & ACCESS_X) != 0 ? "execute-only" : "read-only"), false)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make((access & ACCESS_X) != 0 ? "conforming" : "grow down"), true)
					// conforming: can be jumped to from lower privilege level
				->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("code", "data"), false)
				->AddBitField(5, 2, Dumper::DecDisplay::Make(), "privilege level")
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("present"), false),
			offset_t(access));
	}
	else
	{
		// note: these are included for completeness sake, it is unlikely they would be present
		static const std::map<offset_t, std::string> type_descriptions =
		{
			{ ACCESS_TYPE_EMPTY, "empty" },
			{ ACCESS_TYPE_TSS16_A, "16-bit TSS (available)" },
			{ ACCESS_TYPE_LDT, "LDT" },
			{ ACCESS_TYPE_TSS16_B, "16-bit TSS (busy)" },
			{ ACCESS_TYPE_CALLGATE16, "16-bit call gate" },
			{ ACCESS_TYPE_TASKGATE, "task gate" },
			{ ACCESS_TYPE_INTGATE16, "16-bit interrupt gate" },
			{ ACCESS_TYPE_TRAPGATE16, "16-bit trap gate" },
			{ ACCESS_TYPE_TSS32_A, "32-bit TSS (available)" },
			{ ACCESS_TYPE_TSS32_B, "32-bit TSS (busy)" },
			{ ACCESS_TYPE_CALLGATE32, "32-bit call gate" },
			{ ACCESS_TYPE_INTGATE32, "32-bit interrupt gate" },
			{ ACCESS_TYPE_TRAPGATE32, "32-bit trap gate" },
		};
		descriptor_entry.AddField("Access bits",
			Dumper::BitFieldDisplay::Make(2)
				->AddBitField(0, 4, Dumper::ChoiceDisplay::Make(type_descriptions), false)
				->AddBitField(5, 2, Dumper::DecDisplay::Make(), "privilege level")
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("present"), false),
			offset_t(access));
	}
	descriptor_entry.AddField("Attribute bits",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("alias"), true) // Ergo specific field
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("window"), true) // Ergo specific field
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("32-bit", "16-bit"), false)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("limit in pages", "limit in bytes"), false),
			offset_t(flags & 0xF0));
	descriptor_entry.Display(dump);
}

/* * * Writer members * * */

bool XPFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool XPFormat::FormatIs16bit() const
{
	return wordsize != DWord;
}

bool XPFormat::FormatIsProtectedMode() const
{
	return true;
}

std::string XPFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
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

std::shared_ptr<Linker::OptionCollector> XPFormat::GetOptions()
{
	return std::make_shared<XPOptionCollector>();
}

void XPFormat::SetOptions(std::map<std::string, std::string>& options)
{
	XPOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
	option_create_selector_pairs = collector.dual_selector();
	option_no_intermediate_selectors = collector.no_intermediate_selector();
}

std::vector<Linker::OptionDescription<void>> XPFormat::MemoryModelNames =
{
	Linker::OptionDescription<void>("default/small", "Small model, symbols in .code have a separate preferred segment"),
	Linker::OptionDescription<void>("compact", "Compact model, symbols in .data, .bss and .comm have a common preferred segment, all other sections are treated as separate segments"),
};

std::vector<Linker::OptionDescription<void>> XPFormat::GetMemoryModelNames()
{
	return MemoryModelNames;
}

void XPFormat::SetModel(std::string model)
{
	if(model == "")
	{
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "small")
	{
		memory_model = MODEL_SMALL;
	}
	else if(model == "compact")
	{
		memory_model = MODEL_COMPACT;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_SMALL;
	}
}

void XPFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		if(image != nullptr)
		{
			Linker::Error << "Error: duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		image = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, ignoring" << std::endl;
	}
}

void XPFormat::CreateDefaultSegments()
{
	if(image == nullptr)
	{
		image = std::make_shared<Linker::Segment>(".code");
	}
}

std::unique_ptr<Script::List> XPFormat::GetScript(Linker::Module& module)
{
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
		case MODEL_DEFAULT:
		case MODEL_SMALL:
			/* separate segment/address space for code and data, one for each */
			return Script::parse_string(SmallScript);
		case MODEL_COMPACT:
			/* separate address space for a single code segment and multiple data segments */
			return Script::parse_string(CompactScript);
		default:
			Linker::FatalError("Internal error: invalid memory model");
		}
	}
}

void XPFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

void XPFormat::ProcessModule(Linker::Module& module)
{
	// simulating EXPRESS behavior by generating binary according to MZ format

	Link(module);

	std::shared_ptr<Linker::Segment> image_segment = GetImageSegment();

	// TODO: create groups

	// zero filled sections should also be written to disk
	image_segment->Fill();

	offset_t full_image_size = image_segment->TotalSize();

	std::shared_ptr<Linker::Section> ss_section = nullptr;
	std::shared_ptr<Linker::Section> cs_section = nullptr;

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		ss_section = stack_top.section;
		esp = stack_top.GetPosition().address - (ss_section != nullptr ? ss_section->GetStartAddress() : 0);
	}
	else
	{
		// use last section for stack, as for MZ
		ss_section = module.Sections().back();
		esp = ss_section->Size();
//		Linker::Debug << "Debug: End of memory: " << sp << std::endl;
//		Linker::Debug << "Debug: Total size: " << (image->ImageSize() + zero_fill) << std::endl;
//		Linker::Debug << "Debug: Stack base: " << ss << std::endl;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		cs_section = entry.section;
		eip = entry.GetPosition().address - (cs_section != nullptr ? cs_section->GetStartAddress() : 0);
	}
	else
	{
		// use first section for stack, as for MZ
		cs_section = module.Sections().front();
		eip = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}

	std::map<std::shared_ptr<Linker::Section>, uint16_t> section_selectors;
	std::map<uint16_t, uint16_t> paragraph_selectors;

	// PSP
	ldt.push_back(Segment(0x0100, full_image_size - 1, Segment::ACCESS_DATA, 0));
	ldt.push_back(Segment(0x0100, full_image_size - 1, Segment::ACCESS_CODE, Segment::FLAG_ALIAS));

	for(auto section : image_segment->sections)
	{
		if(section->IsExecutable() || section == cs_section)
		{
			offset_t start_address = section->GetStartAddress();
			uint16_t selector = (ldt.size() + 1) * 8 + 7;
			section_selectors[section] = selector;
			paragraph_selectors[start_address >> 4] = selector;

			ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_DATA, Segment::FLAG_WINDOW));
			ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_CODE, Segment::FLAG_WINDOW));
		}
	}

	for(auto section : image_segment->sections)
	{
		// TODO: generate stack attribute for sections
		if((section->GetFlags() & Linker::Section::Stack) != 0 || section == ss_section)
		{
			offset_t start_address = section->GetStartAddress();
			uint16_t selector = ldt.size() * 8 + 7;
			section_selectors[section] = selector;
			paragraph_selectors[start_address >> 4] = selector;

			ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_DATA, Segment::FLAG_WINDOW));
			ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_CODE, Segment::FLAG_WINDOW));
		}
	}

	for(auto& group : section_groups)
	{
		// TODO: none of this is tested, the linker needs support for groups
		uint16_t selector = ldt.size() * 8 + 7;
		offset_t start_address = group.GetStartAddress(this);
		offset_t group_length = group.GetLength(this);
		ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), group_length - 1), Segment::ACCESS_DATA, Segment::FLAG_WINDOW));
		ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), group_length - 1), Segment::ACCESS_CODE, Segment::FLAG_WINDOW));

		if(option_no_intermediate_selectors)
		{
			for(size_t selector_index = group.first_section + 1; selector_index < group.first_section + group.section_count; selector_index++)
			{
				auto section = image_segment->sections[selector_index];
				offset_t start_address = section->GetStartAddress();
				section_selectors[section] = selector;
				paragraph_selectors[start_address >> 4] = selector;
			}
		}
	}

	for(auto section : image_segment->sections)
	{
		if(section_selectors.find(section) != section_selectors.end())
			continue;

		offset_t start_address = section->GetStartAddress();
		uint16_t selector = ldt.size() * 8 + 7;
		section_selectors[section] = selector;
		paragraph_selectors[start_address >> 4] = selector;

		ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_DATA, Segment::FLAG_WINDOW));
		if(option_create_selector_pairs)
			ldt.push_back(Segment(0x0100 + start_address, std::min(offset_t(0xFFFF), full_image_size - start_address - 1), Segment::ACCESS_CODE, Segment::FLAG_WINDOW));
	}

	cs = section_selectors[cs_section];
	ss = section_selectors[ss_section];

	std::set<uint32_t> relocation_offsets;
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		switch(rel.kind)
		{
		case Linker::Relocation::Direct:
			rel.WriteWord(resolution.value);
			break;
		case Linker::Relocation::SelectorIndex:
			if(resolution.reference != nullptr)
			{
				Linker::Error << "Error: intersegment differences impossible in protected mode, ignoring" << std::endl;
				continue;
			}

Linker::Debug << "Selector index for value " << std::hex << resolution.value + (rel.addend << 4) << std::endl;
			{
				auto selector_pair = paragraph_selectors.find((resolution.value >> 4) + rel.addend);
				if(selector_pair == paragraph_selectors.end())
				{
					Linker::Error << "Error: no selector allocated for paragraph, ignoring" << std::endl;
					continue;
				}
				rel.WriteWord(selector_pair->second);
			}
			break;
		default:
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}

	ds = es = 0x07; // PSP
	eflags = 0x3242; // IOPL 3, interrupts enabled, zero flags on
	ecx = full_image_size;
}

std::shared_ptr<Linker::Segment> XPFormat::GetImageSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(image);
}

offset_t XPFormat::Group::GetStartAddress(XPFormat * format) const
{
	// group selectors begin at the second section
	return format->GetImageSegment()->sections[first_section + 1]->GetStartAddress();
}

offset_t XPFormat::Group::GetLength(XPFormat * format) const
{
	// group length encompasses the entire group
	return format->GetImageSegment()->sections[first_section + section_count - 1]->GetEndAddress()
		- format->GetImageSegment()->sections[first_section]->GetStartAddress();
}

