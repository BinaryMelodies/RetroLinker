
#include "xpexp.h"

/* TODO: untested */

using namespace Ergo;

void XPFormat::CalculateValues()
{
	offset = 0; // TODO: if stub is available, later
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
	offset = 0;
	ldt_offset = 0;
	image_offset = 0;
	relocation_offset = 0; // TODO: unsure
	minimum_extent = 0;
	maximum_extent = 0;
	unknown_field = 0;
	gs = fs = ds = ss = cs = es = edi = esi = ebp = esp = ebx = edx = ecx = eax = eflags = eip = 0;
	ldt.clear();
	image = nullptr;
}

void XPFormat::ReadFile(Linker::Reader& rd)
{
	Clear();
	rd.endiantype = ::LittleEndian;
	offset = rd.Tell();
	rd.Skip(4); // signature
	ldt_offset = rd.ReadUnsigned(4);
	uint32_t ldt_count = rd.ReadUnsigned(4);
	image_offset = rd.ReadUnsigned(4);
	uint32_t image_size = rd.ReadUnsigned(4);
	relocation_offset = rd.ReadUnsigned(4);
	uint32_t relocation_count = rd.ReadUnsigned(4);
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

	rd.Seek(offset + ldt_offset);
	for(uint32_t i = 0; i < ldt_count; i++)
	{
		ldt.push_back(Segment::ReadFile(rd));
	}

	rd.Seek(image_offset);
	image = Linker::Buffer::ReadFromFile(rd, image_size);

	// TODO: relocations, format unknown
	(void) relocation_count;
}

void XPFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(offset);
	wr.WriteData(4, "XP\1\0");
	wr.WriteWord(4, ldt_offset);
	wr.WriteWord(4, ldt.size());
	wr.WriteWord(4, image_offset);
	wr.WriteWord(4, image->ActualDataSize());
	wr.WriteWord(4, relocation_offset);
	wr.WriteWord(4, 0); // TODO: relocation_count
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

	wr.Seek(offset + ldt_offset);
	for(auto segment : ldt)
	{
		segment.WriteFile(wr);
	}

	wr.Seek(image_offset);
	image->WriteFile(wr);

	/* TODO */
}

std::string XPFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
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
	segment.limit |= (uint32_t)(segment.flags & 0x0F) << 16;
	segment.flags &= ~0x0F;
	segment.base |= (uint32_t)rd.ReadUnsigned(1) << 24;
	return segment;
}

void XPFormat::Segment::WriteFile(Linker::Writer& wr)
{
	wr.WriteWord(2, limit & 0xFFFF);
	wr.WriteWord(3, base & 0xFFFFFF);
	wr.WriteWord(1, access);
	wr.WriteWord(1,
		((limit >> 16) & 0x0F)
		| (flags & 0xF0));
	wr.WriteWord(1, base >> 24);
}

