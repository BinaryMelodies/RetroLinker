
#include "peexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Microsoft;

bool PEFormat::PEOptionalHeader::Is64Bit() const
{
	return magic == EXE64;
}

uint32_t PEFormat::PEOptionalHeader::GetSize() const
{
	return (Is64Bit() ? 112 : 96) + 8 * data_directories.size();
}

uint32_t PEFormat::PEOptionalHeader::AddressToRVA(offset_t address) const
{
	offset_t rva = address - image_base;
	if(address == 0)
		return 0;

	Linker::Debug << "Debug: Convert address " << std::hex << address << " with image base " << std::hex << image_base << " to " << std::hex << rva << std::endl;
	if(rva > 0xFFFFFFFF)
	{
		Linker::FatalError("Fatal error: relative virtual address exceeds 4GiB limit");
	}
	return uint32_t(rva);
}

offset_t PEFormat::PEOptionalHeader::RVAToAddress(uint32_t rva, bool suppress_on_zero) const
{
	if(suppress_on_zero && rva == 0)
		return 0;
	else
		return image_base + rva;
}

void PEFormat::PEOptionalHeader::ReadFile(Linker::Reader& rd)
{
	magic = rd.ReadUnsigned(2);
	version_stamp = rd.ReadUnsigned(2);
	code_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	entry_address = rd.ReadUnsigned(4);
	code_address = rd.ReadUnsigned(4);
	if(!Is64Bit())
	{
		data_address = rd.ReadUnsigned(4);
	}
	image_base = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	section_align = rd.ReadUnsigned(4);
	file_align = rd.ReadUnsigned(4);
	os_version.major = rd.ReadUnsigned(2);
	os_version.minor = rd.ReadUnsigned(2);
	image_version.major = rd.ReadUnsigned(2);
	image_version.minor = rd.ReadUnsigned(2);
	subsystem_version.major = rd.ReadUnsigned(2);
	subsystem_version.minor = rd.ReadUnsigned(2);
	win32_version = rd.ReadUnsigned(4);
	total_image_size = rd.ReadUnsigned(4);
	total_headers_size = rd.ReadUnsigned(4);
	checksum = rd.ReadUnsigned(4);
	subsystem = SubsystemType(rd.ReadUnsigned(2));
	flags = rd.ReadUnsigned(2);
	reserved_stack_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	committed_stack_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	reserved_heap_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	committed_heap_size = rd.ReadUnsigned(Is64Bit() ? 8 : 4);
	loader_flags = rd.ReadUnsigned(4);
	uint32_t directory_count = rd.ReadUnsigned(4);
	for(uint32_t i = 0; i < directory_count; i++)
	{
		DataDirectory dirent;
		dirent.address = rd.ReadUnsigned(4);
		dirent.size = rd.ReadUnsigned(4);
		data_directories.emplace_back(dirent);
	}
}

void PEFormat::PEOptionalHeader::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(2, magic);
	wr.WriteWord(2, version_stamp);
	wr.WriteWord(4, code_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, entry_address);
	wr.WriteWord(4, code_address);
	if(!Is64Bit())
	{
		wr.WriteWord(4, data_address);
	}
	wr.WriteWord(Is64Bit() ? 8 : 4, image_base);
	wr.WriteWord(4, section_align);
	wr.WriteWord(4, file_align);
	wr.WriteWord(2, os_version.major);
	wr.WriteWord(2, os_version.minor);
	wr.WriteWord(2, image_version.major);
	wr.WriteWord(2, image_version.minor);
	wr.WriteWord(2, subsystem_version.major);
	wr.WriteWord(2, subsystem_version.minor);
	wr.WriteWord(4, win32_version);
	wr.WriteWord(4, total_image_size);
	wr.WriteWord(4, total_headers_size);
	wr.WriteWord(4, checksum);
	wr.WriteWord(2, subsystem);
	wr.WriteWord(2, flags);
	wr.WriteWord(Is64Bit() ? 8 : 4, reserved_stack_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, committed_stack_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, reserved_heap_size);
	wr.WriteWord(Is64Bit() ? 8 : 4, committed_heap_size);
	wr.WriteWord(4, loader_flags);
	wr.WriteWord(4, data_directories.size());
	for(auto& dirent : data_directories)
	{
		wr.WriteWord(4, dirent.address);
		wr.WriteWord(4, dirent.size);
	}
}

offset_t PEFormat::PEOptionalHeader::CalculateValues(COFFFormat& coff)
{
	code_size = 0;
	data_size = 0;
	bss_size = 0;

	total_image_size = AlignTo(24 + GetSize() + coff.sections.size() * 40, section_align);

	for(auto& coff_section : coff.sections)
	{
		std::shared_ptr<Section> section = std::static_pointer_cast<Section>(coff_section);

		if((coff_section->flags & COFFFormat::Section::TEXT) != 0)
			code_size += AlignTo(coff_section->size, file_align);
		else if((coff_section->flags & COFFFormat::Section::DATA) != 0)
			data_size += AlignTo(coff_section->size, file_align);
		else if((coff_section->flags & COFFFormat::Section::BSS) != 0)
			bss_size += AlignTo(std::static_pointer_cast<Section>(coff_section)->virtual_size(), file_align);

		total_image_size += AlignTo(section->MemorySize(dynamic_cast<PEFormat&>(coff)), section_align);
	}

	code_address = AddressToRVA(coff.GetCodeSegment()->base_address);
	data_address = AddressToRVA(coff.GetDataSegment()->base_address); // technically not needed for 64-bit binaries

	// TODO: checksum must be calculated for critical DLLs

	return offset_t(-1);
}

