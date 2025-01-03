
#include "8bitexe.h"

using namespace Binary;

// AppleFormat

void AppleFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	base_address = rd.ReadUnsigned(2);
	uint16_t size = rd.ReadUnsigned(2);
	Linker::Buffer * buffer = new Linker::Buffer;
	buffer->ReadFile(rd, size);
	image = buffer;
}

void AppleFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, base_address);
	wr.WriteWord(2, image->ActualDataSize());
	image->WriteFile(wr);
}

// AtariFormat

void AtariFormat::Segment::ReadFile(Linker::Reader& rd)
{
	uint16_t word = rd.ReadUnsigned(2);
	if(word >= 0xFFFF) /* TODO: check SpartaDOS X allowed entries */
	{
		header = word;
		header_optional = false;
		word = rd.ReadUnsigned(2);
	}
	else
	{
		header = 0xFFFF;
		header_optional = true;
	}
	address = word;
	uint16_t length = (rd.ReadUnsigned(2) + 1 - address) & 0xFFFF;
	Linker::Buffer * buffer = new Linker::Buffer;
	buffer->ReadFile(rd, length);
	image = buffer;
}

void AtariFormat::Segment::WriteFile(Linker::Writer& wr)
{
	if((uint32_t)address + image->ActualDataSize() > 0x10000)
	{
		Linker::Warning << "Warning: Address overflows" << std::endl;
	}
	if(!header_optional || header != 0xFFFF)
	{
		wr.WriteWord(2, header);
	}
	wr.WriteWord(2, address);
	wr.WriteWord(2, address + image->ActualDataSize() - 1);
	image->WriteFile(wr);
}

void AtariFormat::OnNewSegment(Linker::Segment * segment)
{
	Segment * atari_segment = new Segment(); /* TODO: header type */
	segment->Fill();
	atari_segment->address = segment->base_address;
	atari_segment->image = segment;
	if(segments.size() == 0)
		atari_segment->header_optional = false;
	segments.push_back(atari_segment);
}

void AtariFormat::ProcessModule(Linker::Module& module)
{
	BinaryFormat::ProcessModule(module);
	/* TODO: enable multiple segments */
	/* TODO: enable initializers and entry point */
}

void AtariFormat::ReadFile(Linker::Reader& rd)
{
	rd.Seek(0);
	if(rd.ReadUnsigned(2) != 0xFFFF)
	{
		throw new Linker::Exception("Fatal Error: Expected binary image to start with 0xFFFF");
	}
	while(true) /* TODO: check if file ended */
	{
		Segment * segment = new Segment;
		segment->ReadFile(rd);
		segments.push_back(segment);
	}
}

void AtariFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	for(Segment * segment : segments)
	{
		segment->WriteFile(wr);
	}
}

// CommodoreFormat


void CommodoreFormat::Initialize()
{
	loader = nullptr;
}

void CommodoreFormat::Clear()
{
	if(loader != nullptr)
		delete loader;
	loader = nullptr;
}

void CommodoreFormat::SetupDefaultLoader()
{
	Linker::Section * loader_section = new Linker::Section(".loader");
	std::ostringstream oss;
	oss << " (" << base_address << ")";
	std::string text = oss.str();
	loader_section->WriteWord(2, base_address + 7 + text.size());
	loader_section->WriteWord(2, 10); /* line number */
	loader_section->WriteWord(1, BASIC_SYS);
	loader_section->Append(text.c_str());
	loader_section->WriteWord(2, 0);
	if(loader == nullptr)
	{
		loader = new Linker::Segment(".loader");
	}
	loader->Append(loader_section);
	loader->SetStartAddress(base_address - loader->data_size);
}

void CommodoreFormat::ProcessModule(Linker::Module& module)
{
	BinaryFormat::ProcessModule(module);
	SetupDefaultLoader(); /* TODO: if a separate loader is ready, use that instead */
}

void CommodoreFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, loader->base_address);
	loader->WriteFile(wr);
	image->WriteFile(wr);
}

std::string CommodoreFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".prg";
}

// CPM3Format

void CPM3Format::Initialize()
{
	memcpy(preinit_code, "\xC9\0\0\0\0\0\0\0\0\0", 10);
	loader_active = true;
	rsx_count = 0;
}

void CPM3Format::Clear()
{
	for(auto& rsx : rsx_table)
	{
		if(rsx.module)
			delete rsx.module;
	}
	rsx_table.clear();
}

void CPM3Format::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	rd.Skip(1);
	uint16_t data_size = rd.ReadUnsigned(2);
	rd.ReadData(10, preinit_code);
	loader_active = rd.ReadUnsigned(1) != 0;
	uint8_t rsx_count = rd.ReadUnsigned(1);
	for(int i = 0; i < rsx_count; i++)
	{
		rsx_record rsx;
		rsx.offset = rd.ReadUnsigned(2);
		rsx.length = rd.ReadUnsigned(2);
		rsx.nonbanked_only = rd.ReadUnsigned(1) != 0;
		rd.Skip(1);
		rsx.name = rd.ReadData(8, true);
		rd.Skip(2);
		rsx_table.push_back(rsx);
	}
	rd.Seek(0x100);
	Linker::Buffer * buffer = new Linker::Section(".code");
	image = buffer;
	buffer->ReadFile(rd, data_size);
	for(auto& rsx : rsx_table)
	{
		rd.Seek(rsx.offset);
		Linker::Buffer * buffer = new Linker::Section(".code");
		rsx.module->image = buffer;
		buffer->ReadFile(rd, rsx.length);
	}
}

