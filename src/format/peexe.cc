
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

	auto code = coff.GetCodeSegment();
	code_address = code ? AddressToRVA(code->base_address) : 0;
	auto data = coff.GetDataSegment();
	data_address = data ? AddressToRVA(data->base_address) : 0; // technically not needed for 64-bit binaries

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
	header_region.AddField("Entry address (RVA)", Dumper::HexDisplay::Make(), offset_t(entry_address));
	header_region.AddField("Text address (RVA)", Dumper::HexDisplay::Make(), offset_t(code_address));
	if(!Is64Bit())
		header_region.AddField("Data address (RVA)", Dumper::HexDisplay::Make(), offset_t(data_address));
	header_region.AddField("Image base", Dumper::HexDisplay::Make(Is64Bit() ? 16 : 8), offset_t(image_base));
	header_region.AddField("Section alignment in memory", Dumper::HexDisplay::Make(), offset_t(section_align));
	header_region.AddField("Section alignment in file", Dumper::HexDisplay::Make(), offset_t(file_align));
	header_region.AddField("OS version", Dumper::VersionDisplay::Make(), offset_t(os_version.major), offset_t(os_version.minor));
	header_region.AddField("Image version", Dumper::VersionDisplay::Make(), offset_t(image_version.major), offset_t(image_version.minor));
	header_region.AddField("Subsystem version", Dumper::VersionDisplay::Make(), offset_t(subsystem_version.major), offset_t(subsystem_version.minor));
	header_region.AddOptionalField("Win32 version (reserved)", Dumper::HexDisplay::Make(), offset_t(win32_version));
	header_region.AddField("Total image size in memory", Dumper::HexDisplay::Make(), offset_t(total_image_size));
	header_region.AddField("Total headers size in file", Dumper::HexDisplay::Make(), offset_t(total_headers_size));
	header_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(), offset_t(checksum));
	static const std::map<offset_t, std::string> subsystem_choice =
	{
		{ 0, "unknown" },
		{ 1, "device driver or native Windows process" },
		{ 2, "Windows GUI" },
		{ 3, "Windows character based" },
		{ 5, "OS/2 character based" },
		{ 7, "POSIX character based" },
		{ 8, "Native Windows 9x driver" },
		{ 9, "Windows CE" },
		{ 10, "EFI application" },
		{ 11, "EFI driver with boot services" },
		{ 12, "EFI driver with run-time services" },
		{ 13, "EFI ROM image" },
		{ 14, "Xbox" },
		{ 16, "Windows boot application" },
	};
	header_region.AddField("Subsystem type", Dumper::ChoiceDisplay::Make(subsystem_choice, Dumper::HexDisplay::Make(4)), offset_t(subsystem));
	header_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(5,  1, Dumper::ChoiceDisplay::Make("handles high entropy 64-bit address space"), true)
			->AddBitField(6,  1, Dumper::ChoiceDisplay::Make("relocatable DLL"), true)
			->AddBitField(7,  1, Dumper::ChoiceDisplay::Make("code integrity checks enforced"), true)
			->AddBitField(8,  1, Dumper::ChoiceDisplay::Make("NX compatible"), true)
			->AddBitField(9,  1, Dumper::ChoiceDisplay::Make("isolation aware"), true)
			->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("no structured exception handling"), true)
			->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("do not bind image"), true)
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("must run in AppContainer"), true)
			->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("WDM driver"), true)
			->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("supports control flow guard"), true)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("terminal server aware"), true),
		offset_t(flags));
	header_region.AddField("Reserved stack size", Dumper::HexDisplay::Make(Is64Bit() ? 16 : 8), offset_t(reserved_stack_size));
	header_region.AddField("Committed stack size", Dumper::HexDisplay::Make(Is64Bit() ? 16 : 8), offset_t(committed_stack_size));
	header_region.AddField("Reserved heap size", Dumper::HexDisplay::Make(Is64Bit() ? 16 : 8), offset_t(reserved_heap_size));
	header_region.AddField("Committed heap size", Dumper::HexDisplay::Make(Is64Bit() ? 16 : 8), offset_t(committed_heap_size));
	header_region.AddOptionalField("Loader flags (reserved)", Dumper::HexDisplay::Make(), offset_t(win32_version));
	header_region.AddField("Directory count", Dumper::HexDisplay::Make(), offset_t(data_directories.size()));
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

void PEFormat::Section::Dump(Dumper::Dumper& dump, const COFFFormat& format, unsigned section_index) const
{
	Dumper::Block section_block("Section", section_pointer, image->AsImage(), address, 8);
	section_block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(section_index + 1));
	section_block.AddField("Name", Dumper::StringDisplay::Make("\""), name);
	section_block.AddField("Size in memory", Dumper::HexDisplay::Make(), offset_t(virtual_size()));
	section_block.AddOptionalField("Line numbers", Dumper::HexDisplay::Make(), offset_t(line_number_pointer));
	section_block.AddOptionalField("Line numbers count", Dumper::DecDisplay::Make(), offset_t(line_number_count));
	static const std::map<offset_t, std::string> alignment_type =
	{
		{ 1,  "Align on 1-byte boundary" },
		{ 2,  "Align on 2-byte boundary" },
		{ 3,  "Align on 4-byte boundary" },
		{ 4,  "Align on 8-byte boundary" },
		{ 5,  "Align on 16-byte boundary" },
		{ 6,  "Align on 32-byte boundary" },
		{ 7,  "Align on 64-byte boundary" },
		{ 8,  "Align on 128-byte boundary" },
		{ 9,  "Align on 256-byte boundary" },
		{ 10, "Align on 512-byte boundary" },
		{ 11, "Align on 1024-byte boundary" },
		{ 12, "Align on 2048-byte boundary" },
		{ 13, "Align on 4096-byte boundary" },
		{ 14, "Align on 8192-byte boundary" },
	};
	section_block.AddOptionalField("Flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("no padding (obsolete)"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("contains executable code (text)"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("contains initialized data (data)"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("contains uninitialized data (bss)"), true)
			->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("LNK_OTHER (reserved)"), true)
			->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("comments or other information"), true)
			->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("remove from image"), true)
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("contains COMDAT data"), true)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("global pointer relative data"), true)
			->AddBitField(16, 1, Dumper::ChoiceDisplay::Make("MEM_PURGEABLE (reserved)"), true)
			->AddBitField(17, 1, Dumper::ChoiceDisplay::Make("MEM_16BIT (reserved)"), true)
			->AddBitField(18, 1, Dumper::ChoiceDisplay::Make("MEM_LOCKED (reserved)"), true)
			->AddBitField(19, 1, Dumper::ChoiceDisplay::Make("MEM_PRELOAD (reserved)"), true)
			->AddBitField(20, 4, Dumper::ChoiceDisplay::Make(alignment_type), true)
			->AddBitField(24, 1, Dumper::ChoiceDisplay::Make("contains extended relocations"), true)
			->AddBitField(25, 1, Dumper::ChoiceDisplay::Make("discardable"), true)
			->AddBitField(26, 1, Dumper::ChoiceDisplay::Make("cannot be cached"), true)
			->AddBitField(27, 1, Dumper::ChoiceDisplay::Make("not pageable"), true)
			->AddBitField(28, 1, Dumper::ChoiceDisplay::Make("can be in shared memory"), true)
			->AddBitField(29, 1, Dumper::ChoiceDisplay::Make("executable"), true)
			->AddBitField(30, 1, Dumper::ChoiceDisplay::Make("readable"), true)
			->AddBitField(31, 1, Dumper::ChoiceDisplay::Make("writable"), true),
		offset_t(flags));

#if 0
	if(relocation_count != 0)
	{
		Dumper::Region relocations("Section relocation", file_offset + relocation_pointer, 0, 8); /* TODO: size */
		section_block.AddOptionalField("Count", Dumper::DecDisplay::Make(), offset_t(relocation_count));
		relocations.Display(dump);
	}

	unsigned i = 0;
	for(auto& relocation : relocations)
	{
		Dumper::Entry relocation_entry("Relocation", i + 1, offset_t(-1) /* TODO: offset */, 8);
		relocation->FillEntry(relocation_entry);
		// TODO: fill addend
		relocation_entry.Display(dump);

		section_block.AddSignal(relocation->GetAddress() - address, relocation->GetSize());
		i++;
	}
#endif

	if(auto * pe = dynamic_cast<const PEFormat *>(&format))
	{
		for(auto block : pe->base_relocations->blocks_list)
		{
			if(address <= block->page_rva && block->page_rva < address + size)
			{
				for(auto rel : block->relocations_list)
				{
					uint32_t rva = block->page_rva + rel.offset;
					if(address <= rva && rva < address + size)
					{
						section_block.AddSignal(rva - address, rel.GetRelocationSize(pe));
					}
				}
			}
		}

		size_t entry_size = pe->Is64Bit() ? 8 : 4;

		for(auto& library : pe->imports->libraries)
		{
			if(address < library.address_table_rva + library.import_table.size() * entry_size && library.address_table_rva < address + size)
			{
				for(
					offset_t entry = std::max(offset_t(library.address_table_rva), offset_t(address & ~(entry_size - 1)));
					entry < std::min(address + size, library.address_table_rva + library.import_table.size() * entry_size);
					entry += entry_size)
				{
					section_block.AddSignal(entry - address, entry_size);
				}
			}
		}
	}

	section_block.Display(dump);
}