void PEFormat::PEOptionalHeader::DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const
{
	static const std::map<offset_t, std::string> magic_choice =
	{
		{ ROM32, "32-bit ROM image" },
		{ EXE32, "32-bit executable (PE32)" },
		{ EXE64, "64-bit executable (PE32+)" },
	};
	header_region.AddField("File type", Dumper::ChoiceDisplay::Make(magic_choice, Dumper::HexDisplay::Make(4)), offset_t(magic));
	//header_region.AddOptionalField("Version stamp", Dumper::HexDisplay::Make(), offset_t(version_stamp)); // TODO
	header_region.AddField("Text size", Dumper::HexDisplay::Make(), offset_t(code_size));
	header_region.AddField("Data size", Dumper::HexDisplay::Make(), offset_t(data_size));
	header_region.AddField("Bss size",  Dumper::HexDisplay::Make(), offset_t(bss_size));
	std::string entry;
	switch(coff.cpu_type)
	{
	case CPU_I386:
		header_region.AddField("Entry address (RVA) (EIP)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_AMD64:
		header_region.AddField("Entry address (RVA) (RIP)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	case CPU_M68K:
	case CPU_PPC:
	case CPU_ARM:
	case CPU_ARM64:
	case CPU_ALPHA:
	case CPU_IA64:
	case CPU_MIPS:
	case CPU_SH:
		header_region.AddField("Entry address (RVA) (PC)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	default:
		header_region.AddField("Entry address (RVA)", Dumper::HexDisplay::Make(), offset_t(entry_address));
		break;
	}
	header_region.AddField("Text address (RVA)", Dumper::HexDisplay::Make(), offset_t(code_address));
	if(!Is64Bit())
		header_region.AddField("Data address (RVA)", Dumper::HexDisplay::Make(), offset_t(data_address));
	// TODO
}

void PEFormat::Section::ReadSectionData(Linker::Reader& rd, const COFFFormat& coff_format)
{
	ReadSectionData(rd, dynamic_cast<const PEFormat&>(coff_format));
}

void PEFormat::Section::WriteSectionData(Linker::Writer& wr, const COFFFormat& coff_format) const
{
	WriteSectionData(wr, dynamic_cast<const PEFormat&>(coff_format));
}

uint32_t PEFormat::Section::ImageSize(const COFFFormat& coff_format) const
{
	return ImageSize(dynamic_cast<const PEFormat&>(coff_format));
}

void PEFormat::Section::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	rd.Seek(section_pointer);
	std::dynamic_pointer_cast<Linker::Buffer>(image)->ReadFile(rd, ImageSize(fmt));
}

void PEFormat::Section::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	wr.Seek(section_pointer);
	image->WriteFile(wr);
}

uint32_t PEFormat::Section::ImageSize(const PEFormat& fmt) const
{
	return std::dynamic_pointer_cast<Linker::Segment>(image)->ImageSize();
}

uint32_t PEFormat::Section::MemorySize(const PEFormat& fmt) const
{
	return std::dynamic_pointer_cast<Linker::Segment>(image)->TotalSize();
}

void PEFormat::ResourceDirectory::AddResource(std::shared_ptr<Resource>& resource, size_t level)
{
	// TODO: test
	auto level_ref = resource->full_identifier[level];
	if(auto * level_name = std::get_if<std::string>(&level_ref))
	{
		for(auto& entry : name_entries)
		{
			if(entry.identifier == *level_name)
			{
				auto * subdir = std::get_if<std::shared_ptr<ResourceDirectory>>(&entry.content);
				if(!subdir)
				{
					Linker::Error << "Error: resource identifier mismatch" << std::endl;
					return;
				}
				if(level + 1 == resource->full_identifier.size())
				{
					Linker::Error << "Error: resource identifier mismatch" << std::endl;
					return;
				}
				(*subdir)->AddResource(resource, level + 1);
				return;
			}
		}

		if(level + 1 == resource->full_identifier.size())
		{
			Entry<std::string> entry{*level_name, resource};
			name_entries.push_back(entry);
		}
		else
		{
			std::shared_ptr<ResourceDirectory> subdir = std::make_shared<ResourceDirectory>();
			Entry<std::string> entry{*level_name, subdir};
			name_entries.push_back(entry);
			subdir->AddResource(resource, level + 1);
		}
	}
	else if(auto * level_id = std::get_if<uint32_t>(&level_ref))
	{
		for(auto& entry : id_entries)
		{
			if(entry.identifier == *level_id)
			{
				auto * subdir = std::get_if<std::shared_ptr<ResourceDirectory>>(&entry.content);
				if(!subdir)
				{
					Linker::Error << "Error: resource identifier mismatch" << std::endl;
					return;
				}
				if(level + 1 == resource->full_identifier.size())
				{
					Linker::Error << "Error: resource identifier mismatch" << std::endl;
					return;
				}
				(*subdir)->AddResource(resource, level + 1);
				return;
			}
		}

		if(level + 1 == resource->full_identifier.size())
		{
			Entry<uint32_t> entry{*level_id, resource};
			id_entries.push_back(entry);
		}
		else
		{
			std::shared_ptr<ResourceDirectory> subdir = std::make_shared<ResourceDirectory>();
			Entry<uint32_t> entry{*level_id, subdir};
			id_entries.push_back(entry);
			subdir->AddResource(resource, level + 1);
		}
	}
}

bool PEFormat::ResourcesSection::IsPresent() const
{
	return name_entries.size() > 0 || id_entries.size() > 0;
}

void PEFormat::ResourcesSection::Generate(PEFormat& fmt)
{
	// TODO
}

void PEFormat::ResourcesSection::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	// TODO
}

void PEFormat::ResourcesSection::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	// TODO
}

uint32_t PEFormat::ResourcesSection::ImageSize(const PEFormat& fmt) const
{
	return size;
}

uint32_t PEFormat::ResourcesSection::MemorySize(const PEFormat& fmt) const
{
	return virtual_size();
}

bool PEFormat::ImportsSection::IsPresent() const
{
	return directories.size() > 0;
}

void PEFormat::ImportsSection::Generate(PEFormat& fmt)
{
	uint32_t rva = address;

	// import directory table
	rva += 20 * (directories.size() + 1);

	// first list the import lookup tables
	for(auto& directory : directories)
	{
		directory.lookup_table_rva = rva;
		rva += (directory.import_table.size() + 1) * (fmt.Is64Bit() ? 8 : 4);
	}

	// then list the import address tables (identical formats)
	address_table_rva = rva;
	for(auto& directory : directories)
	{
		directory.address_table_rva = rva;
		rva += (directory.import_table.size() + 1) * (fmt.Is64Bit() ? 8 : 4);
	}
	address_table_size = rva - address_table_rva;

	// list all the hint/name pairs
	for(auto& directory : directories)
	{
		for(auto& import_entry : directory.import_table)
		{
			if(auto import_name = std::get_if<ImportDirectory::Name>(&import_entry))
			{
				import_name->rva = rva;
				rva = AlignTo(rva + 3 + import_name->name.size(), 2);
			}
		}
	}

	// list DLL names
	for(auto& directory : directories)
	{
		directory.name_rva = rva;
		rva += directory.name.size() + 1;
	}

	virtual_size() = size = rva - address;
}

void PEFormat::ImportsSection::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	// TODO
}

