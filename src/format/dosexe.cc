
#include "dosexe.h"

/* untested */

void SeychellDOS32::AdamFormat::CalculateValues()
{
	is_v35 = (dlink_version & 0xFF) * (dlink_version >> 8) >= 0xF0; // Michael Tippach's condition
	uint32_t relocation_size = 0;
	if(!is_v35)
	{
		relocation_size = 4 * relocations.size();
	}
	else
	{
		// TODO: relocation_size for v3.5
	}
	image_size = header_size + image->ImageSize() + relocation_size;
	relocation_start = header_size + image->ImageSize();
}

void SeychellDOS32::AdamFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	switch(rd.ReadUnsigned(1))
	{
	case 'A':
		/* "Adam" */
		is_dll = false;
		break;
	case 'D':
		/* "DLL " */
		is_dll = true;
		break;
	default:
		Linker::FatalError("Fatal error: Unknown signature");
	}
	rd.Skip(3);
	minimum_dos_version = rd.ReadUnsigned(2);
	dlink_version = rd.ReadUnsigned(2);
	is_v35 = (dlink_version & 0xFF) * (dlink_version >> 8) >= 0xF0; // Michael Tippach's condition

	uint32_t program_size;
	uint32_t relocation_count = 0;

	if(!is_v35)
	{
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		program_size = rd.ReadUnsigned(4);
		extra_memory_size = rd.ReadUnsigned(4) - program_size;
		eip = rd.ReadUnsigned(4);
		esp = rd.ReadUnsigned(4);
		relocation_count = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
		// TODO: relocation size?
	}
	else
	{
		// TODO: needs verification
		program_size = rd.ReadUnsigned(4);
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		eip = rd.ReadUnsigned(4);
		extra_memory_size = rd.ReadUnsigned(4) - program_size;
		esp = rd.ReadUnsigned(4);
		relocation_start = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
		last_header_field = rd.ReadUnsigned(4);
	}

	rd.Skip(header_size - 0x28);

	image = Linker::Buffer::ReadFromFile(rd, program_size);

	for(size_t i = 0; i < relocation_count; i++)
	{
		if(is_v35)
			Linker::FatalError("Fatal error: relocations for 3.5 not implemented"); // TODO: not for v3.5
		relocations.insert(rd.ReadUnsigned(4));
	}
}

offset_t SeychellDOS32::AdamFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	if(!is_dll)
	{
		wr.WriteData(4, "Adam");
	}
	else
	{
		wr.WriteData(4, "DLL ");
	}
	wr.WriteWord(2, minimum_dos_version);
	wr.WriteWord(2, dlink_version);
	if(!is_v35)
	{
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, image->ImageSize());
		wr.WriteWord(4, image->ImageSize() + extra_memory_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, relocations.size());
		wr.WriteWord(4, flags);
	}
	else
	{
		wr.WriteWord(4, image->ImageSize() /* + relocation_size*/); // TODO
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, image->ImageSize() + extra_memory_size);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, relocation_start);
		wr.WriteWord(4, flags);
		wr.WriteWord(4, last_header_field);
	}

	wr.Skip(header_size - 0x28);
	image->WriteFile(wr);

	for(auto rel : relocations)
	{
		wr.WriteWord(4, rel);
	}

	return offset_t(-1);
}

void SeychellDOS32::AdamFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Adam format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void BrocaD3X::D3X1Format::ReadFile(Linker::Reader& rd)
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

offset_t BrocaD3X::D3X1Format::WriteFile(Linker::Writer& wr) const
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

void BrocaD3X::D3X1Format::Dump(Dumper::Dumper& dump) const
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

