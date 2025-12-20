
#include "dosexe.h"

/* untested */

void SeychellDOS32::AdamFormat::CalculateValues()
{
	uint32_t relocations_size = 0;

	if(!IsV35())
	{
		selector_relocations.clear();
		for(auto offset_and_type : relocations_map)
		{
			if(offset_and_type.second == Selector16)
				selector_relocations.push_back(offset_and_type.first);
		}

		offset_relocations.clear();
		for(auto offset_and_type : relocations_map)
		{
			if(offset_and_type.second == Offset32)
				offset_relocations.push_back(offset_and_type.first);
		}

		relocation_count = selector_relocations.size();
		relocations_size = 4 * relocation_count;
	}
	else
	{
		// TODO: relocation_size for v3.5
	}

	if(!IsDLL())
	{
		memcpy(signature.data(), "Adam", 4);
	}
	else
	{
		memcpy(signature.data(), "DLL ", 4);
	}

	if(IsV35() || format == FORMAT_DX64)
	{
		header_size = std::max(header_size, uint32_t(0x2C));
	}
	else
	{
		header_size = std::max(header_size, uint32_t(0x28));
	}

	program_size = image->ImageSize();
	image_size = header_size + program_size + relocations_size;
	contents_size = image->ImageSize() + relocations_size;
}

void SeychellDOS32::AdamFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	offset_t image_end = rd.GetImageEnd();
	rd.ReadData(signature);
	rd.ReadData(dlink_version);
	rd.ReadData(minimum_dos_version);

	uint32_t relocation_count = 0;

	if(!IsV35())
	{
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		program_size = rd.ReadUnsigned(4);
		memory_size = rd.ReadUnsigned(4);
		eip = rd.ReadUnsigned(4);
		esp = rd.ReadUnsigned(4);
		relocation_count = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
	}
	else
	{
		contents_size = rd.ReadUnsigned(4);
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		eip = rd.ReadUnsigned(4);
		memory_size = rd.ReadUnsigned(4);
		esp = rd.ReadUnsigned(4);
		program_size = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
	}

	rd.Skip(header_size - 0x28);

	image = Linker::Buffer::ReadFromFile(rd, program_size);

	if(!IsV35())
	{
		for(size_t i = 0; i < relocation_count; i++)
		{
			uint32_t offset = rd.ReadUnsigned(4);
			selector_relocations.push_back(offset);
			relocations_map[offset] = Selector16;
		}

		// DX64
		while(rd.Tell() + 4 <= image_end)
		{
			uint32_t offset = rd.ReadUnsigned(4) - 3;
			offset_relocations.push_back(offset);
			relocations_map[offset] = Offset32;
		}
	}
	else
	{
		// TODO
	}
}

offset_t SeychellDOS32::AdamFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(signature);
	wr.WriteData(dlink_version);
	wr.WriteData(minimum_dos_version);
	if(!IsV35())
	{
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, image->ImageSize());
		wr.WriteWord(4, memory_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, relocation_count);
		wr.WriteWord(4, flags);
	}
	else
	{
		wr.WriteWord(4, contents_size);
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, memory_size);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, program_size);
		wr.WriteWord(4, flags);
	}

	wr.Skip(header_size - 0x28);
	image->WriteFile(wr);

	if(!IsV35())
	{
		for(auto rel : selector_relocations)
		{
			wr.WriteWord(4, rel);
		}

		// DX64
		for(auto rel : offset_relocations)
		{
			wr.WriteWord(4, rel + 3);
		}
	}
	else
	{
		// TODO: v3.5
	}

	return offset_t(-1);
}

void SeychellDOS32::AdamFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Adam format");
	Dumper::Region file_region("File", file_offset, image_size + (!IsV35() && offset_relocations.size() != 0 ? 4 * offset_relocations.size() : 0), 8);
	file_region.AddField("Signature", Dumper::StringDisplay::Make(4, "'"), std::string(signature.data(), 4));
	file_region.AddField("DLINK version", Dumper::VersionDisplay::Make(), offset_t(dlink_version[1]), offset_t(dlink_version[0]));
	file_region.AddField("DOS version", Dumper::VersionDisplay::Make(), offset_t(minimum_dos_version[1]), offset_t(minimum_dos_version[0]));
	if(IsV35())
		file_region.AddField("Image size (after header)", Dumper::HexDisplay::Make(8), offset_t(contents_size));
	if(!IsV35() && offset_relocations.size())
		file_region.AddField("Image size (without offset relocations)", Dumper::HexDisplay::Make(8), offset_t(image_size));
	file_region.AddField("Header size", Dumper::HexDisplay::Make(8), offset_t(header_size));
	file_region.AddField("Entry point (EIP)", Dumper::HexDisplay::Make(8), offset_t(eip)); // TODO: RIP
	file_region.AddField("Starting stack (ESP)", Dumper::HexDisplay::Make(8), offset_t(esp)); // TODO: RSP
	file_region.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(flags)); // TODO: print bits
	file_region.Display(dump);

	Dumper::Block image_block("Image", file_offset + header_size, image->AsImage(), 0 /* TODO */, 8);
	image_block.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(memory_size));
	for(auto rel : relocations_map)
	{
		image_block.AddSignal(rel.first, rel.second == Offset32 ? 4 : 2);
	}
	image_block.Display(dump);
}

void BorcaD3X::D3X1Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Skip(4); /* signature */
	header_size = rd.ReadUnsigned(4);
	binary_size = rd.ReadUnsigned(4);
	extra_size = rd.ReadUnsigned(4);
	entry = rd.ReadUnsigned(4);
	stack_top = rd.ReadUnsigned(4);
	/* TODO */
}

offset_t BorcaD3X::D3X1Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(4, "D3X1");
	wr.WriteWord(4, header_size);
	wr.WriteWord(4, binary_size);
	wr.WriteWord(4, extra_size);
	wr.WriteWord(4, entry);
	wr.WriteWord(4, stack_top);
	/* TODO */
	return offset_t(-1);
}

void BorcaD3X::D3X1Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("D3X1 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void DX64::LVFormat::SetSignature(format_type type)
{
	switch(type)
	{
	case FORMAT_FLAT:
		memcpy(signature, "Flat", 4);
		break;
	case FORMAT_LV:
		memcpy(signature, "LV\0\0", 4);
		break;
	default:
		Linker::FatalError("Internal error: invalid format type");
	}
}

void DX64::LVFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.ReadData(4, signature);
	if(memcmp(signature, "Flat", 4) != 0 && memcmp(signature, "LV\0\0", 4) != 0)
	{
		Linker::Error << "Error: invalid signature" << std::endl;
	}
	uint32_t program_size = rd.ReadUnsigned(4);
	eip = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	extra_memory_size = rd.ReadUnsigned(4);

	image = Linker::Buffer::ReadFromFile(rd, program_size);
}

offset_t DX64::LVFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(4, signature);
	wr.WriteWord(4, image->ImageSize());
	wr.WriteWord(4, eip);
	wr.WriteWord(4, esp);
	wr.WriteWord(4, extra_memory_size);
	image->WriteFile(wr);
	return offset_t(-1);
}

void DX64::LVFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("LV/Flat format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

