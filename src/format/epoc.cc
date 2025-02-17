
#include "epoc.h"
#include "../linker/buffer.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

using namespace EPOC;

uint32_t SymbianFormat::RelocationSection::GetSize() const
{
	uint32_t size = 8;
	for(auto& block : blocks)
	{
		size += 8 + 2 * block.relocations.size();
	}
	return size;
}

uint32_t SymbianFormat::RelocationSection::GetCount() const
{
	uint32_t count = 0;
	for(auto& block : blocks)
	{
		for(auto relocation : block.relocations)
		{
			if(!(relocation.offset == 0 && relocation.type == 0))
			{
				count++;
			}
		}
	}
	return count;
}

void SymbianFormat::RelocationSection::ReadFile(Linker::Reader& rd)
{
	offset_t section_start = rd.Tell();
	uint32_t size = rd.ReadUnsigned(4);
	offset_t section_end = section_start + size;
	uint32_t count = rd.ReadUnsigned(4);
	while(rd.Tell() < section_end)
	{
		RelocationBlock block;
		block.page_offset = rd.ReadUnsigned(4);
		uint32_t block_size = rd.ReadUnsigned(4);
		for(uint32_t i = 8; i < block_size; i += 4)
		{
			uint16_t word = rd.ReadUnsigned(2);
			block.relocations.push_back(Relocation{uint16_t(word & 0x0FFF), Relocation::relocation_type(word >> 12)});
		}
		blocks.emplace_back(block);
	}
}

void SymbianFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian; // TODO
	uid1 = executable_type(rd.ReadUnsigned(4));
	uid2 = application_type(rd.ReadUnsigned(4));
	uid3 = rd.ReadUnsigned(4);
	uid_checksum = rd.ReadUnsigned(4);
	rd.Skip(4); /* 'EPOC' */
	if(!new_format)
	{
		cpu = cpu_type(rd.ReadUnsigned(4));
		code_checksum = rd.ReadUnsigned(4);
		data_checksum = rd.ReadUnsigned(4);
	}
	else
	{
		header_crc = rd.ReadUnsigned(4);
		module_version = rd.ReadUnsigned(4);
		compression_type = rd.ReadUnsigned(4);
	}
	tool_version = rd.ReadUnsigned(4);
	timestamp = rd.ReadUnsigned(8);
	flags = flags_type(rd.ReadUnsigned(4));
	uint32_t code_size = rd.ReadUnsigned(4);
	uint32_t data_size = rd.ReadUnsigned(4);
	heap_size_min = rd.ReadUnsigned(4);
	heap_size_max = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	entry_point = rd.ReadUnsigned(4);
	code_address = rd.ReadUnsigned(4);
	data_address = rd.ReadUnsigned(4);
	uint32_t dll_ref_table_count = rd.ReadUnsigned(4);
	export_table_offset = rd.ReadUnsigned(4);
	uint32_t export_dir_count = rd.ReadUnsigned(4);
	import_table_offset = rd.ReadUnsigned(4);
	code_offset = rd.ReadUnsigned(4);
	data_offset = rd.ReadUnsigned(4);
	import_offset = rd.ReadUnsigned(4);
	code_relocation_offset = rd.ReadUnsigned(4);
	data_relocation_offset = rd.ReadUnsigned(4);
	if(!new_format)
	{
		process_priority = rd.ReadUnsigned(4);
	}
	else
	{
		process_priority = rd.ReadUnsigned(2);
		cpu = cpu_type(rd.ReadUnsigned(2));
	}

	if(new_format)
	{
		uncompressed_size = rd.ReadUnsigned(4);
		secure_id = rd.ReadUnsigned(4);
		vendor_id = rd.ReadUnsigned(4);
		capabilities[0] = capability_type(rd.ReadUnsigned(4));
		capabilities[1] = capability_type(rd.ReadUnsigned(4));
		exception_descriptor = rd.ReadUnsigned(4);
		rd.Skip(4);
		uint16_t export_description_size = rd.ReadUnsigned(2);
		export_description_type = rd.ReadUnsigned(1);
		export_description.resize(export_description_size);
		rd.ReadData(export_description);
	}

	// Code section
	rd.Seek(code_offset);
	code = Linker::Buffer::ReadFromFile(rd, import_table_offset);

	// Import address table
	// TODO: read

	// Export table
	rd.Seek(export_table_offset);
	// TODO: read

	// Data section
	rd.Seek(data_offset);
	data = Linker::Buffer::ReadFromFile(rd, data_size);

	// Import table
	rd.Seek(import_offset);
	uint32_t import_table_size = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < dll_ref_table_count; i++)
	{
		ImportBlock block;
		block.name_offset = rd.ReadUnsigned(4);
		uint32_t import_count = rd.ReadUnsigned(4);
		for(uint32_t j = 0; j < import_count; j++)
		{
			block.imports.push_back(rd.ReadUnsigned(4));
		}
		dll_reference_table.emplace_back(block);
	}

	// Code relocations
	rd.Seek(code_relocation_offset);
	code_relocations.ReadFile(rd);

	// Data relocations
	rd.Seek(data_relocation_offset);
	data_relocations.ReadFile(rd);
}

offset_t SymbianFormat::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

offset_t SymbianFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian; // TODO
	wr.WriteWord(4, uid1);
	wr.WriteWord(4, uid2);
	wr.WriteWord(4, uid3);
	wr.WriteWord(4, uid_checksum);
	wr.WriteData("EPOC");
	if(!new_format)
	{
		wr.WriteWord(4, cpu);
		wr.WriteWord(4, code_checksum);
		wr.WriteWord(4, data_checksum);
	}
	else
	{
		wr.WriteWord(4, header_crc);
		wr.WriteWord(4, module_version);
		wr.WriteWord(4, compression_type);
	}
	// TODO
	return ImageSize();
}

void SymbianFormat::Dump(Dumper::Dumper& dump) const
{
	// TODO
}