void PEFormat::Section::ReadSectionData(Linker::Reader& rd, const PEFormat& fmt)
{
	// unlike COFF (particularly for DJGPP), the section_pointer is from the start of the file
	rd.Seek(section_pointer);
	std::dynamic_pointer_cast<Linker::Buffer>(image)->ReadFile(rd, size);
}

void PEFormat::Section::WriteSectionData(Linker::Writer& wr, const PEFormat& fmt) const
{
	// unlike COFF (particularly for DJGPP), the section_pointer is from the start of the file
	wr.Seek(section_pointer);
	image->WriteFile(wr);
}

uint32_t PEFormat::Section::ImageSize(const PEFormat& fmt) const
{
	return image->ImageSize();
}

uint32_t PEFormat::Section::MemorySize(const PEFormat& fmt) const
{
	return std::dynamic_pointer_cast<Linker::Segment>(image)->TotalSize();
}

size_t PEFormat::Section::ReadData(size_t bytes, size_t offset, void * buffer) const
{
	return image->AsImage()->ReadData(bytes, offset, buffer);
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

void PEFormat::ResourcesSection::ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size)
{
	// TODO
}

void PEFormat::ResourcesSection::DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const
{
	// TODO
}

bool PEFormat::ImportsSection::IsPresent() const
{
	return libraries.size() > 0;
}

