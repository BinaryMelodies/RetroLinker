
#include "dosexe.h"

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
		throw "Unknown signature";
		break;
	}
	rd.Skip(3);
	minimum_dos_version = rd.ReadUnsigned(2);
	dlink_version = rd.ReadUnsigned(2);
	uint32_t image_size = rd.ReadUnsigned(4);
	header_size = rd.ReadUnsigned(4);
	uint32_t program_size = rd.ReadUnsigned(4);
	relocation_size = image_size - header_size - program_size;
	extra_memory_size = rd.ReadUnsigned(4) - program_size;
	eip = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	uint32_t relocation_count = rd.ReadUnsigned(4);
	flags = rd.ReadUnsigned(4);

	rd.Skip(header_size - 0x28);

	if(image)
	{
		delete image;
	}
	image = new Linker::Buffer(program_size);
	dynamic_cast<Linker::Buffer *>(image)->ReadFile(rd, program_size);

	for(size_t i = 0; i < relocation_count; i++)
	{
		relocations.insert(rd.ReadUnsigned(4));
	}
}

void SeychellDOS32::AdamFormat::WriteFile(Linker::Writer& wr)
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
		wr.WriteWord(4, header_size + image->ActualDataSize() + relocation_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, image->ActualDataSize());
		wr.WriteWord(4, image->ActualDataSize() + extra_memory_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, relocations.size());
		wr.WriteWord(4, flags);
	}
	else
	{
#if 0
		/* TODO */
		wr.WriteWord(4, image->ActualDataSize() + relocation_size);
		wr.WriteWord(4, header_size + image->ActualDataSize() + relocation_size); /* ? */
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, memory_size);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, header_size + image->ActualDataSize()); /* ? */
		wr.WriteWord(4, flags);
#endif
	}

	wr.Skip(header_size - 0x28);
	image->WriteFile(wr);

	for(auto rel : relocations)
	{
		wr.WriteWord(4, rel);
	}
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

void BrocaD3X::D3X1Format::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(4, "D3X1");
	wr.WriteWord(4, header_size);
	wr.WriteWord(4, binary_size);
	wr.WriteWord(4, extra_size);
	wr.WriteWord(4, entry);
	wr.WriteWord(4, stack_top);
	/* TODO */
}

void DX64::FlatFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	/* TODO */
}

void DX64::FlatFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	/* TODO */
}

void DX64::LVFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	/* TODO */
}

void DX64::LVFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	/* TODO */
}