void PEFormat::ImportsSection::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	offset_t rva_to_offset = section_pointer - address;

	wr.Seek(section_pointer);

	for(auto& directory : directories)
	{
		wr.WriteWord(4, directory.lookup_table_rva);
		wr.WriteWord(4, directory.timestamp);
		wr.WriteWord(4, directory.forwarder_chain);
		wr.WriteWord(4, directory.name_rva);
		wr.WriteWord(4, directory.address_table_rva);
	}

	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);

	// first list the import lookup tables
	for(auto& directory : directories)
	{
		wr.Seek(rva_to_offset + directory.lookup_table_rva);
		for(auto& import_entry : directory.import_table)
		{
			if(auto import_name = std::get_if<ImportDirectory::Name>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, import_name->rva);
			}
			else if(auto import_id = std::get_if<ImportDirectory::Ordinal>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, (fmt.Is64Bit() ? 0x8000000000000000 : 0x80000000) + *import_id);
			}
			else
			{
				assert(false);
			}
		}
		wr.WriteWord(fmt.Is64Bit() ? 8 : 4, 0);
	}

	// then list the import address tables (identical formats)
	for(auto& directory : directories)
	{
		wr.Seek(rva_to_offset + directory.address_table_rva);
		for(auto& import_entry : directory.import_table)
		{
			if(auto import_name = std::get_if<ImportDirectory::Name>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, import_name->rva);
			}
			else if(auto import_id = std::get_if<ImportDirectory::Ordinal>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, (fmt.Is64Bit() ? 0x8000000000000000 : 0x80000000) + *import_id);
			}
			else
			{
				assert(false);
			}
		}
		wr.WriteWord(fmt.Is64Bit() ? 8 : 4, 0);
	}

	// list all the hint/name pairs
	for(auto& directory : directories)
	{
		for(auto& import_entry : directory.import_table)
		{
			if(auto import_name = std::get_if<ImportDirectory::Name>(&import_entry))
			{
				wr.Seek(rva_to_offset + import_name->rva);
				wr.WriteWord(2, import_name->hint);
				wr.WriteData(import_name->name);
				wr.WriteWord(1, 0);
			}
		}
	}

	// list DLL names
	for(auto& directory : directories)
	{
		wr.Seek(rva_to_offset + directory.name_rva);
		wr.WriteData(directory.name);
		wr.WriteWord(1, 0);
	}
}

uint32_t PEFormat::ImportsSection::ImageSize(const PEFormat& fmt) const
{
	return size;
}

uint32_t PEFormat::ImportsSection::MemorySize(const PEFormat& fmt) const
{
	return virtual_size();
}

void PEFormat::ExportsSection::SetEntry(uint32_t ordinal, std::shared_ptr<Export> entry)
{
	if(ordinal >= entries.size())
	{
		entries.resize(ordinal + 1);
	}

	if(entries[ordinal])
	{
		Linker::Error << "Error: overwriting entry " << ordinal << std::endl;
	}
	entries[ordinal] = std::make_optional<std::shared_ptr<Export>>(entry);

	if(entry->name)
	{
		if(named_exports.find(entry->name.value().name) != named_exports.end())
		{
			Linker::Error << "Error: overwriting entry " << entry->name.value().name << std::endl;
		}
		named_exports[entry->name.value().name] = ordinal;
	}
}

void PEFormat::ExportsSection::AddEntry(std::shared_ptr<Export> entry)
{
	entries.push_back(entry);

	if(entry->name)
	{
		if(named_exports.find(entry->name.value().name) != named_exports.end())
		{
			Linker::Error << "Error: overwriting entry " << entry->name.value().name << std::endl;
		}
		named_exports[entry->name.value().name] = entries.size() - 1;
	}
}

bool PEFormat::ExportsSection::IsPresent() const
{
	// TODO: can a DLL have 0 exported entries? is the .edata section present
	return entries.size() > 0;
}

void PEFormat::ExportsSection::Generate(PEFormat& fmt)
{
	uint32_t rva = address;

	// export directory table
	rva += 40;

	// export address table
	address_table_rva = rva;
	rva += 4 * entries.size();

	// export name pointer table
	name_table_rva = rva;
	rva += 4 * named_exports.size();

	// export ordinal table
	ordinal_table_rva = rva;
	rva += 2 * named_exports.size();

	// export name table
	dll_name_rva = rva;
	rva += dll_name.size() + 1;

	for(auto& entry_opt : entries)
	{
		if(entry_opt)
		{
			if(auto forwarder = std::get_if<Export::Forwarder>(&entry_opt.value()->value))
			{
				std::ostringstream oss;
				oss << forwarder->dll_name << ".";
				if(auto forwarder_s = std::get_if<std::string>(&forwarder->reference))
				{
					oss << *forwarder_s;
				}
				else if(auto forwarder_i = std::get_if<uint32_t>(&forwarder->reference))
				{
					oss << "#" << *forwarder_i;
				}
				forwarder->reference_name = oss.str();
				forwarder->rva = rva;
				rva += forwarder->reference_name.size() + 1;
			}
		}
	}

	for(auto named_export : named_exports)
	{
		auto& name_rva = entries[named_export.second].value()->name.value();
		name_rva.rva = rva;
		rva += name_rva.name.size() + 1;
	}

	virtual_size() = size = rva - address;
}

void PEFormat::ExportsSection::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	// TODO
}

void PEFormat::ExportsSection::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	offset_t rva_to_offset = section_pointer - address;

	wr.Seek(section_pointer);
	wr.WriteWord(4, flags);
	wr.WriteWord(4, timestamp);
	wr.WriteWord(2, version.major);
	wr.WriteWord(2, version.minor);
	wr.WriteWord(4, dll_name_rva);
	wr.WriteWord(4, ordinal_base);
	wr.WriteWord(4, entries.size());
	wr.WriteWord(4, named_exports.size());
	wr.WriteWord(4, address_table_rva);
	wr.WriteWord(4, name_table_rva);
	wr.WriteWord(4, ordinal_table_rva);

	// export address table
	wr.Seek(rva_to_offset + address_table_rva);
	for(auto& entry_opt : entries)
	{
		if(entry_opt)
		{
			if(auto forwarder = std::get_if<Export::Forwarder>(&entry_opt.value()->value))
			{
				wr.WriteWord(4, forwarder->rva);
			}
			else if(auto rva = std::get_if<uint32_t>(&entry_opt.value()->value))
			{
				wr.WriteWord(4, *rva);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			wr.WriteWord(4, 0);
		}
	}

	// export name pointer table
	wr.Seek(rva_to_offset + name_table_rva);
	for(auto named_export : named_exports)
	{
		auto& name_rva = entries[named_export.second].value()->name.value();
		wr.WriteWord(4, name_rva.rva);
	}

	// export ordinal table
	wr.Seek(rva_to_offset + ordinal_table_rva);
	for(auto named_export : named_exports)
	{
		wr.WriteWord(2, uint16_t(named_export.second - ordinal_base));
	}

	wr.Seek(rva_to_offset + dll_name_rva);
	wr.WriteData(dll_name);
	wr.WriteWord(1, 0);

	for(auto& entry_opt : entries)
	{
		if(entry_opt)
		{
			if(auto forwarder = std::get_if<Export::Forwarder>(&entry_opt.value()->value))
			{
				wr.Seek(rva_to_offset + forwarder->rva);
				wr.WriteData(forwarder->reference_name);
				wr.WriteWord(1, 0);
			}
		}
	}

	for(auto named_export : named_exports)
	{
		auto& name_rva = entries[named_export.second].value()->name.value();
		wr.Seek(rva_to_offset + name_rva.rva);
		wr.WriteData(name_rva.name);
		wr.WriteWord(1, 0);
	}
}

