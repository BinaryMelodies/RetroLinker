
#include "xpexp.h"
#include "../linker/buffer.h"
#include "../linker/location.h"

/* TODO: untested */

using namespace Ergo;

bool XPFormat::FormatSupportsSegmentation() const
{
	return true; // TODO: is this the case?
}

bool XPFormat::FormatIs16bit() const
{
	return wordsize != DWord;
}

bool XPFormat::FormatIsProtectedMode() const
{
	return true;
}

void XPFormat::CalculateValues()
{
	file_offset = 0; // TODO: if stub is available, later
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
	wordsize = Unknown;
	ldt.clear();
	image = nullptr;
}

void XPFormat::ReadFile(Linker::Reader& rd)
{
	Clear();
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	rd.Skip(4); // signature
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

std::string XPFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
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
					(access & ACCESS_E) != 0 ? "readable" : "writable",
					(access & ACCESS_E) != 0 ? "execute-only" : "read-only"), false)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make((access & ACCESS_E) != 0 ? "conforming" : "grow down"), true)
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
			->AddBitField(5, 2, Dumper::ChoiceDisplay::Make("window"), true) // Ergo specific field
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("32-bit", "16-bit"), false)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("limit in pages", "limit in bytes"), false),
			offset_t(flags & 0xF0));
	descriptor_entry.Display(dump);
}