void PEFormat::ImportsSection::Generate(PEFormat& fmt)
{
	uint32_t rva = address;

	// import directory table
	rva += 20 * (libraries.size() + 1);

	// first list the import lookup tables
	for(auto& library : libraries)
	{
		library.lookup_table_rva = rva;
		rva += (library.import_table.size() + 1) * (fmt.Is64Bit() ? 8 : 4);
	}

	// then list the import address tables (identical formats)
	address_table_rva = rva;
	for(auto& library : libraries)
	{
		library.address_table_rva = rva;
		rva += (library.import_table.size() + 1) * (fmt.Is64Bit() ? 8 : 4);
	}
	address_table_size = rva - address_table_rva;

	// list all the hint/name pairs
	for(auto& library : libraries)
	{
		for(auto& import_entry : library.import_table)
		{
			if(auto import_name = std::get_if<ImportedLibrary::Name>(&import_entry))
			{
				import_name->rva = rva;
				rva = AlignTo(rva + 3 + import_name->name.size(), 2);
			}
		}
	}

	// list DLL names
	for(auto& library : libraries)
	{
		library.name_rva = rva;
		rva += library.name.size() + 1;
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

	for(auto& library : libraries)
	{
		wr.WriteWord(4, library.lookup_table_rva);
		wr.WriteWord(4, library.timestamp);
		wr.WriteWord(4, library.forwarder_chain);
		wr.WriteWord(4, library.name_rva);
		wr.WriteWord(4, library.address_table_rva);
	}

	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);
	wr.WriteWord(4, 0);

	// first list the import lookup tables
	for(auto& library : libraries)
	{
		wr.Seek(rva_to_offset + library.lookup_table_rva);
		for(auto& import_entry : library.import_table)
		{
			if(auto import_name = std::get_if<ImportedLibrary::Name>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, import_name->rva);
			}
			else if(auto import_id = std::get_if<ImportedLibrary::Ordinal>(&import_entry))
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
	for(auto& library : libraries)
	{
		wr.Seek(rva_to_offset + library.address_table_rva);
		for(auto& import_entry : library.import_table)
		{
			if(auto import_name = std::get_if<ImportedLibrary::Name>(&import_entry))
			{
				wr.WriteWord(fmt.Is64Bit() ? 8 : 4, import_name->rva);
			}
			else if(auto import_id = std::get_if<ImportedLibrary::Ordinal>(&import_entry))
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
	for(auto& library : libraries)
	{
		for(auto& import_entry : library.import_table)
		{
			if(auto import_name = std::get_if<ImportedLibrary::Name>(&import_entry))
			{
				wr.Seek(rva_to_offset + import_name->rva);
				wr.WriteWord(2, import_name->hint);
				wr.WriteData(import_name->name);
				wr.WriteWord(1, 0);
			}
		}
	}

	// list DLL names
	for(auto& library : libraries)
	{
		wr.Seek(rva_to_offset + library.name_rva);
		wr.WriteData(library.name);
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

void PEFormat::ImportsSection::ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size)
{
	size_t entry_size = fmt.Is64Bit() ? 8 : 4;
	uint32_t rva = directory_rva;
	uint32_t end = rva + directory_size;
	while(rva + 20 <= end)
	{
		{
			ImportedLibrary library("");
			library.lookup_table_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
			library.timestamp = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
			library.forwarder_chain = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
			library.name_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
			library.address_table_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
			if(
				library.lookup_table_rva == 0
				&& library.timestamp == 0
				&& library.forwarder_chain == 0
				&& library.name_rva == 0
				&& library.address_table_rva == 0)
			{
				break;
			}
			libraries.push_back(library);
		}

		ImportedLibrary& library = libraries.back();
		library.name = fmt.ReadASCII(library.name_rva, '\0');
		library_indexes[library.name] = libraries.size() - 1;

		// only read the import address table
		uint32_t ilt_rva = library.address_table_rva;
		while(true)
		{
			offset_t entry = fmt.ReadUnsigned(entry_size, ilt_rva, ::LittleEndian); ilt_rva += entry_size;
			if(entry == 0)
				break;
			if(((entry >> (entry_size * 8 - 1)) & 1))
			{
				// import by ordinal if bit 31/63 is set
				uint16_t ordinal = entry & 0xFFFF;
				library.import_table.emplace_back(ordinal);
				library.imports_by_ordinal[ordinal] = library.import_table.size() - 1;
			}
			else
			{
				ImportedLibrary::Name name("");
				name.rva = entry & 0x7FFFFFFF;
				name.hint = fmt.ReadUnsigned(2, name.rva, ::LittleEndian);
				name.name = fmt.ReadASCII(name.rva + 2, '\0');
				library.import_table.emplace_back(name);
				library.imports_by_name[name.name] = library.import_table.size() - 1;
			}
		}
	}
}

void PEFormat::ImportsSection::DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const
{
	size_t entry_size = fmt.Is64Bit() ? 8 : 4;

	Dumper::Region imports_region("Import table", fmt.RVAToFileOffset(directory_rva), directory_size, 8);
	imports_region.AddField("Address", Dumper::HexDisplay::Make(), offset_t(directory_rva));
	imports_region.Display(dump);

	uint32_t rva = directory_rva;
	size_t library_index = 0;
	for(auto library : libraries)
	{
		Dumper::Region library_region("Imported library", fmt.RVAToFileOffset(rva), 20, 8);
		library_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(library_index + 1));
		library_region.AddField("Address", Dumper::HexDisplay::Make(), offset_t(rva));
		library_region.AddField("Name", Dumper::StringDisplay::Make("\""), library.name);
		library_region.AddField("Name address", fmt.MakeRVADisplay(), offset_t(library.name_rva));
		library_region.AddOptionalField("Time stamp", Dumper::HexDisplay::Make(), offset_t(library.timestamp));
		library_region.AddOptionalField("Forwarder chain", Dumper::HexDisplay::Make(), offset_t(library.forwarder_chain));
		library_region.AddField("Lookup table", fmt.MakeRVADisplay(), offset_t(library.lookup_table_rva));
		library_region.AddField("Address table", fmt.MakeRVADisplay(), offset_t(library.address_table_rva));
		library_region.Display(dump);
		rva += 20;

		uint32_t entry_index = 0;
		for(auto& entry : library.import_table)
		{
			uint32_t entry_rva = library.address_table_rva + entry_index * entry_size;
			Dumper::Entry import_entry("Import", entry_index + 1, fmt.RVAToFileOffset(entry_rva), 8);
			import_entry.AddField("Address table entry (RVA)", Dumper::HexDisplay::Make(), offset_t(entry_rva));
			import_entry.AddField("Lookup table entry", fmt.MakeRVADisplay(), offset_t(library.lookup_table_rva + entry_index * entry_size));
			if(auto * ordinal = std::get_if<ImportedLibrary::Ordinal>(&entry))
			{
				import_entry.AddField("Type", Dumper::StringDisplay::Make(), std::string("by ordinal"));
				import_entry.AddField("Ordinal", Dumper::DecDisplay::Make(), offset_t(*ordinal));
			}
			else if(auto * name = std::get_if<ImportedLibrary::Name>(&entry))
			{
				import_entry.AddField("Type", Dumper::StringDisplay::Make(), std::string("by name"));
				import_entry.AddField("Name", Dumper::StringDisplay::Make(), name->name);
				import_entry.AddOptionalField("Hint", Dumper::DecDisplay::Make(), offset_t(name->hint));
				import_entry.AddField("Hint-name", fmt.MakeRVADisplay(), offset_t(name->rva));
			}
			import_entry.Display(dump);
			entry_index ++;
		}

		library_index ++;
	}
}

void PEFormat::ExportsSection::SetEntry(uint32_t ordinal, std::shared_ptr<ExportedEntry> entry)
{
	if(ordinal >= entries.size())
	{
		entries.resize(ordinal + 1);
	}

	if(entries[ordinal])
	{
		Linker::Error << "Error: overwriting entry " << ordinal << std::endl;
	}
	entries[ordinal] = std::make_optional<std::shared_ptr<ExportedEntry>>(entry);

	if(entry->name)
	{
		if(named_exports.find(entry->name.value().name) != named_exports.end())
		{
			Linker::Error << "Error: overwriting entry " << entry->name.value().name << std::endl;
		}
		named_exports[entry->name.value().name] = ordinal;
	}
}

void PEFormat::ExportsSection::AddEntry(std::shared_ptr<ExportedEntry> entry)
{
	assert(entry->name);

	if(named_exports.find(entry->name.value().name) != named_exports.end())
	{
		// already added
		return;
	}

	entries.push_back(entry);
	named_exports[entry->name.value().name] = entries.size() - 1;
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
			if(auto forwarder = std::get_if<ExportedEntry::Forwarder>(&entry_opt.value()->value))
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
			if(auto forwarder = std::get_if<ExportedEntry::Forwarder>(&entry_opt.value()->value))
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
		wr.WriteWord(2, uint16_t(named_export.second));
	}

	wr.Seek(rva_to_offset + dll_name_rva);
	wr.WriteData(dll_name);
	wr.WriteWord(1, 0);

	for(auto& entry_opt : entries)
	{
		if(entry_opt)
		{
			if(auto forwarder = std::get_if<ExportedEntry::Forwarder>(&entry_opt.value()->value))
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

void PEFormat::ExportsSection::ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size)
{
	uint32_t rva = directory_rva;
	flags = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	timestamp = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	version.major = fmt.ReadUnsigned(2, rva, ::LittleEndian); rva += 2;
	version.minor = fmt.ReadUnsigned(2, rva, ::LittleEndian); rva += 2;
	dll_name_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	dll_name = fmt.ReadASCII(dll_name_rva, '\0');
	ordinal_base = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	uint32_t entry_count = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	uint32_t name_count = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	address_table_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	name_table_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
	ordinal_table_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;

	for(uint32_t i = 0; i < entry_count; i++)
	{
		uint32_t export_rva = fmt.ReadUnsigned(4, address_table_rva + 4 * i, ::LittleEndian);
		if(export_rva == 0)
		{
			entries.push_back(std::optional<std::shared_ptr<ExportedEntry>>());
		}
		else if(directory_rva <= export_rva
		&& export_rva < directory_rva + directory_size)
		{
			// forwarder entry
			// TODO: test
			ExportedEntry::Forwarder forwarder = { };
			forwarder.rva = export_rva;
			forwarder.reference_name = fmt.ReadASCII(forwarder.rva, '\0');
			size_t ix = forwarder.reference_name.find('.');
			if(ix != std::string::npos)
			{
				forwarder.dll_name = forwarder.reference_name.substr(0, ix);
				if(ix + 1 < forwarder.reference_name.size() && forwarder.reference_name[ix + 1] == '#')
				{
					forwarder.reference = uint32_t(strtol(forwarder.reference_name.substr(ix + 2).c_str(), NULL, 10));
				}
				else
				{
					forwarder.reference = forwarder.reference_name.substr(ix + 1);
				}
			}
			std::shared_ptr<ExportedEntry> entry = std::make_shared<ExportedEntry>(forwarder);
			entries.push_back(entry);
		}
		else
		{
			std::shared_ptr<ExportedEntry> entry = std::make_shared<ExportedEntry>(export_rva);
			entries.push_back(entry);
		}
	}

	for(uint32_t i = 0; i < name_count; i++)
	{
		uint16_t ordinal = fmt.ReadUnsigned(2, ordinal_table_rva + 2 * i, ::LittleEndian);
		if(ordinal >= entries.size())
		{
			Linker::Error << "Error: export ordinal " << std::dec << ordinal << " is outside export table" << std::endl;
			continue;
		}
		ExportedEntry::Name name("");
		name.rva = fmt.ReadUnsigned(4, name_table_rva + 4 * i, ::LittleEndian);
		name.name = fmt.ReadASCII(name.rva, '\0');
		if(!entries[ordinal])
		{
			Linker::Error << "Error: export ordinal " << std::dec << ordinal << " points to unused entry" << std::endl;
			continue;
		}
		entries[ordinal].value()->name = name;
		named_exports[name.name] = ordinal;
	}
}

void PEFormat::ExportsSection::DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const
{
	Dumper::Region exports_region("Export table", fmt.RVAToFileOffset(directory_rva), directory_size, 8);
	exports_region.AddField("Address", Dumper::HexDisplay::Make(), offset_t(directory_rva));
	exports_region.AddOptionalField("Flags", Dumper::HexDisplay::Make(), offset_t(flags));
	exports_region.AddField("Timestamp", Dumper::HexDisplay::Make(), offset_t(timestamp));
	exports_region.AddField("Version", Dumper::VersionDisplay::Make(), offset_t(version.major), offset_t(version.minor));
	exports_region.AddField("DLL name", Dumper::StringDisplay::Make("\""), dll_name);
	exports_region.AddField("DLL name address", fmt.MakeRVADisplay(), offset_t(dll_name_rva));
	exports_region.AddField("Address table", fmt.MakeRVADisplay(), offset_t(address_table_rva));
	exports_region.AddField("Name pointer table", fmt.MakeRVADisplay(), offset_t(name_table_rva));
	exports_region.AddField("Ordinal table", fmt.MakeRVADisplay(), offset_t(ordinal_table_rva));
	exports_region.Display(dump);

	size_t i = 0;
	for(auto& entry : entries)
	{
		Dumper::Entry export_entry("Entry", i, fmt.RVAToFileOffset(address_table_rva + i * 4), 8);
		export_entry.AddField("Entry (RVA)", Dumper::HexDisplay::Make(), offset_t(address_table_rva + i * 4));
		export_entry.AddField("Ordinal", Dumper::DecDisplay::Make(), offset_t(ordinal_base + i));
		if(!entry)
		{
			export_entry.AddField("Type", Dumper::StringDisplay::Make(), std::string("unused"));
		}
		else if(auto * forwarder = std::get_if<ExportedEntry::Forwarder>(&entry.value()->value))
		{
			// TODO: test
			export_entry.AddField("Type", Dumper::StringDisplay::Make(), std::string("forwarder"));
			export_entry.AddField("Library", Dumper::StringDisplay::Make("\""), forwarder->dll_name);
			if(auto * ordinal = std::get_if<uint32_t>(&forwarder->reference))
			{
				export_entry.AddField("Ordinal", Dumper::DecDisplay::Make(), offset_t(*ordinal));
			}
			else if(auto * name = std::get_if<std::string>(&forwarder->reference))
			{
				export_entry.AddField("Name", Dumper::StringDisplay::Make(), *name);
			}
		}
		else if(auto * value = std::get_if<uint32_t>(&entry.value()->value))
		{
			export_entry.AddField("Type", Dumper::StringDisplay::Make(), std::string("exported"));
			export_entry.AddField("Address", fmt.MakeRVADisplay(), offset_t(*value));
			if(entry.value()->name)
			{
				export_entry.AddField("Name", Dumper::StringDisplay::Make(), entry.value()->name.value().name);
				export_entry.AddField("Name address", fmt.MakeRVADisplay(), offset_t(entry.value()->name.value().rva));
			}
		}
		export_entry.Display(dump);
		i ++;
	}
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
	return blocks_map.size() > 0;
}

void PEFormat::BaseRelocationsSection::Generate(PEFormat& fmt)
{
	uint32_t full_size = 0;
	blocks_list.clear();
	for(auto pair : blocks_map)
	{
		blocks_list.push_back(pair.second);
	}

	for(auto block : blocks_list)
	{
		block->block_size = 8;
		for(auto pair : block->relocations_map)
		{
			Linker::Debug << "Debug: Relocation at " << std::hex << pair.second.offset << std::endl;
			block->relocations_list.push_back(pair.second);
			block->block_size += 2 * pair.second.GetEntryCount(&fmt);
		}
		Linker::Debug << "Debug: Block size " << std::hex << block->block_size << std::endl;
		full_size = AlignTo(full_size, 4) + block->block_size;
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
	for(auto block : blocks_list)
	{
		offset = AlignTo(offset, 4);
		wr.Seek(offset);
		wr.WriteWord(4, block->page_rva);
		wr.WriteWord(4, block->block_size);

		for(auto relocation : block->relocations_list)
		{
			wr.WriteWord(2, (relocation.type << 12) | (relocation.offset));
			if(relocation.GetEntryCount(&fmt) >= 2)
			{
				wr.WriteWord(2, relocation.parameter);
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

void PEFormat::BaseRelocationsSection::ParseDirectoryData(const PEFormat& fmt, uint32_t directory_rva, uint32_t directory_size)
{
	uint32_t rva = directory_rva;
	uint32_t end = directory_rva + directory_size;
	while(rva < end)
	{
		std::shared_ptr<BaseRelocationBlock> block = std::make_shared<BaseRelocationBlock>();
		blocks_list.push_back(block);
		block->page_rva = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
		blocks_map[block->page_rva] = block;
		block->block_size = fmt.ReadUnsigned(4, rva, ::LittleEndian); rva += 4;
		if(block->block_size < 8)
			continue;
		uint32_t block_end = rva + block->block_size - 8;
		if(block_end > end)
			block_end = end; // do not read beyond the directory
		while(rva < block_end)
		{
			BaseRelocation reloc = { };
			uint16_t entry = fmt.ReadUnsigned(2, rva, ::LittleEndian); rva += 2;
			reloc.type = BaseRelocation::relocation_type(entry >> 12);
			reloc.offset = entry & 0x0FFF;
			if(reloc.GetEntryCount(&fmt) >= 2)
			{
				reloc.parameter = fmt.ReadUnsigned(2, rva, ::LittleEndian); rva += 2;
			}
			block->relocations_list.push_back(reloc);
			block->relocations_map[reloc.offset] = reloc;
		}
	}
}

void PEFormat::BaseRelocationsSection::DumpDirectory(const PEFormat& fmt, Dumper::Dumper& dump, uint32_t directory_rva, uint32_t directory_size) const
{
	uint32_t rva = directory_rva;
	for(auto block : blocks_list)
	{
		Dumper::Region block_region("Block", fmt.RVAToFileOffset(rva), block->block_size, 8);
		block_region.AddField("Location", Dumper::HexDisplay::Make(), fmt.RVAToFileOffset(rva));
		block_region.AddField("Page (RVA)", Dumper::HexDisplay::Make(), offset_t(block->page_rva));
		block_region.Display(dump);

		rva += 8;

		size_t i = 0;
		for(auto rel : block->relocations_list)
		{
			Dumper::Entry relocation_entry("Relocation", i + 1, fmt.RVAToFileOffset(rva), 8);
			static const std::map<offset_t, std::string> reloc_type =
			{
				{ 0,  "skipped (ABSOLUTE)" },
				{ 1,  "high 16-bit (HIGH)" },
				{ 2,  "16-bit (LOW)" },
				{ 3,  "32-bit (HIGHLOW)" },
				{ 4,  "adjusted high 16-bit (HIGHADJ)" },
				{ 5,  "MIPS: jump address; ARM: movw/movt; RISC-V: high 20" },
				{ 7,  "Thumb (ARM): movw/movt; RISC-V: low 12 I-type" },
				{ 8,  "RISC-V: low 12 S-type; LoongArch: la address" },
				{ 9,  "MIPS16: jump address" },
				{ 10, "64-bit (DIR64)" },
			};
			relocation_entry.AddField("Type", Dumper::ChoiceDisplay::Make(reloc_type, Dumper::DecDisplay::Make(1)), offset_t(rel.type));
			relocation_entry.AddField("Address (RVA)", Dumper::HexDisplay::Make(), offset_t(block->page_rva + rel.offset));
			relocation_entry.AddOptionalField("Parameter", Dumper::HexDisplay::Make(4), offset_t(rel.parameter));
			relocation_entry.AddField("Location", Dumper::HexDisplay::Make(), fmt.RVAToFileOffset(rva));
			relocation_entry.Display(dump);
			i ++;
			rva += rel.GetEntryCount(&fmt) * 2;
		}
	}
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

offset_t PEFormat::RVAToFileOffset(uint32_t rva) const
{
	std::shared_ptr<Section> section;
	size_t offset;
	if(MapRVAToSectionData(rva, 1, section, offset) != 0)
		return section->section_pointer + offset;
	else
		return 0;
}

size_t PEFormat::MapRVAToSectionData(uint32_t rva, size_t bytes, std::shared_ptr<Section>& found_section, size_t& section_offset) const
{
	for(auto section : Sections())
	{
		if(section->address <= rva && rva < section->address + section->size)
		{
			found_section = std::const_pointer_cast<Section>(section);
			section_offset = rva - section->address;
			return std::min(bytes, section->address + section->size - rva);
		}
	}
	return 0;
}

bool PEFormat::PESections::iterator::operator !=(const PEFormat::PESections::iterator& other) const
{
	return coff_iterator != other.coff_iterator;
}

PEFormat::PESections::iterator& PEFormat::PESections::iterator::operator++()
{
	coff_iterator++;
	return *this;
}

std::shared_ptr<PEFormat::Section> PEFormat::PESections::iterator::operator*() const
{
	return std::dynamic_pointer_cast<PEFormat::Section>(*coff_iterator);
}

bool PEFormat::PESections::const_iterator::operator !=(const PEFormat::PESections::const_iterator& other) const
{
	return coff_iterator != other.coff_iterator;
}

PEFormat::Section * PEFormat::PESections::const_reference::get() const
{
	return std::dynamic_pointer_cast<PEFormat::Section>(coff_reference).get();
}

PEFormat::Section * PEFormat::PESections::const_reference::operator->() const
{
	return get();
}

PEFormat::PESections::const_iterator& PEFormat::PESections::const_iterator::operator++()
{
	coff_iterator++;
	return *this;
}

std::shared_ptr<const PEFormat::Section> PEFormat::PESections::const_iterator::operator*() const
{
	return std::dynamic_pointer_cast<const PEFormat::Section>(*coff_iterator);
}

PEFormat::PESections::iterator PEFormat::PESections::begin()
{
	return iterator{const_cast<PEFormat *>(pe)->sections.begin()};
}

PEFormat::PESections::const_iterator PEFormat::PESections::begin() const
{
	return const_iterator{pe->sections.begin()};
}

PEFormat::PESections::iterator PEFormat::PESections::end()
{
	return iterator{const_cast<PEFormat *>(pe)->sections.end()};
}

PEFormat::PESections::const_iterator PEFormat::PESections::end() const
{
	return const_iterator{pe->sections.end()};
}

size_t PEFormat::PESections::size() const
{
	return pe->sections.size();
}

PEFormat::PESections::const_reference PEFormat::PESections::back() const
{
	return const_reference{pe->sections.back()};
}

const PEFormat::PESections PEFormat::Sections() const
{
	return PESections{this};
}

PEFormat::PESections PEFormat::Sections()
{
	return const_cast<const PEFormat *>(this)->Sections();
}

size_t PEFormat::ReadData(size_t bytes, uint32_t rva, void * buffer) const
{
	size_t count = 0;
	while(bytes > 0)
	{
		std::shared_ptr<Section> section;
		size_t offset;
		size_t available_bytes = MapRVAToSectionData(rva, bytes, section, offset);
		if(available_bytes == 0)
			break;

		size_t result =
			section->ReadData(available_bytes, offset, buffer);
		assert(result == available_bytes);
		bytes -= available_bytes;
		rva += available_bytes;
		buffer = reinterpret_cast<char *>(buffer) + available_bytes;
		count += available_bytes;
	}
	return count;
}

uint64_t PEFormat::ReadUnsigned(size_t bytes, uint32_t rva, ::EndianType endiantype) const
{
	assert(bytes <= 8);
	std::vector<uint8_t> data(bytes);
	ReadData(bytes, rva, data.data());
	return ::ReadUnsigned(bytes, bytes, data.data(), endiantype);
}

uint64_t PEFormat::ReadSigned(size_t bytes, uint32_t rva, ::EndianType endiantype) const
{
	assert(bytes <= 8);
	std::vector<uint8_t> data(bytes);
	ReadData(bytes, rva, data.data());
	return ::ReadSigned(bytes, bytes, data.data(), endiantype);
}

std::string PEFormat::ReadASCII(uint32_t rva, char terminator, size_t maximum) const
{
	std::string tmp;
	char c = 0;
	while(ReadData(1, rva++, &c) != 0 && c != terminator)
	{
		tmp += c;
	}
	return tmp;
}

void PEFormat::AddBaseRelocation(uint32_t rva, BaseRelocation::relocation_type type, uint16_t low_ref)
{
	uint32_t page_rva = rva & ~(BaseRelocationBlock::PAGE_SIZE - 1);
	if(base_relocations->blocks_map.find(page_rva) == base_relocations->blocks_map.end())
	{
		base_relocations->blocks_map[page_rva] = std::make_shared<BaseRelocationBlock>(page_rva);
	}
	uint16_t page_offset = rva & (BaseRelocationBlock::PAGE_SIZE - 1);
	if(base_relocations->blocks_map.find(page_offset) != base_relocations->blocks_map.end())
	{
		Linker::Error << "Error: duplicate relocation for address " << std::hex << rva << std::endl;
	}
	BaseRelocation rel(type, page_offset);
	if(rel.GetEntryCount(this) >= 2)
	{
		// we will store low_ref in the following relocation entry
		rel.parameter = low_ref;
	}
	base_relocations->blocks_map[page_rva]->relocations_map[page_offset] = rel;
}

void PEFormat::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	rd.ReadData(4, pe_signature);
	ReadCOFFHeader(rd);
	optional_header->ReadFile(rd);
	ReadRestOfFile(rd);

	auto& optional_header = GetOptionalHeader();
	for(size_t directory_number = 0; directory_number < optional_header.data_directories.size(); directory_number++)
	{
		auto& data_directory = optional_header.data_directories[directory_number];
		switch(directory_number)
		{
		case PEOptionalHeader::DirExportTable:
			if(data_directory.size != 0)
			{
				exports->ParseDirectoryData(*this, data_directory.address, data_directory.size);
			}
			break;
		case PEOptionalHeader::DirImportTable:
			if(data_directory.size != 0)
			{
				imports->ParseDirectoryData(*this, data_directory.address, data_directory.size);
			}
			break;
		case PEOptionalHeader::DirResourceTable:
			// TODO
			break;
		//case PEOptionalHeader::DirExceptionTable:
		//case PEOptionalHeader::DirCertificateTable:
		case PEOptionalHeader::DirBaseRelocationTable:
			if(data_directory.size != 0)
			{
				base_relocations->ParseDirectoryData(*this, data_directory.address, data_directory.size);
			}
			break;
		//case PEOptionalHeader::DirDebug:
		//case PEOptionalHeader::DirGlobalPointer:
		//case PEOptionalHeader::DirTLSTable:
		//case PEOptionalHeader::DirLoadConfigTable:
		//case PEOptionalHeader::DirBoundImport:
		//case PEOptionalHeader::DirIAT:
		//case PEOptionalHeader::DirDelayImportDescriptor:
		//case PEOptionalHeader::DirCLRRuntimeHeader:
		}
	}
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

	for(auto section : Sections())
	{
		section->virtual_size() = section->MemorySize(*this);
		section->size = section->ImageSize(*this);
		section->section_pointer = offset = AlignTo(offset, GetOptionalHeader().file_align);
		offset += section->size;
	}

	optional_header->CalculateValues(*this);
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
	dump.SetEncoding(Dumper::Block::encoding_windows1252);

	dump.SetTitle("PE format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
#if 0
	static const std::map<offset_t, std::string> endian_descriptions =
	{
		{ ::LittleEndian, "little endian" },
		{ ::BigEndian,    "big endian" },
	};
	file_region.AddField("Byte order", Dumper::ChoiceDisplay::Make(endian_descriptions), offset_t(endiantype));
#endif
	file_region.Display(dump);

	Dumper::Region header_region("File header", file_offset, 20, 8);
	static const std::map<offset_t, std::string> cpu_descriptions =
	{
		{ 0x014C, "Intel 80386 (32-bit x86)" },
		{ 0x014D, "Intel i860 or Intel 80486 (obsolete)" },
		{ 0x014E, "Intel Pentium (obsolete)" },
		{ 0x0160, "MIPS I (32-bit, big endian)" },
		{ 0x0162, "MIPS I (32-bit, little endian)" },
		{ 0x0163, "MIPS II" },
		{ 0x0166, "MIPS III (64-bit, little endian)" },
		{ 0x0168, "MIPS IV (64-bit, little endian)" },
		{ 0x0169, "MIPS (Windows CE 2.0, little endian)" },
		{ 0x0183, "Alpha (obsolete)" },
		{ 0x0184, "Alpha (32-bit address space)" },
		{ 0x01A2, "Hitachi SH3" },
		{ 0x01A3, "Hitachi SH3 DSP" },
		{ 0x01A6, "Hitachi SH4" },
		{ 0x01A8, "Hitachi SH5" },
		{ 0x01C0, "ARM (32-bit, little endian)" },
		{ 0x01C2, "ARM Thumb" },
		{ 0x01C4, "ARM Thumb-2 (little endian)" },
		{ 0x01D3, "Matsushita AM33" },
		{ 0x01F0, "PowerPC (little endian)" },
		{ 0x01F1, "PowerPC with FPU (little endian)" },
		{ 0x0200, "Intel Itanium" },
		{ 0x0266, "MIPS16" },
		{ 0x0268, "Motorola 68000" },
		{ 0x0284, "Alpha (64-bit address space)" },
		{ 0x0290, "HP PA-RISC" },
		{ 0x0366, "MIPS with FPU" },
		{ 0x0466, "MIPS16 with FPU" },
		{ 0x0500, "Hitachi SH (big endian)" },
		{ 0x0550, "Hitachi SH (little endian)" },
		{ 0x0601, "PowerPC based Macintosh" },
		{ 0x0EBC, "EFI byte code" },
		{ 0x5032, "RISC-V (32-bit address space)" },
		{ 0x5064, "RISC-V (64-bit address space)" },
		{ 0x5128, "RISC-V (128-bit address space)" },
		{ 0x6232, "LoongArch 32-bit" },
		{ 0x6264, "LoongArch 64-bit" },
		{ 0x8664, "AMD64 (64-bit x86)" },
		{ 0x9041, "Mitsubishi M32R (little endian)" },
		{ 0xA641, "ARM64EC (AArch64 and AMD64 interoperability)" },
		{ 0xA64E, "ARM64X (ARM64 and ARM64EC coexists)" },
		{ 0xAA64, "AArch64 (64-bit ARM, little endian)" },
	};
	header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(cpu_descriptions, Dumper::HexDisplay::Make(4)), offset_t(::ReadUnsigned(2, 2, reinterpret_cast<const uint8_t *>(signature), endiantype)));
	header_region.AddOptionalField("Time stamp", Dumper::HexDisplay::Make(), offset_t(timestamp));
	// TODO: other fields?
	header_region.AddOptionalField("Flags",
		Dumper::BitFieldDisplay::Make()
			->AddBitField(0,  1, Dumper::ChoiceDisplay::Make("no base relocations"), true)
			->AddBitField(1,  1, Dumper::ChoiceDisplay::Make("executable"), true)
			->AddBitField(2,  1, Dumper::ChoiceDisplay::Make("no COFF line numbers (deprecated)"), true)
			->AddBitField(3,  1, Dumper::ChoiceDisplay::Make("no COFF symbols (deprecated)"), true)
			->AddBitField(4,  1, Dumper::ChoiceDisplay::Make("aggressively trim working set (deprecated)"), true)
			->AddBitField(5,  1, Dumper::ChoiceDisplay::Make("handles large (more than 2 GiB) address space"), true)
			->AddBitField(7,  1, Dumper::ChoiceDisplay::Make("little endian (obsolete)"), true)
			->AddBitField(8,  1, Dumper::ChoiceDisplay::Make("32-bit"), true)
			->AddBitField(9,  1, Dumper::ChoiceDisplay::Make("no debug information"), true)
			->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("copy to swap file if on removable media"), true)
			->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("copy to swap file if on network"), true)
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("system file"), true)
			->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("dynamic-link library (DLL)"), true)
			->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("uniprocessor system only"), true)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("big endian (obsolete)"), true),
		offset_t(flags));
	header_region.Display(dump);

	if(optional_header)
	{
		optional_header->Dump(*this, dump);

		size_t directory_number = 0;
		for(auto& data_directory : GetOptionalHeader().data_directories)
		{
			static const std::map<offset_t, std::string> directory_type =
			{
				{ 1,  "Export table (.edata)" },
				{ 2,  "Import table (.idata)" },
				{ 3,  "Resource table (.rsrc)" },
				{ 4,  "Exception table (.pdata)" },
				{ 5,  "Certificate table" },
				{ 6,  "Base relocation table (.reloc)" },
				{ 7,  "Debug data (.debug)" },
				{ 8,  "Architecture (reserved)" },
				{ 9,  "Global pointer" },
				{ 10, "Thread local storage table (.tls)" },
				{ 11, "Load config table" },
				{ 12, "Bound import table" },
				{ 13, "Import address table" },
				{ 14, "Delay import descriptor" },
				{ 15, "CLR runtime header" },
				{ 16, "(reserved)" },
			};

			Dumper::Region directory_region("Directory", RVAToFileOffset(data_directory.address), data_directory.size, 8);
			directory_region.InsertField(0, "Type", Dumper::ChoiceDisplay::Make(directory_type, Dumper::DecDisplay::Make()), offset_t(directory_number + 1));
			directory_region.AddField("Address (RVA)", Dumper::HexDisplay::Make(), offset_t(data_directory.address));
			directory_region.Display(dump);

			switch(directory_number)
			{
			case PEOptionalHeader::DirExportTable:
				if(data_directory.size != 0)
				{
					exports->DumpDirectory(*this, dump, data_directory.address, data_directory.size);
				}
				break;
			case PEOptionalHeader::DirImportTable:
				if(data_directory.size != 0)
				{
					imports->DumpDirectory(*this, dump, data_directory.address, data_directory.size);
				}
				break;
			case PEOptionalHeader::DirResourceTable:
				// TODO
				break;
			//case PEOptionalHeader::DirExceptionTable:
			//case PEOptionalHeader::DirCertificateTable:
			case PEOptionalHeader::DirBaseRelocationTable:
				if(data_directory.size != 0)
				{
					base_relocations->DumpDirectory(*this, dump, data_directory.address, data_directory.size);
				}
				break;
			//case PEOptionalHeader::DirDebug:
			//case PEOptionalHeader::DirGlobalPointer:
			//case PEOptionalHeader::DirTLSTable:
			//case PEOptionalHeader::DirLoadConfigTable:
			//case PEOptionalHeader::DirBoundImport:
			//case PEOptionalHeader::DirIAT:
			//case PEOptionalHeader::DirDelayImportDescriptor:
			//case PEOptionalHeader::DirCLRRuntimeHeader:
			}

			directory_number ++;
		}
	}

	unsigned section_index = 0;
	for(auto section : Sections())
	{
		section->Dump(dump, *this, section_index);
		section_index ++;
	}

#if 0
	Dumper::Region symbol_table("Symbol table", file_offset + symbol_table_offset, symbol_count * 18, 8);
	symbol_table.AddField("Count", Dumper::DecDisplay::Make(), offset_t(symbol_count));
	symbol_table.Display(dump);
	unsigned i = 0;
	for(auto& symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i + 1, file_offset + symbol_table_offset + 18 * i, 8);
		if(symbol)
		{
			/* otherwise, ignored symbol */
			symbol_entry.AddField("Name", Dumper::StringDisplay::Make(), symbol->name);
			symbol_entry.AddOptionalField("Name index", Dumper::HexDisplay::Make(), offset_t(symbol->name_index));
			symbol_entry.AddField("Value", Dumper::HexDisplay::Make(), offset_t(symbol->value));
			symbol_entry.AddField("Section", Dumper::HexDisplay::Make(), offset_t(symbol->section_number));
			symbol_entry.AddField("Type", Dumper::HexDisplay::Make(4), offset_t(symbol->type));
			symbol_entry.AddField("Storage class", Dumper::HexDisplay::Make(2), offset_t(symbol->storage_class));
			symbol_entry.AddOptionalField("Auxiliary count", Dumper::DecDisplay::Make(), offset_t(symbol->auxiliary_count));
		}
		symbol_entry.Display(dump);
		i ++;
	}
#endif
	// TODO
}

/* * * Writer members * * */

std::shared_ptr<PEFormat> PEFormat::CreateConsoleApplication(target_type target, PEOptionalHeader::SubsystemType subsystem)
{
	// TODO: use subsystem, but only if it has a CUI variant
	auto fmt = std::make_shared<PEFormat>(target, PEOptionalHeader::WindowsCUI, OUTPUT_EXE);
	return fmt;
}

std::shared_ptr<PEFormat> PEFormat::CreateGUIApplication(target_type target, PEOptionalHeader::SubsystemType subsystem)
{
	// TODO: use subsystem, but only if it has a GUI variant
	auto fmt = std::make_shared<PEFormat>(target, PEOptionalHeader::WindowsGUI, OUTPUT_EXE);
	return fmt;
}

std::shared_ptr<PEFormat> PEFormat::CreateLibraryModule(target_type target)
{
	auto fmt = std::make_shared<PEFormat>(target, PEOptionalHeader::WindowsGUI, OUTPUT_DLL); // TODO: GUI or CUI?
	return fmt;
}

std::shared_ptr<PEFormat> PEFormat::CreateDeviceDriver(target_type target)
{
	auto fmt = std::make_shared<PEFormat>(target, PEOptionalHeader::Native, OUTPUT_SYS);
	return fmt;
}

std::shared_ptr<PEFormat> PEFormat::SimulateLinker(compatibility_type compatibility)
{
	this->compatibility = compatibility;
	switch(compatibility)
	{
	case CompatibleNone:
		break;
	case CompatibleWatcom:
		/* TODO */
		break;
	case CompatibleMicrosoft:
		/* TODO */
		break;
	case CompatibleBorland:
		/* TODO */
		break;
	case CompatibleGNU:
		/* TODO */
		break;
	}
	return std::dynamic_pointer_cast<PEFormat>(shared_from_this());
}

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

PEFormat::ImportedLibrary& PEFormat::FetchImportLibrary(std::string library_name, bool create_if_not_present)
{
	auto imported_library_reference = imports->library_indexes.find(library_name);
	if(imported_library_reference != imports->library_indexes.end())
	{
		return imports->libraries[imported_library_reference->second];
	}
	else if(create_if_not_present)
	{
		imports->libraries.push_back(ImportedLibrary(library_name));
		imports->library_indexes[library_name] = imports->libraries.size() - 1;
		return imports->libraries.back();
	}
	else
	{
		Linker::FatalError("Internal error: library " + library_name + " not included");
	}
}

void PEFormat::ImportedLibrary::AddImportByName(std::string entry_name, uint16_t hint)
{
	if(imports_by_name.find(entry_name) != imports_by_name.end())
		return;

	import_table.push_back(ImportedLibrary::Name(entry_name, hint));

	imports_by_name[entry_name] = import_table.size() - 1;
}

void PEFormat::ImportedLibrary::AddImportByOrdinal(uint16_t ordinal)
{
	if(imports_by_ordinal.find(ordinal) != imports_by_ordinal.end())
		return;

	ImportedLibrary::ImportTableEntry entry = ImportedLibrary::Ordinal(ordinal);
	import_table.push_back(entry);

	imports_by_ordinal[ordinal] = import_table.size() - 1;
}

offset_t PEFormat::ImportedLibrary::GetImportByNameAddress(const PEFormat& fmt, std::string name)
{
	auto import_pointer = imports_by_name.find(name);
	if(import_pointer == imports_by_name.end())
	{
		Linker::FatalError("Internal error: imported name " + name + " not included");
	}

	return fmt.RVAToAddress(address_table_rva + import_pointer->second * (fmt.Is64Bit() ? 8 : 4));
}

offset_t PEFormat::ImportedLibrary::GetImportByOrdinalAddress(const PEFormat& fmt, uint16_t ordinal)
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

	if(collector.compat())
	{
		compatibility = collector.compat();
	}

	output = collector.output();

	// TODO: DLL flags and similar

	if(collector.image_base())
	{
		GetOptionalHeader().image_base = collector.image_base();
	}
	else
	{
		// it is not known at this point whether this is 64-bit or not, so we set it to 0
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

	option_import_thunks = collector.import_thunks();
	if(compatibility != CompatibleNone)
	{
		option_import_thunks = true;
	}

	/* TODO */
}

void PEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	auto first_section = segment->sections[0];
	if(first_section->IsExecutable())
	{
		Linker::Debug << "Debug: PE code section: " << segment->name << std::endl;
		for(auto section : segment->sections)
		{
			Linker::Debug << "Debug: - containing " << section->name << std::endl;
		}
		sections.push_back(std::make_shared<Section>(Section::TEXT | Section::EXECUTE | Section::READ, segment));
	}
	else if(!first_section->IsZeroFilled())
	{
		Linker::Debug << "Debug: PE data section: " << segment->name << std::endl;
		for(auto section : segment->sections)
		{
			Linker::Debug << "Debug: - containing " << section->name << std::endl;
		}
		uint32_t flags = Section::DATA | Section::READ;
		for(auto section : segment->sections)
		{
			if(section->IsWritable())
			{
				flags |= Section::WRITE;
				break;
			}
		}
		sections.push_back(std::make_shared<Section>(flags, segment));
	}
	else
	{
		uint32_t flags = Section::BSS | Section::READ | Section::WRITE;
		if(compatibility == CompatibleWatcom)
		{
			segment->Fill();
			flags = Section::DATA | Section::READ | Section::WRITE;
		}

		Linker::Debug << "Debug: PE bss section: " << segment->name << std::endl;
		for(auto section : segment->sections)
		{
			Linker::Debug << "Debug: - containing " << section->name << std::endl;
		}
		sections.push_back(std::make_shared<Section>(flags, segment));
	}
}

void PEFormat::OnCallDirective(std::string identifier)
{
	if(identifier == "GenerateImportThunks")
	{
		if(option_import_thunks)
		{
			assert(import_thunk_segment == nullptr);
			std::shared_ptr<Section> text_section;

			bool need_new_section = false;

			if(Sections().size() == 0)
			{
				need_new_section = true;
			}
			else
			{
				switch(compatibility)
				{
				case CompatibleNone:
				default:
					text_section = std::dynamic_pointer_cast<PEFormat::Section>(sections.back());
					break;
				case CompatibleWatcom:
					text_section = std::dynamic_pointer_cast<PEFormat::Section>(sections.back());
					if(text_section->name != "AUTO") // the actual condition seems to be that the last section is not part of a group
						need_new_section = true;
					break;
				case CompatibleGNU:
					text_section = *Sections().begin();
					break;
				}
			}

			if(need_new_section)
			{
				std::string dummy_segment_name;
				switch(compatibility)
				{
				case CompatibleNone:
					dummy_segment_name = ".code";
					break;
				case CompatibleWatcom:
					dummy_segment_name = "AUTO";
					break;
				default:
				case CompatibleGNU:
					dummy_segment_name = ".text";
					break;
				}
				import_thunk_segment = std::make_shared<Linker::Segment>(dummy_segment_name);
				import_thunk_segment->base_address = GetMemoryImageEnd() - GetOptionalHeader().image_base;
				import_thunk_segment->Append(std::make_shared<Linker::Section>(dummy_segment_name, Linker::Section::Readable | Linker::Section::Executable));
				text_section = std::make_shared<Section>(Section::TEXT | Section::EXECUTE | Section::READ, import_thunk_segment);
				sections.push_back(text_section);
				SegmentManager::AppendSegment(import_thunk_segment);
				Linker::Debug << "Debug: Create new section " << dummy_segment_name << " for import thunks" << std::endl;
			}
			else
			{
				if(!(text_section->flags & Section::TEXT))
				{
					Linker::FatalError("Fatal error: in linker script, GenerateImportThunks must be called before data sections");
				}
				Linker::Debug << "Debug: Using existing section " << text_section->name << " for import thunks" << std::endl;
				import_thunk_segment = std::dynamic_pointer_cast<Linker::Segment>(text_section->image);
			}

			// generate indirect calls to all the imported symbols
			// TODO: this works for imported procedures, but how do we handle imported values?

			if(current_module->GetImportedSymbols().size() > 0)
			{
				import_thunk_segment->Fill();
			}

			if(compatibility == CompatibleGNU)
			{
				unsigned misalign = import_thunk_segment->data_size & 3;
				switch(cpu_type)
				{
				case CPU_I386:
					switch(misalign)
					{
					case 1:
						import_thunk_segment->WriteData(3, import_thunk_segment->data_size, "\x66\x90\x90");
						break;
					case 2:
						import_thunk_segment->WriteData(2, import_thunk_segment->data_size, "\x66\x90");
						break;
					case 3:
						import_thunk_segment->WriteData(3, import_thunk_segment->data_size, "\x90");
						break;
					}
					break;
				default:
					// TODO: other CPUs
					break;
				}
			}

			for(const Linker::SymbolName& symbol : current_module->GetImportedSymbols())
			{
				assert(import_thunk_segment->zero_fill == 0);

				std::string library;
				if(symbol.LoadLibraryName(library))
				{
					uint16_t ordinal;
					std::string name;

					if(symbol.LoadName(name))
					{
						import_thunks_by_name[std::make_pair(library, name)] = import_thunk_segment->data_size;
					}
					else if(symbol.LoadOrdinalOrHint(ordinal))
					{
						import_thunks_by_ordinal[std::make_pair(library, ordinal)] = import_thunk_segment->data_size;
					}
				}

				switch(cpu_type)
				{
				case CPU_I386:
				case CPU_AMD64:
					// jmp [abs32] (32-bit) or jmp [rel32] (64-bit)
					// we fill in the actual address later
					import_thunk_segment->WriteData(6, import_thunk_segment->data_size, "\xFF\x25\0\0\0\0");
					if(compatibility == CompatibleGNU && cpu_type == CPU_I386)
					{
						// nop; nop
						import_thunk_segment->WriteData(2, import_thunk_segment->data_size, "\x90\x90");
					}
					break;
				case CPU_ARM:
					// TODO: test
					// ldr ip, [pc]
					// ldr pc, [ip]
					// .word abs32
					import_thunk_segment->WriteData(12, import_thunk_segment->data_size, "\xE5\x9F\xC0\x00\xE5\x9C\xF0\x00\0\0\0\0");
					// note: from ARMv6T2 onwards, movw/movt can also be used
					break;
				case CPU_ARM64:
					// TODO: test
					// we fill in the actual instructions later
					// adrp x16, rel
					// ldr x16, [x16, #rel]
					// br x16
					import_thunk_segment->WriteData(12, import_thunk_segment->data_size, "\0\0\0\0\0\0\0\0\xD6\x1F\x02\x00");
					break;
				default:
					Linker::Error << "Error: generating import thunks not supported for architecture" << std::endl;
					break;
				}
			}
			if(compatibility == CompatibleGNU && cpu_type == CPU_I386)
			{
				// TODO: not sure what this is
				import_thunk_segment->WriteData(16, import_thunk_segment->data_size, "\xFF\xFF\xFF\xFF\0\0\0\0\xFF\xFF\xFF\xFF\0\0\0\0");
			}
		}
	}
	else
	{
		Linker::SegmentManager::OnCallDirective(identifier);
	}
}

std::unique_ptr<Script::List> PEFormat::GetScript(Linker::Module& module)
{
	// TODO: the first section must come after the COFF/PE and section headers
	// and so the start of the code section depends on the number of sections

#if 0
	static const char * SimpleScript = R"(
".code"
{
	at ?image_base? + ?section_align?;
	all execute;
};

call "GenerateImportThunks";

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
#endif

	static const char * DefaultScript = R"(
at ?image_base? + ?section_align?;

for execute
{
	at align(here, ?section_align?);
	all;
};

call "GenerateImportThunks";

for not zero
{
	at align(here, ?section_align?);
	all;
};

for any
{
	at align(here, ?section_align?);
	all;
};
)";

	static const char * GNUScript = R"(
at ?image_base? + ?section_align?;

for ".text"
{
	at align(here, ?section_align?);
	all;
};

call "GenerateImportThunks";

for execute
{
	at align(here, ?section_align?);
	all;
};

for ".data"
{
	at align(here, ?section_align?);
	all;
};

for not zero
{
	at align(here, ?section_align?);
	all;
};

for ".bss"
{
	at align(here, ?section_align?);
	all;
};

for any
{
	at align(here, ?section_align?);
	all;
};
)";

	// This script attempts to simulate the default behavior of the Watcom linker
	// We don't currently support groups, so we make the following assumptions:
	// .text and .code belong to no group
	// .data, .bss and .comm belong to DGROUP
	// every other section belongs to a group with the same name as itself
	static const char * WatcomScript = R"(