void CPM3Format::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC9);
	wr.WriteWord(2, image->ActualDataSize());
	wr.WriteData(10, preinit_code);
	wr.WriteWord(1, rsx_table.size() == 0 && loader_active ? 0xFF : 0);
	wr.WriteWord(1, rsx_table.size());
	for(auto& rsx : rsx_table)
	{
		wr.WriteWord(2, rsx.offset);
		wr.WriteWord(2, rsx.module->image->ActualDataSize());
		wr.WriteWord(1, rsx.nonbanked_only ? 0xFF : 0);
		wr.Skip(1);
		wr.WriteData(8, rsx.name);
		wr.Skip(2);
	}
	image->WriteFile(wr);
	for(auto& rsx : rsx_table)
	{
		wr.Seek(rsx.offset);
		rsx.module->image->WriteFile(wr);
	}
}

// FLEXFormat

void FLEXFormat::Segment::WriteFile(Linker::Writer& wr)
{
	for(uint16_t offset = 0; offset < image->ActualDataSize(); offset += 0xFF)
	{
		/* cut the segment up into 255 byte morcels */
		wr.WriteWord(1, 0x02);
		wr.WriteWord(2, address + offset);
		uint16_t count = std::min((offset_t)0xFF, image->ActualDataSize() - address - offset);
		wr.WriteWord(1, count);
		image->WriteFile(wr, count, offset);
	}
}

void FLEXFormat::OnNewSegment(Linker::Segment * segment)
{
	Segment * flex_segment = new Segment();
	segment->Fill();
	flex_segment->address = segment->base_address;
	flex_segment->image = segment;
	segments.push_back(flex_segment);
}

void FLEXFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::BigEndian;
	for(Segment * segment : segments)
	{
		segment->WriteFile(wr);
	}
}

std::string FLEXFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".cmd";
}

// PRLFormat

/* TODO: prepare relocations offsets */
/* TODO: SPR files start at 0, OVL files start at a specified address */

void PRLFormat::OnNewSegment(Linker::Segment * segment)
{
	bool is_first_segment = image == nullptr;
	BinaryFormat::OnNewSegment(segment);
	if(is_first_segment)
	{
		zero_fill = segment->zero_fill;
	}
}

bool PRLFormat::ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution)
{
	rel.WriteWord(resolution.value);
	if(resolution.target != nullptr && resolution.reference == nullptr)
	{
		Linker::Debug << "Debug: PRL relocation: " << rel << " at " <<
			rel.source.GetPosition().GetSegmentOffset()
			<< std::endl;
		relocations.insert(rel.source.GetPosition().GetSegmentOffset());
	}
	return true;
}

void PRLFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0);
	wr.WriteWord(2, image->ActualDataSize());
	wr.WriteWord(1, 0);
	wr.WriteWord(2, zero_fill);
	wr.WriteWord(1, 0);
	wr.WriteWord(2, 0); /* load address, non-zero only for OVL files */
	wr.WriteWord(1, 0);
	wr.WriteWord(2, 0); /* base address of code group, usually zero */
	wr.Seek(0x0100);
	image->WriteFile(wr);

	if(!suppress_relocations) /* suppress relocations only for OVL files */
	{
		for(uint16_t offset = 0; offset < ::AlignTo(image->ActualDataSize(), 8); offset += 8)
		{
			uint8_t reloc_byte = 0;
			for(int byte = 0; byte < 8; byte ++)
			{
				if(relocations.find(offset + byte) != relocations.end())
				{
					reloc_byte |= 1 << (7 - byte);
				}
			}
			wr.WriteWord(1, reloc_byte);
		}
	}
}

// UZIFormat

/* TODO: base address should be 0x0103 */
void UZIFormat::ProcessModule(Linker::Module& module)
{
	BinaryFormat::ProcessModule(module);
	entry = 0x0103; /* TODO: enable entry point */
}

void UZIFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC3);
	wr.WriteWord(2, entry);
	if(uzi180_header)
	{
		wr.WriteData("UZI");
	}
	image->WriteFile(wr);
}

std::string UZIFormat::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

// UZI280Format

void UZI280Format::OnNewSegment(Linker::Segment * segment)
{
	if(segment->name == ".code")
	{
		code = segment;
	}
	else if(segment->name == ".data")
	{
		data = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, `.data`, ignoring" << std::endl;
	}
}

/* TODO: apparently both .code and .data are loaded at 0x0100 */

void UZI280Format::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, 0x00FF);
	wr.WriteWord(2, data->ActualDataSize());
	wr.WriteWord(2, code->ActualDataSize());
	wr.AlignTo(512);
	data->WriteFile(wr);
	wr.AlignTo(512);
	code->WriteFile(wr);
	wr.AlignTo(512);
}

std::string UZI280Format::GetDefaultExtension(Linker::Module& module)
{
	return "a.out";
}

