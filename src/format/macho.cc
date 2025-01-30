
#include "macho.h"

using namespace MachO;

// TODO: incomplete, untested

void MachOFormat::LoadCommand::ReadFile(Linker::Reader& rd)
{
	// by default, there is nothing to do here
}

void MachOFormat::LoadCommand::WriteFile(Linker::Writer& wr) const
{
	// by default, there is nothing to do here
}

std::unique_ptr<MachOFormat::LoadCommand> MachOFormat::LoadCommand::Read(Linker::Reader& rd)
{
	std::unique_ptr<MachOFormat::LoadCommand> load_command;
	command_type command = command_type(rd.ReadUnsigned(4));
	uint32_t size = rd.ReadUnsigned(4);
	switch(command & ~REQ_DYLD)
	{
	// TODO: other types
	case UUID:
	default:
		load_command = std::make_unique<GenericDataCommand>(command);
		break;
	}
	load_command->Read(rd, size);
	return load_command;
}

void MachOFormat::LoadCommand::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, command);
	wr.WriteWord(4, GetSize());
}

void MachOFormat::GenericDataCommand::Read(Linker::Reader& rd, offset_t size)
{
	command_image = Linker::Buffer::ReadFromFile(rd, size);
}

void MachOFormat::GenericDataCommand::Write(Linker::Writer& wr) const
{
	LoadCommand::Write(wr);
	command_image->WriteFile(wr);
}

offset_t MachOFormat::GenericDataCommand::GetSize() const
{
	return 8 + command_image->ImageSize();
}

MachOFormat::Section MachOFormat::Section::Read(Linker::Reader& rd, int wordsize)
{
	Section section;
	section.name = rd.ReadData(16); // TODO: trim
	section.segment_name = rd.ReadData(16); // TODO: trim
	section.address = rd.ReadUnsigned(wordsize);
	section.memory_size = rd.ReadUnsigned(wordsize);
	section.offset = rd.ReadUnsigned(4);
	section.align_shift = rd.ReadUnsigned(4);
	section.relocation_offset = rd.ReadUnsigned(4);
	section.relocation_count = rd.ReadUnsigned(4);
	section.flags = rd.ReadUnsigned(4);
	section.reserved1 = rd.ReadUnsigned(4);
	section.reserved2 = rd.ReadUnsigned(4);
	return section;
}

void MachOFormat::Section::Write(Linker::Writer& wr, int wordsize) const
{
	wr.WriteData(16, name, '\0'); // TODO: what is the padding character?
	wr.WriteData(16, segment_name, '\0'); // TODO: what is the padding character?
	wr.WriteWord(wordsize, address);
	wr.WriteWord(wordsize, memory_size);
	wr.WriteWord(4, offset);
	wr.WriteWord(4, align_shift);
	wr.WriteWord(4, relocation_offset);
	wr.WriteWord(4, relocation_count);
	wr.WriteWord(4, flags);
	wr.WriteWord(4, reserved1);
	wr.WriteWord(4, reserved2);
}

void MachOFormat::SegmentCommand::ReadFile(Linker::Reader& rd)
{
	// TODO
}

void MachOFormat::SegmentCommand::WriteFile(Linker::Writer& wr) const
{
	// TODO
}

void MachOFormat::SegmentCommand::Read(Linker::Reader& rd, offset_t size)
{
	size_t wordsize = command == SEGMENT_64 ? 8 : 4;
	name = rd.ReadData(16); // TODO: trim
	address = rd.ReadUnsigned(wordsize);
	memory_size = rd.ReadUnsigned(wordsize);
	offset = rd.ReadUnsigned(wordsize);
	file_size = rd.ReadUnsigned(wordsize);
	max_protection = rd.ReadUnsigned(4);
	init_protection = rd.ReadUnsigned(4);
	uint32_t section_count = rd.ReadUnsigned(4);
	flags = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < section_count; i++)
	{
		sections.emplace_back(Section::Read(rd, wordsize));
	}
}

void MachOFormat::SegmentCommand::Write(Linker::Writer& wr) const
{
	size_t wordsize = command == SEGMENT_64 ? 8 : 4;
	wr.WriteData(16, name, '\0'); // TODO: what is the padding character?
	wr.WriteWord(wordsize, address);
	wr.WriteWord(wordsize, memory_size);
	wr.WriteWord(wordsize, offset);
	wr.WriteWord(wordsize, file_size);
	wr.WriteWord(4, max_protection);
	wr.WriteWord(4, init_protection);
	wr.WriteWord(4, sections.size());
	wr.WriteWord(4, flags);
	for(auto& section : sections)
	{
		section.Write(wr, wordsize);
	}
}

offset_t MachOFormat::SegmentCommand::GetSize() const
{
	if(command == SEGMENT_64)
	{
		return 64 + 76 * sections.size();
	}
	else
	{
		return 48 + 68 * sections.size();
	}
}