at ?image_base? + ?section_align?;

for execute until ".text" or ".code"
{
	at align(here, ?section_align?);
	all;
};

for ".text" or ".code" call "AUTO"
{
	at align(here, ?section_align?);
	all ".text" or ".code";
};

for execute
{
	at align(here, ?section_align?);
	all;
};

call "GenerateImportThunks";

for ".data" or ".bss" or ".comm" call "DGROUP"
{
	at align(here, ?section_align?);
	all ".data" or ".bss" or ".comm";
};

for any call "AUTO"
{
	at align(here, ?section_align?);
	all not zero;
	all zero;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(compatibility)
		{
		case CompatibleWatcom:
			return Script::parse_string(WatcomScript);
		case CompatibleGNU:
			return Script::parse_string(GNUScript);
		default:
			return Script::parse_string(DefaultScript);
		}
	}
}

void PEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	current_module = &module;

	ProcessScript(script, module);

	//CreateDefaultSegments();
}

offset_t PEFormat::GenerateResourceSection(Linker::Module& module, offset_t image_end)
{
	// generate .rsrc
	sections.push_back(resources);
	resources->address = AddressToRVA(image_end);
	resources->Generate(*this);
	image_end = AlignTo(image_end + resources->MemorySize(*this), GetOptionalHeader().section_align);
	GetOptionalHeader().data_directories[PEOptionalHeader::DirResourceTable] =
		PEOptionalHeader::DataDirectory{uint32_t(resources->address), uint32_t(resources->size)};
	return image_end;
}