uint32_t PEFormat::ExportsSection::ImageSize(const PEFormat& fmt) const
{
	return size;
}

uint32_t PEFormat::ExportsSection::MemorySize(const PEFormat& fmt) const
{
	return virtual_size();
}

size_t PEFormat::BaseRelocation::GetEntryCount(const PEFormat * format) const
{
	switch(type)
	{
	case RelHighAdj:
		return 2;
	default:
		return 1;
	}
}

size_t PEFormat::BaseRelocation::GetRelocationSize(const PEFormat * format) const
{
	switch(type)
	{
	case RelAbsolute:
		return 0;
	case RelHigh:
		return 2;
	case RelLow:
		return 2;
	case RelHighLow:
		return 4;
	case RelHighAdj:
		return 2;
	case 5:
		switch(format->cpu_type)
		{
		case CPU_MIPS:
		//case RelMIPSJmpAddr:
			return 4;
		case CPU_ARM:
		//case RelARMMov32:
			return 8;
		case CPU_RISCV32:
		case CPU_RISCV64:
		case CPU_RISCV128:
		//case RelRISCVHigh20:
			return 4;
		default:
			return 0;
		}
	case 7:
		switch(format->cpu_type)
		{
		case CPU_ARM:
		//case RelThumbMov32:
			return 8;
		case CPU_RISCV32:
		case CPU_RISCV64:
		case CPU_RISCV128:
		//case RelRISCVLow12I:
			return 4;
		default:
			return 0;
		}
	case 8:
		switch(format->cpu_type)
		{
		case CPU_RISCV32:
		case CPU_RISCV64:
		case CPU_RISCV128:
		//case RelRISCVLow12S:
			return 4;
		//case RelLoongArch32MarkLA:
		//case RelLoongArch64MarkLA:
			return 8;
		default:
			return 0;
		}
	case 9:
		switch(format->cpu_type)
		{
		case CPU_MIPS:
		//case RelMIPSJmpAddr16:
			return 2;
		default:
			return 0;
		}
	case RelDir64:
		return 8;
	default:
		return 0;
	}
}

bool PEFormat::BaseRelocationsSection::IsPresent() const
{
	return relocations.size() > 0;
}

void PEFormat::BaseRelocationsSection::Generate(PEFormat& fmt)
{
	uint32_t full_size = 0;
	relocations_list.clear();
	for(auto pair : relocations_map)
	{
		relocations_list.push_back(pair.second);
	}

	for(auto relocations : relocations_list)
	{
		relocations->block_size = 4;
		for(auto pair : relocations->relocations_map)
		{
			relocations->relocations_list.push_back(pair.second);
			relocations->block_size += 2 * pair.second.GetEntryCount(&fmt);
		}
		full_size = AlignTo(full_size, 4) + relocations->block_size;
	}

	virtual_size() = size = full_size;
}

void PEFormat::BaseRelocationsSection::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	// TODO
}

void PEFormat::BaseRelocationsSection::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	offset_t offset = section_pointer;
	for(auto block : relocations_list)
	{
		offset = AlignTo(offset, 4);
		wr.Seek(offset);
		wr.WriteWord(4, block->page_rva);
		wr.WriteWord(4, block->block_size);

		for(auto relocation : block->relocations_list)
		{
			wr.WriteWord(4, (relocation.type << 12) | (relocation.offset));
			if(relocation.GetEntryCount(&fmt) >= 2)
			{
				wr.WriteWord(4, relocation.parameter);
			}
		}

		offset += block->block_size;
	}
}

uint32_t PEFormat::BaseRelocationsSection::ImageSize(const PEFormat& fmt) const
{
	return size;
}

uint32_t PEFormat::BaseRelocationsSection::MemorySize(const PEFormat& fmt) const
{
	return virtual_size();
}

PEFormat::PEOptionalHeader& PEFormat::GetOptionalHeader()
{
	return *dynamic_cast<PEFormat::PEOptionalHeader *>(optional_header.get());
}

const PEFormat::PEOptionalHeader& PEFormat::GetOptionalHeader() const
{
	return *dynamic_cast<const PEFormat::PEOptionalHeader *>(optional_header.get());
}

bool PEFormat::Is64Bit() const
{
	return GetOptionalHeader().Is64Bit();
}

uint32_t PEFormat::AddressToRVA(offset_t address) const
{
	return GetOptionalHeader().AddressToRVA(address);
}

offset_t PEFormat::RVAToAddress(uint32_t rva, bool suppress_on_zero) const
{
	return GetOptionalHeader().RVAToAddress(rva, suppress_on_zero);
}

void PEFormat::AddBaseRelocation(uint32_t rva, BaseRelocation::relocation_type type, uint16_t low_ref)
{
	uint32_t page_rva = rva & ~(BaseRelocationBlock::PAGE_SIZE - 1);
	if(base_relocations->relocations_map.find(page_rva) == base_relocations->relocations_map.end())
	{
		base_relocations->relocations_map[page_rva] = std::make_shared<BaseRelocationBlock>(page_rva);
	}
	uint16_t page_offset = rva & (BaseRelocationBlock::PAGE_SIZE - 1);
	if(base_relocations->relocations_map.find(page_offset) != base_relocations->relocations_map.end())
	{
		Linker::Error << "Error: duplicate relocation for address " << std::hex << rva << std::endl;
	}
	BaseRelocation rel(type, page_offset);
	if(rel.GetEntryCount(this) >= 2)
	{
		// we will store low_ref in the following relocation entry
		rel.parameter = low_ref;
	}
	base_relocations->relocations_map[page_rva]->relocations_map[page_offset] = rel;
}

void PEFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	rd.ReadData(4, pe_signature);
	ReadCOFFHeader(rd);
	ReadOptionalHeader(rd);
	ReadRestOfFile(rd);
}

::EndianType PEFormat::GetMachineEndianType() const
{
	switch(target)
	{
	case TargetWin9x:
	case TargetWinNT:
	case TargetWinCE: // TODO: not sure
	case TargetWin32s:
	case TargetTNT:
	case TargetEFI: // TODO: not sure
	case TargetDotNET: // TODO: ???
	case TargetXbox: // TODO: not sure
	default:
		return ::LittleEndian;
	case TargetMacintosh:
		return ::BigEndian;
	}
}

