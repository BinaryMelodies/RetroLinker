
#include <filesystem>
#include <sstream>
#include "8bitexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Binary;

// AppleFormat

void AppleFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	base_address = rd.ReadUnsigned(2);
	uint16_t size = rd.ReadUnsigned(2);
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Buffer>();
	buffer->ReadFile(rd, size);
	image = buffer;
}

offset_t AppleFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, base_address);
	wr.WriteWord(2, image->ImageSize());
	image->WriteFile(wr);
	return offset_t(-1);
}

void AppleFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Apple 8-bit format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

// AtariFormat

offset_t AtariFormat::Segment::GetSize() const
{
	return image->ImageSize();
}

bool AtariFormat::HasEntryPoint() const
{
	for(auto& segment : segments)
	{
		if(segment->address <= ENTRY_ADDRESS && ENTRY_ADDRESS + 1 < segment->address + segment->GetSize())
			return true;
	}
	return false;
}

void AtariFormat::AddEntryPoint(uint16_t entry)
{
	std::unique_ptr<Segment> entry_segment = std::make_unique<Segment>();
	entry_segment->address = ENTRY_ADDRESS;
	std::shared_ptr<Linker::Section> entry_section = std::make_shared<Linker::Section>(".entry");
	entry_section->WriteWord(2, entry, ::LittleEndian);
	entry_segment->image = entry_section;
	segments.push_back(std::move(entry_segment));
}

void AtariFormat::Segment::ReadFile(Linker::Reader& rd)
{
	uint16_t word = rd.ReadUnsigned(2);
	if(word >= SIGNATURE_LOW)
	{
		header_type = segment_type(word);
		header_type_optional = false;
		if(header_type == ATARI_SEGMENT || header_type == SDX_FIXED)
			address = rd.ReadUnsigned(2);
	}
	else
	{
		header_type = ATARI_SEGMENT;
		header_type_optional = true;
		address = word;
	}
	switch(header_type)
	{
	case SDX_SYMREQ:
		block_number = rd.ReadUnsigned(1);
		rd.ReadData(8, symbol_name);
		address = rd.ReadUnsigned(2);
		break;
	case SDX_SYMDEF:
		rd.ReadData(8, symbol_name);
		ReadRelocations(rd);
		break;
	case SDX_FIXUPS:
		block_number = rd.ReadUnsigned(1);
		ReadRelocations(rd);
		break;
	case SDX_RAMALLOC:
	//case SDX_POSIND:
		block_number = rd.ReadUnsigned(1);
		control_byte = control_byte_type(rd.ReadUnsigned(1));
		address = rd.ReadUnsigned(2);
		size = rd.ReadUnsigned(2);
		if((control_byte & CB_RAMALLOC) == 0)
		{
			image = Linker::Buffer::ReadFromFile(rd, size);
		}
		break;
	case SDX_FIXED:
	case ATARI_SEGMENT:
		size = (rd.ReadUnsigned(2) + 1 - address) & 0xFFFF;
		image = Linker::Buffer::ReadFromFile(rd, size);
		break;
	default:
		Linker::FatalError("Fatal error: Invalid segment type");
	}
}

void AtariFormat::Segment::WriteFile(Linker::Writer& wr) const
{
	if(uint32_t(address) + GetSize() > 0x10000)
	{
		Linker::Warning << "Warning: Address overflows" << std::endl;
	}
	switch(header_type)
	{
	case SDX_SYMREQ:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		wr.WriteData(8, symbol_name);
		wr.WriteWord(2, address);
		break;
	case SDX_SYMDEF:
		wr.WriteWord(2, header_type);
		wr.WriteData(8, symbol_name);
		WriteRelocations(wr);
		break;
	case SDX_FIXUPS:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		WriteRelocations(wr);
		break;
	case SDX_RAMALLOC:
	//case SDX_POSIND:
		wr.WriteWord(2, header_type);
		wr.WriteWord(1, block_number);
		wr.WriteWord(1, control_byte);
		wr.WriteWord(2, address); // TODO: is this the right field?
		wr.WriteWord(2, size);
		if((control_byte & CB_RAMALLOC) == 0)
		{
			image->WriteFile(wr);
		}
		break;
	case SDX_FIXED:
	case ATARI_SEGMENT:
		if(!header_type_optional || header_type != ATARI_SEGMENT)
		{
			wr.WriteWord(2, header_type);
		}
		wr.WriteWord(2, address);
		wr.WriteWord(2, address + GetSize() - 1);
		image->WriteFile(wr);
		break;
	default:
		Linker::FatalError("Fatal error: Invalid segment type");
	}
}