offset_t PEFormat::GenerateImportSection(Linker::Module& module, offset_t image_end)
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
	return image_end;
}

offset_t PEFormat::GenerateExportSection(Linker::Module& module, offset_t image_end)
{
	// generate .edata
	sections.push_back(exports);
	exports->address = AddressToRVA(image_end);
	exports->Generate(*this);
	image_end = AlignTo(image_end + exports->MemorySize(*this), GetOptionalHeader().section_align);
	GetOptionalHeader().data_directories[PEOptionalHeader::DirExportTable] =
		PEOptionalHeader::DataDirectory{uint32_t(exports->address), uint32_t(exports->size)};
	return image_end;
}

offset_t PEFormat::GenerateBaseRelocationSection(Linker::Module& module, offset_t image_end)
{
	// generate .reloc
	sections.push_back(base_relocations);
	base_relocations->address = AddressToRVA(image_end);
	base_relocations->Generate(*this);
	image_end = AlignTo(image_end + base_relocations->MemorySize(*this), GetOptionalHeader().section_align);
	GetOptionalHeader().data_directories[PEOptionalHeader::DirBaseRelocationTable] =
		PEOptionalHeader::DataDirectory{uint32_t(base_relocations->address), uint32_t(base_relocations->size)};
	return image_end;
}