void PEFormat::CalculateValues()
{
	COFFFormat::CalculateValues();
	file_offset = stub.GetStubImageSize();

	flags = FLAG_EXECUTABLE;
	if(!option_relocatable)
		flags |= FLAG_NO_RELOCATIONS;

	if(option_include_deprecated_flags)
	{
		if(!option_coff_line_numbers)
			flags |= option_coff_local_symbols;
		if(!option_coff_line_numbers)
			flags |= FLAG_NO_SYMBOLS;

		// TODO: make FLAG_TRIM_WORKING_SET an option

		// TODO: should we include this? only Watcom seems to generate it, not even Microsoft's own linker
		switch(GetMachineEndianType())
		{
		case ::LittleEndian:
			flags |= FLAG_LITTLE_ENDIAN;
			break;
		case ::BigEndian:
			flags |= FLAG_BIG_ENDIAN;
			break;
		default:
			break;
		}
	}

	if(!Is64Bit())
		flags |= FLAG_32BIT;
	if(Is64Bit()) // TODO: enable as option
		flags |= FLAG_LARGE_ADDRESS;

	if(option_debug_info)
		flags |= FLAG_NO_DEBUG;

	// TODO: make FLAG_ON_REMOVABLE/FLAG_ON_NETWORK and FLAG_UNIPROCESSOR_ONLY options

	switch(output)
	{
	case OUTPUT_EXE:
		break;
	case OUTPUT_DLL:
		flags |= FLAG_LIBRARY;
		break;
	case OUTPUT_SYS:
		// TODO: unsure if this is what triggers this flag
		flags |= FLAG_SYSTEM;
		break;
	}

	offset_t section_header_size = 40;
	offset_t offset = file_offset + 24 + optional_header->GetSize() + sections.size() * section_header_size;

	for(auto& coff_section : sections)
	{
		std::shared_ptr<Section> section = std::static_pointer_cast<Section>(coff_section);
		section->virtual_size() = section->MemorySize(*this);
		section->size = section->ImageSize(*this);
		section->section_pointer = offset = AlignTo(offset, GetOptionalHeader().file_align);
		offset += section->size;
	}

	optional_header->CalculateValues(*this);
}

void PEFormat::ReadOptionalHeader(Linker::Reader& rd)
{
	optional_header = std::make_unique<PEOptionalHeader>();
	optional_header->ReadFile(rd);
}

offset_t PEFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	stub.WriteStubImage(wr);
	wr.Seek(file_offset);
	wr.WriteData(4, pe_signature);
	WriteFileContents(wr);
	return offset_t(-1);
}

void PEFormat::Dump(Dumper::Dumper& dump) const
{
	// TODO: Windows encoding
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PE format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

/* * * Writer members * * */

bool PEFormat::FormatSupportsLibraries() const
{
	return true;
}

unsigned PEFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	// TODO: do we need stack/heap?
	return 0;
}

std::shared_ptr<PEFormat::Resource>& PEFormat::AddResource(std::shared_ptr<Resource>& resource)
{
	resources->AddResource(resource);
	return resource;
}

PEFormat::ImportDirectory& PEFormat::FetchImportLibrary(std::string library_name, bool create_if_not_present)
{
	auto imported_library_reference = imports->library_indexes.find(library_name);
	if(imported_library_reference != imports->library_indexes.end())
	{
		return imports->directories[imported_library_reference->second];
	}
	else if(create_if_not_present)
	{
		imports->directories.push_back(ImportDirectory(library_name));
		imports->library_indexes[library_name] = imports->directories.size() - 1;
		return imports->directories.back();
	}
	else
	{
		Linker::FatalError("Internal error: library " + library_name + " not included");
	}
}

void PEFormat::ImportDirectory::AddImportByName(std::string entry_name, uint16_t hint)
{
	if(imports_by_name.find(entry_name) != imports_by_name.end())
		return;

	import_table.push_back(ImportDirectory::Name(entry_name, hint));

	imports_by_name[entry_name] = import_table.size() - 1;
}

void PEFormat::ImportDirectory::AddImportByOrdinal(uint16_t ordinal)
{
	if(imports_by_ordinal.find(ordinal) != imports_by_ordinal.end())
		return;

	ImportDirectory::ImportTableEntry entry = ImportDirectory::Ordinal(ordinal);
	import_table.push_back(entry);

	imports_by_ordinal[ordinal] = import_table.size() - 1;
}

offset_t PEFormat::ImportDirectory::GetImportByNameAddress(const PEFormat& fmt, std::string name)
{
	auto import_pointer = imports_by_name.find(name);
	if(import_pointer == imports_by_name.end())
	{
		Linker::FatalError("Internal error: imported name " + name + " not included");
	}

	return fmt.RVAToAddress(address_table_rva + import_pointer->second * (fmt.Is64Bit() ? 8 : 4));
}

offset_t PEFormat::ImportDirectory::GetImportByOrdinalAddress(const PEFormat& fmt, uint16_t ordinal)
{
	auto import_pointer = imports_by_ordinal.find(ordinal);
	if(import_pointer == imports_by_ordinal.end())
	{
		Linker::FatalError("Internal error: imported ordinal not included"); // TODO: display ordinal
	}

	return fmt.RVAToAddress(address_table_rva + import_pointer->second * (fmt.Is64Bit() ? 8 : 4));
}

void PEFormat::AddImportByName(std::string library_name, std::string entry_name, uint16_t hint)
{
	FetchImportLibrary(library_name, true).AddImportByName(entry_name, hint);
}

void PEFormat::AddImportByOrdinal(std::string library_name, uint16_t ordinal)
{
	FetchImportLibrary(library_name, true).AddImportByOrdinal(ordinal);
}

std::vector<Linker::OptionDescription<void> *> PEFormat::ParameterNames =
{
	// TODO
};

std::vector<Linker::OptionDescription<void> *> PEFormat::GetLinkerScriptParameterNames()
{
	return ParameterNames;
}

std::shared_ptr<Linker::OptionCollector> PEFormat::GetOptions()
{
	return std::make_shared<PEOptionCollector>();
}