void AtariFormat::Segment::ReadRelocations(Linker::Reader& rd)
{
	// TODO
}

void AtariFormat::Segment::WriteRelocations(Linker::Writer& wr) const
{
	// TODO
}

void AtariFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	std::unique_ptr<Segment> atari_segment = std::make_unique<Segment>(); /* TODO: header type, for now we set it to the default value 0xFFFF */
	segment->Fill();
	atari_segment->address = segment->base_address;
	atari_segment->image = segment;
	if(segments.size() == 0)
		atari_segment->header_type_optional = false;
	segments.push_back(std::move(atari_segment));
}

void AtariFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		AddEntryPoint(entry.GetPosition().address);
	}
	else if(!HasEntryPoint())
	{
		Linker::Warning << "Warning: no entry point has been provided" << std::endl;
	}

	/* TODO: enable multiple segments */
}

void AtariFormat::ReadFile(Linker::Reader& rd)
{
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(0);
	uint16_t signature = rd.ReadUnsigned(2);
	if(signature < Segment::segment_type::SIGNATURE_LOW)
	{
		Linker::FatalError("Fatal error: Expected binary image to start with 0xFFFF or valid SpartaDOS X signature");
	}
	rd.Seek(0);
	while(rd.Tell() < end)
	{
		std::unique_ptr<Segment> segment = std::make_unique<Segment>();
		segment->ReadFile(rd);
		segments.push_back(std::move(segment));
	}
}

offset_t AtariFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	for(auto& segment : segments)
	{
		segment->WriteFile(wr);
	}
	return offset_t(-1);
}

void AtariFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Atari 8-bit format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

// CommodoreFormat

void CommodoreFormat::Clear()
{
	loader = nullptr;
}

void CommodoreFormat::SetupDefaultLoader()
{
	std::shared_ptr<Linker::Section> loader_section = std::make_shared<Linker::Section>(".loader");
	std::ostringstream oss;
	oss << " (" << base_address << ")";
	std::string text = oss.str();
	//loader_section->WriteWord(2, base_address + 7 + text.size());
	loader_section->WriteWord(2, BASIC_START + 7 + text.size());
	loader_section->WriteWord(2, 10); /* line number */
	loader_section->WriteWord(1, BASIC_SYS);
	loader_section->Append(text.c_str());
	loader_section->WriteWord(2, 0); // TODO: why are two bytes
	loader_section->WriteWord(2, 0);
	if(loader == nullptr)
	{
		loader = std::make_shared<Linker::Segment>(".loader");
	}
	loader->Append(loader_section);
	//loader->SetStartAddress(base_address - loader->data_size);
	loader->SetStartAddress(BASIC_START);
}

void CommodoreFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);
	SetupDefaultLoader(); /* TODO: if a separate loader is ready, use that instead */
}

offset_t CommodoreFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, loader->base_address);
	loader->WriteFile(wr);
	image->WriteFile(wr);
	return offset_t(-1);
}

void CommodoreFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Commodore 8-bit format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

std::string CommodoreFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".prg";
}

// CPM3Format

void CPM3Format::rsx_record::OpenAndPrepare()
{
	if(rsx_file_name != "")
	{
		std::ifstream rsx_file;
		rsx_file.open(rsx_file_name, std::ios_base::in | std::ios_base::binary);
		if(rsx_file.is_open())
		{
			Linker::Reader rd(::LittleEndian, &rsx_file);
			module = std::make_shared<PRLFormat>();
			module->ReadFile(rd);
			rsx_file.close();

			uint8_t nonbanked_flag = module->image->AsImage()->GetByte(15);
			switch(nonbanked_flag)
			{
			case 0x00:
				nonbanked_only = false;
				break;
			case 0xFF:
				nonbanked_only = true;
				break;
			default:
				Linker::Warning << "Warning: invalid nonbank flag in RSX file, pretending to be 0" << std::endl;
				break;
			}
		}
		else
		{
			Linker::Error << "Error: unable to open RSX file " << rsx_file_name << ", generating dummy entry" << std::endl;
		}
	}
}

void CPM3Format::Clear()
{
	rsx_table.clear();
}