void PEFormat::ProcessRelocations(Linker::Module& module)
{
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;

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

				if(rel.shift != 0 && rel.shift != 16)
				{
					Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
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
				break;
			case 8:
				if(!Is64Bit())
				{
					Linker::Error << "Error: 64-bit relocations not allowed in 32-bit mode, ignoring" << std::endl;
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

		if(rel.Resolve(module, resolution))
		{
			rel.WriteWord(resolution.value);
		}
		else
		{
			if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
			{
				if(!option_import_thunks && rel.size == 2)
				{
					Linker::Error << "Error: imported 16-bit references not allowed, ignoring" << std::endl;
					Linker::Error << "Error: " << rel << std::endl;
					continue;
				}

				std::string library, name;
				uint16_t ordinal;
				offset_t address = 0;

				if(symbol->GetImportedName(library, name))
				{
					if(!option_import_thunks)
					{
						address = FetchImportLibrary(library).GetImportByNameAddress(*this, name);
					}
					else
					{
						address = import_thunks_by_name[std::make_pair(library, name)] + import_thunk_segment->base_address;
					}
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					if(!option_import_thunks)
					{
						address = FetchImportLibrary(library).GetImportByOrdinalAddress(*this, ordinal);
					}
					else
					{
						address = import_thunks_by_ordinal[std::make_pair(library, ordinal)] + import_thunk_segment->base_address;
					}
				}
				else
				{
					Linker::Error << "Error: undefined " << *symbol << std::endl;
					continue;
				}

				Linker::Position reference;
				if(!rel.reference.Lookup(module, reference))
				{
					Linker::Error << "Error: unable to resolve " << rel << std::endl;
				}
				else
				{
					address -= reference.address;
				}

				rel.WriteWord(address + rel.addend);
			}
			else
			{
				Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			}
		}

		if(rel.IsAbsolute())
		{
			switch(rel.size)
			{
			case 2:
				if(rel.shift == 16)
				{
					Linker::Debug << "Debug: " << rel.source.GetPosition() << " REL_HIGH" << std::endl;
					AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelHigh);
				}
				else
				{
					Linker::Debug << "Debug: " << rel.source.GetPosition() << " REL_LOW" << std::endl;
					AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelLow);
				}
				break;
			case 4:
				Linker::Debug << "Debug: " << rel.source.GetPosition() << " REL_HIGHLOW" << std::endl;
				AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelHighLow);
				break;
			case 8:
				Linker::Debug << "Debug: " << rel.source.GetPosition() << " REL_DIR64" << std::endl;
				AddBaseRelocation(AddressToRVA(rel.source.GetPosition().address), BaseRelocation::RelDir64);
				break;
			}
		}
	}

	if(option_import_thunks && import_thunk_segment != nullptr)
	{
		// create the actual addresses for the imported thunks

		for(auto& import_data : import_thunks_by_name)
		{
			offset_t address = FetchImportLibrary(import_data.first.first).GetImportByNameAddress(*this, import_data.first.second);
			FixupImportThunk(module, import_data.second, address);
		}

		for(auto& import_data : import_thunks_by_ordinal)
		{
			offset_t address = FetchImportLibrary(import_data.first.first).GetImportByOrdinalAddress(*this, import_data.first.second);
			FixupImportThunk(module, import_data.second, address);
		}
	}
}