void PEFormat::SetOptions(std::map<std::string, std::string>& options)
{
	PEOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();

	target = collector.target();

	switch(collector.subsystem())
	{
	default:
		switch(target)
		{
		default:
			GetOptionalHeader().subsystem = PEOptionalHeader::WindowsGUI;
			break;
		case TargetEFI:
			GetOptionalHeader().subsystem = PEOptionalHeader::EFIApplication;
			break;
		case TargetXbox:
			GetOptionalHeader().subsystem = PEOptionalHeader::Xbox;
			break;
		}
		break;
	case PEOptionCollector::Subsystem_Windows:
		if(target == TargetWinCE)
		{
			GetOptionalHeader().subsystem = PEOptionalHeader::WinCEGUI;
		}
		else
		{
			GetOptionalHeader().subsystem = PEOptionalHeader::WindowsGUI;
		}
		break;
	case PEOptionCollector::Subsystem_Console:
		GetOptionalHeader().subsystem = PEOptionalHeader::WindowsCUI;
		break;
	case PEOptionCollector::Subsystem_Native:
		if(target == TargetWin9x)
		{
			GetOptionalHeader().subsystem = PEOptionalHeader::NativeWin95;
		}
		else
		{
			GetOptionalHeader().subsystem = PEOptionalHeader::Native;
		}
		break;
	case PEOptionCollector::Subsystem_OS2:
		GetOptionalHeader().subsystem = PEOptionalHeader::OS2CUI;
		break;
	case PEOptionCollector::Subsystem_POSIX:
		GetOptionalHeader().subsystem = PEOptionalHeader::POSIXCUI;
		break;
	case PEOptionCollector::Subsystem_EFIApplication:
		GetOptionalHeader().subsystem = PEOptionalHeader::EFIApplication;
		break;
	case PEOptionCollector::Subsystem_EFIBootServiceDriver:
		GetOptionalHeader().subsystem = PEOptionalHeader::EFIBootServiceDriver;
		break;
	case PEOptionCollector::Subsystem_EFIRuntimeDriver:
		GetOptionalHeader().subsystem = PEOptionalHeader::EFIRuntimeDriver;
		break;
	case PEOptionCollector::Subsystem_EFI_ROM:
		GetOptionalHeader().subsystem = PEOptionalHeader::EFIROM;
		break;
	case PEOptionCollector::Subsystem_BootApplication:
		GetOptionalHeader().subsystem = PEOptionalHeader::WindowsBootApplication;
		break;
	}

	output = collector.output();

	// TODO: DLL flags and similar

	// TODO: it is not known at this point whether this is 64-bit or not
	if(collector.image_base())
	{
		GetOptionalHeader().image_base = collector.image_base();
	}
	else
	{
		GetOptionalHeader().image_base = 0;
	}

	if(collector.section_align())
	{
		GetOptionalHeader().section_align = collector.section_align();
	}
	else
	{
		switch(target)
		{
		default:
			GetOptionalHeader().section_align = 0x00001000;
			break;
		case TargetDotNET:
			GetOptionalHeader().section_align = 0x00002000;
			break;
		case TargetMacintosh:
			if(cpu_type == CPU_M68K)
				GetOptionalHeader().section_align = 0x00000004;
			else
				GetOptionalHeader().section_align = 0x00000020;
			break;
		}
	}

	switch(target)
	{
	default:
		GetOptionalHeader().file_align = 0x00000200; // TODO: make option?
		break;
	case TargetMacintosh:
		if(cpu_type == CPU_M68K)
			GetOptionalHeader().file_align = 0x00000004;
		else
			GetOptionalHeader().file_align = 0x00000020;
		break;
	}

	switch(GetOptionalHeader().subsystem)
	{
	case PEOptionalHeader::Unknown:
		break;

	case PEOptionalHeader::Native:
		// TODO
		break;

	case PEOptionalHeader::WindowsGUI:
	case PEOptionalHeader::WindowsCUI:
		switch(target)
		{
		case TargetWin32s:
		case TargetWin9x:
			// Windows 95 (4.0)
			GetOptionalHeader().subsystem_version = version_type{4, 0};
			break;
		case TargetWinNT:
		case TargetTNT:
			switch(cpu_type)
			{
			case CPU_I386:
			case CPU_ALPHA:
			case CPU_MIPS:
				// NT 3.1
				GetOptionalHeader().subsystem_version = version_type{3, 10};
				break;
			case CPU_PPC:
				// NT 3.5
				GetOptionalHeader().subsystem_version = version_type{3, 50};
				break;
			case CPU_IA64:
				// NT 5.1 (Windows XP 64-bit Edition)
				GetOptionalHeader().subsystem_version = version_type{5, 1};
				break;
			case CPU_AMD64:
				// NT 5.2 (Windows XP Professional x64 Edition)
				GetOptionalHeader().subsystem_version = version_type{5, 2};
				break;
			case CPU_ARM:
			case CPU_ARM64: // TODO
				// NT 6.2 (Windows Phone 8)
				GetOptionalHeader().subsystem_version = version_type{6, 2};
				break;
			default:
				break;
			}
			break;
		case TargetWinCE:
			switch(cpu_type)
			{
			case CPU_SH:
			case CPU_MIPS:
				// CE 1.0
				GetOptionalHeader().subsystem_version = version_type{1, 0};
				break;
			case CPU_I386:
			case CPU_ARM:
			case CPU_PPC:
				// CE 2.0
				GetOptionalHeader().subsystem_version = version_type{2, 0};
				break;
			default:
				break;
			}
			break;
		case TargetMacintosh:
			break;
		case TargetDotNET:
			GetOptionalHeader().subsystem_version = version_type{4, 0};
			break;
		case TargetEFI:
			GetOptionalHeader().subsystem_version = version_type{1, 0};
			break;
		case TargetXbox:
			// TODO
			break;
		}
		break;

	case PEOptionalHeader::OS2CUI:
		// TODO
		break;

	case PEOptionalHeader::POSIXCUI:
		GetOptionalHeader().subsystem_version = version_type{1, 0};
		break;

	case PEOptionalHeader::NativeWin95:
		// TODO
		break;

	case PEOptionalHeader::WinCEGUI:
		switch(cpu_type)
		{
		case CPU_SH:
		case CPU_MIPS:
			// CE 1.0
			GetOptionalHeader().subsystem_version = version_type{1, 0};
			break;
		case CPU_I386:
		case CPU_ARM:
		case CPU_PPC:
			// CE 2.0
			GetOptionalHeader().subsystem_version = version_type{2, 0};
			break;
		default:
			break;
		}
		break;

	case PEOptionalHeader::EFIApplication:
	case PEOptionalHeader::EFIBootServiceDriver:
	case PEOptionalHeader::EFIRuntimeDriver:
	case PEOptionalHeader::EFIROM:
		GetOptionalHeader().subsystem_version = version_type{1, 0};
		break;

	case PEOptionalHeader::Xbox:
		// TODO
		break;

	case PEOptionalHeader::WindowsBootApplication:
		// Microsoft linker
		GetOptionalHeader().subsystem_version = version_type{1, 0};
		break;
	}

	if(target == TargetTNT)
		memcpy(pe_signature, "PL\0\0", 4);
	else
		memcpy(pe_signature, "PE\0\0", 4);
	
	/* TODO */
}

void PEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		if(GetCodeSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 0)
		{
			Linker::Warning << "Warning: Out of order `.code` segment" << std::endl;
		}
		sections.push_back(std::make_unique<Section>(Section::TEXT | Section::EXECUTE | Section::READ, segment));
	}
	else if(segment->name == ".data")
	{
		if(GetDataSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.data` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 1)
		{
			Linker::Warning << "Warning: Out of order `.data` segment" << std::endl;
		}
		sections.push_back(std::make_unique<Section>(Section::DATA | Section::READ | Section::WRITE, segment));
	}
	else if(segment->name == ".bss")
	{
		if(GetBssSegment() != nullptr)
		{
			Linker::Error << "Error: Duplicate `.bss` segment, ignoring" << std::endl;
			return;
		}
		if(sections.size() != 2)
		{
			Linker::Warning << "Warning: Out of order `.bss` segment" << std::endl;
		}
		sections.push_back(std::make_unique<Section>(Section::BSS | Section::READ | Section::WRITE, segment));
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

std::unique_ptr<Script::List> PEFormat::GetScript(Linker::Module& module)
{
	// TODO: the first section must come after the COFF/PE and section headers
	// and so the start of the code section depends on the number of sections

	static const char * DefaultScript = R"(
".code"
{
	at ?image_base? + ?section_align?;
	all not write;
};

".data"
{
	at align(here, ?section_align?);
	all not zero;
};

".bss"
{
	at align(here, ?section_align?);
	all zero;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(DefaultScript);
	}
}

void PEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

void PEFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	GetOptionalHeader().data_directories.clear();
	GetOptionalHeader().data_directories.resize(PEOptionalHeader::DirTotalCount, PEOptionalHeader::DataDirectory{0, 0});

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string library;
		if(symbol.LoadLibraryName(library))
		{
			uint16_t hint;
			std::string name;
			if(!symbol.LoadOrdinalOrHint(hint))
			{
				hint = 0;
			}

			if(symbol.LoadName(name))
			{
				AddImportByName(library, name, hint);
			}
			else
			{
				AddImportByOrdinal(library, hint);
			}
		}
	}

	/* first make entries for those symbols exported by ordinals, or those that have hints */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(it.first.LoadOrdinalOrHint(ordinal))
		{
			std::shared_ptr<Export> exported_symbol;
			if(!it.first.IsExportedByOrdinal())
			{
				std::string name = "";
				it.first.LoadName(name);
				exported_symbol = std::make_shared<Export>(AddressToRVA(it.second.GetPosition().address), name);
			}
			else
			{
				exported_symbol = std::make_shared<Export>(AddressToRVA(it.second.GetPosition().address));
			}
			exports->SetEntry(ordinal, exported_symbol);
		}
	}

	/* then make entries for those symbols exported by name only */
	for(auto it : module.GetExportedSymbols())
	{
		if(!it.first.IsExportedByOrdinal())
		{
			std::shared_ptr<Export> exported_symbol;
			std::string name = "";
			it.first.LoadName(name);
			exported_symbol = std::make_shared<Export>(AddressToRVA(it.second.GetPosition().address), name);
			exports->AddEntry(exported_symbol);
		}
	}

	for(auto& coff_section : sections)
	{
		std::shared_ptr<Section> section = std::static_pointer_cast<Section>(coff_section);
		std::shared_ptr<Linker::Segment> image = GetSegment(coff_section);
		section->name = image->name;
		section->address = AddressToRVA(image->base_address);
	}

	offset_t image_end = GetOptionalHeader().image_base + 1;
	if(auto bss = GetBssSegment())
	{
		image_end = bss->base_address + bss->zero_fill;
	}
	else if(auto data = GetDataSegment())
	{
		image_end = data->base_address + data->data_size;
	}
	else if(auto code = GetCodeSegment())
	{
		image_end = code->base_address + code->data_size;
	}

	image_end = AlignTo(image_end, GetOptionalHeader().section_align);

	// TODO: order of these sections

	if(resources->IsPresent())
	{
		// generate .rsrc
		sections.push_back(resources);
		resources->address = AddressToRVA(image_end);
		resources->Generate(*this);
		image_end = AlignTo(image_end + resources->MemorySize(*this), GetOptionalHeader().section_align);
		GetOptionalHeader().data_directories[PEOptionalHeader::DirResourceTable] =
			PEOptionalHeader::DataDirectory{uint32_t(resources->address), uint32_t(resources->size)};
	}

	if(imports->IsPresent())
	{
		// generate .idata
		sections.push_back(imports);
		imports->address = AddressToRVA(image_end);
		imports->Generate(*this);
		image_end = AlignTo(image_end + imports->MemorySize(*this), GetOptionalHeader().section_align);
		GetOptionalHeader().data_directories[PEOptionalHeader::DirImportTable] =
			PEOptionalHeader::DataDirectory{uint32_t(imports->address), uint32_t(imports->size)};
		GetOptionalHeader().data_directories[PEOptionalHeader::DirIAT] =
			PEOptionalHeader::DataDirectory{uint32_t(imports->address_table_rva), uint32_t(imports->address_table_size)};
	}

	if(exports->IsPresent())
	{
		// generate .edata
		sections.push_back(imports);
		exports->address = AddressToRVA(image_end);
		exports->Generate(*this);
		image_end = AlignTo(image_end + exports->MemorySize(*this), GetOptionalHeader().section_align);
		GetOptionalHeader().data_directories[PEOptionalHeader::DirExportTable] =
			PEOptionalHeader::DataDirectory{uint32_t(exports->address), uint32_t(exports->size)};
	}

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(rel.Resolve(module, resolution))
		{
			if(resolution.target != nullptr)
			{
				if(resolution.reference != nullptr)
				{
					Linker::Error << "Error: intersegment relocations not supported, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				}

				switch(rel.kind)
				{
				case Linker::Relocation::Direct:
					switch(rel.size)
					{
					case 2:
						if((rel.mask & 0xFFFF) != 0xFFFF)
						{
							Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
						}

						switch(rel.shift)
						{
						default:
							Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
							/* fall through */
						case 0:
							AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelLow);
							break;
						case 16:
							AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelHigh);
							break;
						}
						break;
					case 4:
						if((rel.mask & 0xFFFFFFFF) != 0xFFFFFFFF)
						{
							Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
						}

						if(rel.shift != 0)
						{
							Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
						}

						AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelHighLow);
						break;
					case 8:
						if(rel.mask != 0xFFFFFFFFFFFFFFFF)
						{
							Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
						}

						if(rel.shift != 0)
						{
							Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
						}

						AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelDir64);
						break;
					}

					break;
				case Linker::Relocation::ParagraphAddress:
				case Linker::Relocation::SelectorIndex:
					Linker::Error << "Error: segment relocations not supported, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				default:
					Linker::Error << "Error: invalid relocation type, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				}
			}

			rel.WriteWord(resolution.value);
		}
		else
		{
			if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
			{
				switch(rel.kind)
				{
				case Linker::Relocation::Direct:
					switch(rel.size)
					{
					case 2:
						Linker::Error << "Error: imported 16-bit references not allowed, ignoring" << std::endl;
						Linker::Error << "Error: " << rel << std::endl;
						continue;
					case 4:
						if(Is64Bit())
						{
							Linker::Error << "Error: imported 32-bit references not allowed in 64-bit mode, ignoring" << std::endl;
							Linker::Error << "Error: " << rel << std::endl;
							continue;
						}

						if((rel.mask & 0xFFFFFFFF) != 0xFFFFFFFF)
						{
							Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
						}

						if(rel.shift != 0)
						{
							Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
						}
						break;
					case 8:
						if(!Is64Bit())
						{
							Linker::Error << "Error: imported 64-bit references not allowed in 32-bit mode, ignoring" << std::endl;
							Linker::Error << "Error: " << rel << std::endl;
							continue;
						}

						if(rel.mask != 0xFFFFFFFFFFFFFFFF)
						{
							Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
						}

						if(rel.shift != 0)
						{
							Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
						}
						break;
					}

					break;
				case Linker::Relocation::ParagraphAddress:
				case Linker::Relocation::SelectorIndex:
					Linker::Error << "Error: segment relocations not supported, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				default:
					Linker::Error << "Error: invalid relocation type, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				}

				std::string library, name;
				uint16_t ordinal;
				offset_t address = 0;

				if(symbol->GetImportedName(library, name))
				{
					address = FetchImportLibrary(library).GetImportByNameAddress(*this, name);
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					address = FetchImportLibrary(library).GetImportByOrdinalAddress(*this, ordinal);
				}
				else
				{
					Linker::Error << "Error: invalid import" << std::endl;
					continue;
				}

				rel.WriteWord(address);
			}
			else
			{
				Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			}
		}
	}

	if(base_relocations->IsPresent())
	{
		// generate .reloc
		sections.push_back(imports);
		base_relocations->address = AddressToRVA(image_end);
		base_relocations->Generate(*this);
		image_end = AlignTo(image_end + base_relocations->MemorySize(*this), GetOptionalHeader().section_align);
		GetOptionalHeader().data_directories[PEOptionalHeader::DirBaseRelocationTable] =
			PEOptionalHeader::DataDirectory{uint32_t(base_relocations->address), uint32_t(base_relocations->size)};
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		GetOptionalHeader().entry_address = AddressToRVA(entry.GetPosition().address);
	}
	else
	{
		GetOptionalHeader().entry_address = 0x1000; // TODO: find actual start of .code
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}

	GetOptionalHeader().os_version = version_type{4, 0}; // TODO: this seems to be a recurring value, whatever system it is compiled for
	//GetOptionalHeader().image_version = version_type{1, 0}; // TODO: customize
	// TODO: GetOptionalHeader().flags

	// TODO: make these customizable
	GetOptionalHeader().reserved_stack_size  = 0x00100000;
	GetOptionalHeader().committed_stack_size = 0x00001000;
	GetOptionalHeader().reserved_heap_size   = 0x00100000;
	GetOptionalHeader().committed_heap_size  = 0x00001000;
}

void PEFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	// PE is always stored as little endian, except for the actual binary data
	uint16_t machine_type = 0;
	endiantype = ::LittleEndian;

	switch(module.cpu)
	{
	case Linker::Module::I386:
		// 9x, NT, CE
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_I386;
		machine_type = 0x014C;
		break;
	case Linker::Module::X86_64:
		// NT
		GetOptionalHeader().magic = PEOptionalHeader::EXE64;
		cpu_type = CPU_AMD64;
		machine_type = 0x8664;
		break;
	case Linker::Module::M68K:
		// Mac
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_M68K;
		machine_type = 0x0268;
		break;
	case Linker::Module::PPC:
		// NT, CE, PPC
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_PPC;
		if(target == TargetMacintosh)
			machine_type = 0x0601;
		else
			machine_type = 0x01F0; // TODO: other values?
		break;
	case Linker::Module::ARM:
		// NT, CE
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_ARM;
		machine_type = 0x01C0; // TODO: other values?
		break;
	case Linker::Module::ARM64:
		// NT
		GetOptionalHeader().magic = PEOptionalHeader::EXE64;
		cpu_type = CPU_ARM64;
		machine_type = 0xAA64;
		break;
	case Linker::Module::MIPS:
		// NT, CE
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_MIPS;
		machine_type = 0x0166; // TODO: which value to use?
		break;
	case Linker::Module::SH:
		// CE
		GetOptionalHeader().magic = PEOptionalHeader::EXE32;
		cpu_type = CPU_SH;
		machine_type = 0x01A2; // TODO: which value to use?
		break;
	case Linker::Module::ALPHA:
		// NT
		GetOptionalHeader().magic = PEOptionalHeader::EXE32; // only 32-bit addressing was supported
		cpu_type = CPU_ALPHA;
		machine_type = 0x0184;
		// TODO: allow 32-bit? 0x0284
		break;
	case Linker::Module::IA64:
		// NT
		GetOptionalHeader().magic = PEOptionalHeader::EXE64;
		cpu_type = CPU_IA64;
		machine_type = 0x0200;
		break;
	default:
		Linker::Error << "Error: Unsupported CPU type" << std::endl;
	}

	AssignMagicValue(machine_type, ::LittleEndian);

	if(GetOptionalHeader().subsystem == PEOptionalHeader::EFIROM)
	{
		GetOptionalHeader().magic = PEOptionalHeader::ROM32;
	}

	if(GetOptionalHeader().image_base == 0)
	{
		switch(output)
		{
		case OUTPUT_EXE:
		case OUTPUT_SYS: // TODO
			switch(target)
			{
			case TargetWin9x:
			case TargetMacintosh:
				GetOptionalHeader().image_base = PEOptionalHeader::Win32Base;
				break;
			case TargetWinNT:
			default: // TODO
				if(Is64Bit())
					GetOptionalHeader().image_base = PEOptionalHeader::Win64Base;
				else
					GetOptionalHeader().image_base = PEOptionalHeader::Win32Base;
				break;
			case TargetWinCE:
				GetOptionalHeader().image_base = PEOptionalHeader::WinCEBase;
				break;
			}
			break;
		case OUTPUT_DLL:
			if(Is64Bit())
				GetOptionalHeader().image_base = PEOptionalHeader::Dll64Base;
			else
				GetOptionalHeader().image_base = PEOptionalHeader::Dll32Base;
			break;
		}
		// TODO
	}

	linker_parameters["image_base"] = GetOptionalHeader().image_base;
	linker_parameters["section_align"] = GetOptionalHeader().section_align;
	// TODO

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe";
}