void CPM3Format::SetOptions(std::map<std::string, std::string>& options)
{
	if(auto rsx_file_names_option = FetchOption(options, "rsx"))
	{
		std::string rsx_file_names = rsx_file_names_option.value();
		size_t string_offset = 0;
		size_t comma;
		while((comma = rsx_file_names.find(',', string_offset)) != std::string::npos)
		{
			rsx_table.push_back(rsx_record());
			rsx_table.back().rsx_file_name = rsx_file_names.substr(string_offset, comma - string_offset);
			string_offset = comma + 1;
		}
		rsx_table.push_back(rsx_record());
		rsx_table.back().rsx_file_name = rsx_file_names.substr(string_offset);
	}

	for(auto& rsx : rsx_table)
	{
		size_t eq_offset = rsx.rsx_file_name.find('=');
		if(eq_offset != std::string::npos)
		{
			rsx.name = rsx.rsx_file_name.substr(0, eq_offset);
			rsx.rsx_file_name = rsx.rsx_file_name.substr(eq_offset + 1);
		}
		else
		{
			std::filesystem::path rsx_file_path(rsx.rsx_file_name);
			rsx.name = rsx_file_path.stem();
		}

		rsx.name.resize(8, ' ');
		std::transform(rsx.name.begin(), rsx.name.end(), rsx.name.begin(), ::toupper);
	}
}

void CPM3Format::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = ::LittleEndian;
	rd.Skip(1);
	uint16_t data_size = rd.ReadUnsigned(2);
	rd.ReadData(10, preinit_code);
	loader_active = rd.ReadUnsigned(1) != 0;
	rd.Skip(1);
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
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(".code");
	image = buffer;
	buffer->ReadFile(rd, data_size);
	for(auto& rsx : rsx_table)
	{
		rd.Seek(rsx.offset);
		std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Section>(".code");
		rsx.module->image = buffer;
		buffer->ReadFile(rd, rsx.length);
	}
}

offset_t CPM3Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC9);
	wr.WriteWord(2, image->ImageSize());
	wr.WriteData(10, preinit_code);
	wr.WriteWord(1, rsx_table.size() == 0 && loader_active ? 0xFF : 0);
	wr.Skip(1);
	wr.WriteWord(1, rsx_table.size());
	for(auto& rsx : rsx_table)
	{
		wr.WriteWord(2, rsx.offset);
		wr.WriteWord(2, rsx.module->image->ImageSize());
		wr.WriteWord(1, rsx.nonbanked_only ? 0xFF : 0);
		wr.Skip(1);
		wr.WriteData(8, rsx.name);
		wr.Skip(2);
	}
	wr.Seek(0x100);
	image->WriteFile(wr);
	for(auto& rsx : rsx_table)
	{
		wr.Seek(rsx.offset);
		rsx.module->WriteWithoutHeader(wr);
	}
	return offset_t(-1);
}

void CPM3Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("CP/M Plus format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

void CPM3Format::CalculateValues()
{
	uint16_t offset = 0x100 + image->ImageSize();
	for(auto& rsx : rsx_table)
	{
		rsx.offset = offset;
		rsx.OpenAndPrepare();
		offset += rsx.module->image->ImageSize() + ((rsx.module->image->ImageSize() + 7) >> 3);
	}
}

// FLEXFormat

void FLEXFormat::Segment::WriteFile(Linker::Writer& wr) const
{
	for(uint16_t offset = 0; offset < image->ImageSize(); offset += 0xFF)
	{
		/* cut the segment up into 255 byte morcels */
		wr.WriteWord(1, 0x02);
		wr.WriteWord(2, address + offset);
		uint16_t count = std::min(offset_t(0xFF), image->ImageSize() - address - offset);
		wr.WriteWord(1, count);
		image->WriteFile(wr, count, offset);
	}
}

void FLEXFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	std::unique_ptr<Segment> flex_segment = std::make_unique<Segment>();
	segment->Fill();
	flex_segment->address = segment->base_address;
	flex_segment->image = segment;
	segments.push_back(std::move(flex_segment));
}

offset_t FLEXFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	for(auto& segment : segments)
	{
		segment->WriteFile(wr);
	}
	return offset_t(-1);
}

void FLEXFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("FLEX format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

std::string FLEXFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".cmd";
}

// PRLFormat

/* TODO: prepare relocations offsets */
/* TODO: SPR files start at 0, OVL files start at a specified address */

void PRLFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	bool is_first_segment = image == nullptr;
	GenericBinaryFormat::OnNewSegment(segment);
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
		if(rel.size == 2)
		{
			Linker::Debug << "Debug: PRL relocation: " << rel << " at " <<
				rel.source.GetPosition().GetSegmentOffset() + 1
				<< std::endl;
			relocations.insert(rel.source.GetPosition().GetSegmentOffset() + 1);
		}
	}
	return true;
}

void PRLFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Seek(1);
	uint16_t image_size = rd.ReadUnsigned(2);
	rd.Skip(1);
	zero_fill = rd.ReadUnsigned(2);
	rd.Skip(1);
	load_address = rd.ReadUnsigned(2);
	rd.Skip(1);
	csbase = rd.ReadUnsigned(2);
	rd.Seek(0x0100);
	ReadWithoutHeader(rd, image_size);
}

void PRLFormat::ReadWithoutHeader(Linker::Reader& rd, uint16_t image_size)
{
	offset_t offset = rd.Tell();
	rd.SeekEnd();
	offset_t end = rd.Tell();
	rd.Seek(offset);

	image = Linker::Buffer::ReadFromFile(rd, image_size);

	relocations.clear();
	Linker::Debug << "Debug: File end: " << end << ", expected end with relocations: " << (offset + image_size + ((image_size + 7) >> 3)) << std::endl;
	if(end >= offset + image_size + ((image_size + 7) >> 3))
	{
		suppress_relocations = false;
		for(uint16_t byte_offset = 0; byte_offset < image_size; byte_offset += 8)
		{
			uint8_t reloc_byte = rd.ReadUnsigned(1);
			for(int byte = 7; byte >= 0; byte --)
			{
				if((reloc_byte & 1) != 0 && byte_offset + byte < image_size)
				{
					relocations.insert(byte_offset + byte);
				}
				reloc_byte >>= 1;
			}
		}
	}
	else
	{
		suppress_relocations = true;
	}
}

offset_t PRLFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0);
	wr.WriteWord(2, image->ImageSize());
	wr.WriteWord(1, 0);
	wr.WriteWord(2, zero_fill);
	wr.WriteWord(1, 0);
	wr.WriteWord(2, load_address); /* load address, non-zero only for OVL files */
	wr.WriteWord(1, 0);
	wr.WriteWord(2, csbase); /* base address of code group, usually zero */
	wr.Seek(0x0100);
	WriteWithoutHeader(wr);
	return offset_t(-1);
}

void PRLFormat::WriteWithoutHeader(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	image->WriteFile(wr);

	if(!suppress_relocations) /* suppress relocations only for OVL files */
	{
		Linker::Debug << "Debug: Writing relocations" << std::endl;
		for(uint16_t offset = 0; offset < ::AlignTo(image->ImageSize(), 8); offset += 8)
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

void PRLFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PRL format");
	Dumper::Region file_region("File", 0, offset_t(0x0100) + image->ImageSize() + (suppress_relocations ? 0 : (image->ImageSize() + 7) >> 3), 4);
	file_region.AddField("Zero fill", Dumper::HexDisplay::Make(4), offset_t(zero_fill));
	file_region.AddOptionalField("Load address", Dumper::HexDisplay::Make(4), offset_t(load_address));
	file_region.AddOptionalField("BIOS link", Dumper::HexDisplay::Make(4), offset_t(csbase));
	file_region.AddField("Relocations", Dumper::ChoiceDisplay::Make("present", "missing"), offset_t(!suppress_relocations));
	file_region.Display(dump);

	Dumper::Block image_block("Image", 0x0100, image->AsImage(), 0x0100, 4); // TODO: .SPR files
	for(auto relocation : relocations)
	{
		image_block.AddSignal(relocation, 1);
	}
	image_block.Display(dump);

	unsigned i = 0;
	for(auto relocation : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, 0x100 + image->ImageSize() + (relocation >> 3), 4);
		relocation_entry.AddField("Source", Dumper::HexDisplay::Make(4), offset_t(relocation));
		relocation_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(2), offset_t(image->AsImage()->ReadUnsigned(1, relocation, ::LittleEndian)));
		relocation_entry.Display(dump);
		i++;
	}
}

// UZIFormat

/* TODO: base address should be 0x0103 */
void UZIFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);
	entry = 0x0103; /* TODO: enable entry point */
}

offset_t UZIFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(1, 0xC3);
	wr.WriteWord(2, entry);
	if(uzi180_header)
	{
		wr.WriteData("UZI");
	}
	image->WriteFile(wr);
	return offset_t(-1);
}

void UZIFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("UZI format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

std::string UZIFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

// UZI280Format

void UZI280Format::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
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

offset_t UZI280Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteWord(2, 0x00FF);
	wr.WriteWord(2, data->ImageSize());
	wr.WriteWord(2, code->ImageSize());
	wr.AlignTo(512);
	data->WriteFile(wr);
	wr.AlignTo(512);
	code->WriteFile(wr);
	wr.AlignTo(512);
	return offset_t(-1);
}

void UZI280Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("UZI280 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 4);
	file_region.Display(dump);

	// TODO
}

std::string UZI280Format::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