void PEFormat::FixupImportThunk(Linker::Module& module, offset_t offset, offset_t address)
{
	switch(cpu_type)
	{
	case CPU_I386:
		import_thunk_segment->WriteWord(4, offset + 2, address, ::LittleEndian);
		break;
	case CPU_AMD64:
		import_thunk_segment->WriteWord(4, offset + 2, address - (import_thunk_segment->base_address - offset + 6), ::LittleEndian);
		break;
	case CPU_ARM:
		import_thunk_segment->WriteWord(4, offset + 4, address, ::LittleEndian);
		// note: from ARMv6T2 onwards, movw/movt can also be used
		break;
	case CPU_ARM64:
		// TODO: test
		{
			// adrp x16, address
			uint64_t relative_address = address - (import_thunk_segment->base_address - offset);
			uint32_t instruction = 0x90000010
				| ((relative_address >> (12 - 5)) & 0x00FFFE00)
				| (relative_address >> (12 + 19 - 29) & 0x60000000);
			import_thunk_segment->WriteWord(4, offset, instruction, ::LittleEndian);
			// ldr x16, [x16, #address]
			instruction = 0xF9400210
				| ((relative_address & 0xFFF) << 10);
			import_thunk_segment->WriteWord(4, offset + 4, instruction, ::LittleEndian);
		}
		break;
	default:
		Linker::Error << "Error: generating import thunks not supported for architecture" << std::endl;
		break;
	}
}