void MachOFormat::ReadFile(Linker::Reader& rd)
{
	std::string signature = rd.ReadData(4);
	if(signature == "\xFE\xED\xFA\xCE")
	{
		wordsize = 4;
		endiantype = ::BigEndian;
	}
	else if(signature == "\xFE\xED\xFA\xCF")
	{
		wordsize = 8;
		endiantype = ::BigEndian;
	}
	else if(signature == "\xCE\xFA\xED\xFE")
	{
		wordsize = 4;
		endiantype = ::LittleEndian;
	}
	else if(signature == "\xCF\xFA\xED\xFE")
	{
		wordsize = 8;
		endiantype = ::LittleEndian;
	}
	else
	{
		Linker::FatalError("Fatal error: invalid Mach-O signature");
	}

	rd.endiantype = endiantype;
	cpu = cpu_type(rd.ReadUnsigned(4));
	cpu_subtype = rd.ReadUnsigned(4);
	file_type = rd.ReadUnsigned(4);
	uint32_t command_count = rd.ReadUnsigned(4);
	commands_size = rd.ReadUnsigned(4);
	flags = rd.ReadUnsigned(4);
	if(wordsize == 8)
	{
		rd.Skip(4);
	}

	for(uint32_t i = 0; i < command_count; i++)
	{
		load_commands.emplace_back(LoadCommand::Read(rd));
	}

	for(auto& command : load_commands)
	{
		command->ReadFile(rd);
	}
}

offset_t MachOFormat::WriteFile(Linker::Writer& wr)
{
	if(wordsize != 4 && wordsize != 8)
	{
		Linker::FatalError("Fatal error: invalid word size");
	}

	if(endiantype != ::BigEndian && endiantype != ::LittleEndian)
	{
		Linker::FatalError("Fatal error: invalid byte order");
	}

	switch(endiantype)
	{
	case ::BigEndian:
		switch(wordsize)
		{
		case 4:
			wr.WriteData("\xFE\xED\xFA\xCE");
			break;
		case 8:
			wr.WriteData("\xFE\xED\xFA\xCF");
			break;
		}
		break;
	case ::LittleEndian:
		switch(wordsize)
		{
		case 4:
			wr.WriteData("\xCE\xFA\xED\xFE");
			break;
		case 8:
			wr.WriteData("\xFE\xED\xFA\xCF");
			break;
		}
		break;
	default:
		break;
	}

	wr.endiantype = endiantype;
	wr.WriteWord(4, cpu);
	wr.WriteWord(4, cpu_subtype);
	wr.WriteWord(4, file_type);
	wr.WriteWord(4, load_commands.size());
	wr.WriteWord(4, commands_size);
	wr.WriteWord(4, flags);
	if(wordsize == 8)
	{
		wr.Skip(4);
	}

	for(auto& command : load_commands)
	{
		command->Write(wr);
	}

	for(auto& command : load_commands)
	{
		command->WriteFile(wr);
	}

	return offset_t(-1);
}

void MachOFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Mach-O format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

FatMachOFormat::Entry FatMachOFormat::Entry::Read(Linker::Reader& rd)
{
	Entry entry;
	entry.cpu = MachOFormat::cpu_type(rd.ReadUnsigned(4));
	entry.cpu_subtype = rd.ReadUnsigned(4);
	entry.offset = rd.ReadUnsigned(4);
	entry.size = rd.ReadUnsigned(4);
	entry.align = rd.ReadUnsigned(4);
	return entry;
}

void FatMachOFormat::Entry::Write(Linker::Writer& wr) const
{
	wr.WriteWord(4, cpu);
	wr.WriteWord(4, cpu_subtype);
	wr.WriteWord(4, offset);
	wr.WriteWord(4, size);
	wr.WriteWord(4, align);
}

offset_t FatMachOFormat::ImageSize()
{
	// the size of the header
	offset_t furthest = 8 + entries.size() * 20;
	for(auto& entry : entries)
	{
		offset_t entry_end = entry.offset + entry.size;
		if(entry_end > furthest)
			furthest = entry_end;
	}
	return furthest;
}

void FatMachOFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.Skip(4); // magic code
	uint32_t entry_count = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < entry_count; i++)
	{
		entries.emplace_back(Entry::Read(rd));
	}
	for(auto& entry : entries)
	{
		std::shared_ptr<MachOFormat> macho = std::make_shared<MachOFormat>();
		entry.image = macho;
		rd.Seek(entry.offset);
		macho->ReadFile(rd);
	}
}

offset_t FatMachOFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData("\xCA\xFE\xBA\xBE");
	wr.WriteWord(4, entries.size());
	for(auto& entry : entries)
	{
		entry.Write(wr);
	}
	for(auto& entry : entries)
	{
		wr.Seek(entry.offset);
		entry.image->WriteFile(wr);
	}
	return ImageSize();
}

void FatMachOFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Multiple-architecture Mach-O format");

	// TODO
}

void FatMachOFormat::CalculateValues()
{
	offset_t current_offset = 8 + entries.size() * 20;
	for(auto& entry : entries)
	{
		if(MachOFormat * macho = dynamic_cast<MachOFormat *>(entry.image.get()))
		{
			entry.cpu = macho->cpu;
			entry.cpu_subtype = macho->cpu_subtype;
		}
		else
		{
			entry.cpu = MachOFormat::cpu_type(
				(entry.image->GetByte(4) << 24) + (entry.image->GetByte(5) << 16) + (entry.image->GetByte(6) << 8) + entry.image->GetByte(7));
			entry.cpu_subtype = (entry.image->GetByte(8) << 24) + (entry.image->GetByte(9) << 16) + (entry.image->GetByte(10) << 8) + entry.image->GetByte(11);
		}
		offset_t page_shift = 12; // TODO: depends on architecture
		entry.offset = ::AlignTo(current_offset, 1 << page_shift);
		entry.size = entry.image->ImageSize(); // TODO
		entry.align = page_shift;
		current_offset = entry.offset + entry.size;
	}
}