offset_t PEFormat::GetMemoryImageEnd() const
{
	offset_t image_end = GetOptionalHeader().image_base + 1;
	if(Sections().size() != 0)
	{
		auto last_section = Sections().back();
		image_end = GetOptionalHeader().image_base + last_section->address + last_section->MemorySize(*this);
	}
	image_end = AlignTo(image_end, GetOptionalHeader().section_align);
	return image_end;
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
			std::shared_ptr<ExportedEntry> exported_symbol;
			if(!it.first.IsExportedByOrdinal())
			{
				std::string name = "";
				it.first.LoadName(name);
				exported_symbol = std::make_shared<ExportedEntry>(AddressToRVA(it.second.GetPosition().address), name);
			}
			else
			{
				exported_symbol = std::make_shared<ExportedEntry>(AddressToRVA(it.second.GetPosition().address));
			}
			if(ordinal < exports->ordinal_base)
			{
				Linker::Error << "Error: invalid ordinal " << ordinal << " for ordinal base set to " << exports->ordinal_base << std::endl;
				continue;
			}
			exports->SetEntry(ordinal - exports->ordinal_base, exported_symbol);
		}
	}

	/* then make entries for those symbols exported by name only */
	for(auto it : module.GetExportedSymbols())
	{
		if(!it.first.IsExportedByOrdinal())
		{
			std::shared_ptr<ExportedEntry> exported_symbol;
			std::string name = "";
			it.first.LoadName(name);
			exported_symbol = std::make_shared<ExportedEntry>(AddressToRVA(it.second.GetPosition().address), name);
			exports->AddEntry(exported_symbol);
		}
	}

	for(auto section : sections)
	{
		std::shared_ptr<Linker::Segment> image = GetSegment(section);
		Linker::Debug << "Debug: Naming PE segment to " << image->name << std::endl;
		section->name = image->name;
		section->address = AddressToRVA(image->base_address);
	}

	offset_t image_end = GetMemoryImageEnd();

	// TODO: order of these sections

	if(resources->IsPresent())
	{
		image_end = GenerateResourceSection(module, image_end);
	}

	if(compatibility == CompatibleGNU && exports->IsPresent())
	{
		image_end = GenerateExportSection(module, image_end);
	}

	if(imports->IsPresent())
	{
		image_end = GenerateImportSection(module, image_end);
	}

	if(compatibility != CompatibleGNU && exports->IsPresent())
	{
		image_end = GenerateExportSection(module, image_end);
	}

	ProcessRelocations(module);

	if(base_relocations->IsPresent())
	{
		image_end = GenerateBaseRelocationSection(module, image_end);
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

	exports->dll_name = filename;

	linker_parameters["image_base"] = GetOptionalHeader().image_base;
	linker_parameters["section_align"] = GetOptionalHeader().section_align;
	// TODO

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	switch(output)
	{
	default:
	case OUTPUT_EXE:
		return filename + ".exe";
	case OUTPUT_DLL:
		return filename + ".dll";
	case OUTPUT_SYS:
		return filename + ".sys";
	}
}

void PEFormat::RVADisplay::DisplayValue(Dumper::Dumper& dump, std::tuple<offset_t> values)
{
	dump.out << "(rva): ";
	dump.PrintHex(std::get<0>(values), width);
	offset_t file_offset = pe->RVAToFileOffset(std::get<0>(values));
	if(file_offset != 0)
	{
		dump.out << ", (off): ";
		dump.PrintHex(file_offset, width);
	}
}

std::shared_ptr<PEFormat::RVADisplay> PEFormat::MakeRVADisplay(unsigned width) const
{
	return std::make_shared<RVADisplay>(this, width);
}

