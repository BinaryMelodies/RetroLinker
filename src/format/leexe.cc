
#include <sstream>
#include "leexe.h"
#include "mzexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Microsoft;

offset_t LEFormat::PageSet::ImageSize() const
{
	offset_t size = 0;
	for(auto& page : pages)
	{
		size += page->ImageSize();
	}
	return size;
}

offset_t LEFormat::PageSet::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	offset_t total_count = 0;
	for(auto& page : pages)
	{
		if(offset >= page->ImageSize())
		{
			offset -= page->ImageSize();
		}
		else
		{
			offset_t actual_count = page->WriteFile(wr, count);
			total_count += actual_count;
			if(actual_count >= count)
				break;
			count -= actual_count;
			offset = 0;
		}
	}
	return total_count;
}

offset_t LEFormat::SegmentPage::ImageSize() const
{
	return size;
}

offset_t LEFormat::SegmentPage::WriteFile(Linker::Writer& wr, offset_t count, offset_t offset) const
{
	if(offset >= this->size)
		return 0;
	if(offset + count > this->size)
		count = this->size - offset;
	offset_t actual_count = image->WriteFile(wr, count, this->offset + offset);
	if(actual_count < count)
	{
		wr.Skip(count - actual_count);
	}
	return count;
}

std::shared_ptr<const Linker::ActualImage> LEFormat::SegmentPage::AsImage() const
{
	if(offset == 0 && size == image->ImageSize())
	{
		return image->AsImage();
	}
	else
	{
		return Image::AsImage();
	}
}

LEFormat::Page::Relocation::source_type LEFormat::Page::Relocation::GetType(Linker::Relocation& rel)
{
	if(rel.kind == Linker::Relocation::SelectorIndex)
	{
		return Selector16;
	}
	else
	{
		switch(rel.size)
		{
		case 1:
			return Offset8;
		case 2:
			return Offset16;
		case 4:
			return rel.IsRelative() ? Relative32 : Offset32;
		default:
			return source_type(-1);
		}
	}
}

bool LEFormat::Page::Relocation::IsExternal() const
{
	switch(flags & FlagTypeMask)
	{
	case Internal:
	case Entry: /* TODO: is this internal? */
		return false;
	default:
		return true;
	}
}

bool LEFormat::Page::Relocation::IsSelectorOrOffset() const
{
	switch(type & SourceTypeMask)
	{
	case Selector16:
	case Pointer32:
	case Pointer48:
		return true;
	default:
		return false;
	}
}

bool LEFormat::Page::Relocation::ComesBefore() const
{
	return IsExternal() || IsSelectorOrOffset();
}

size_t LEFormat::Page::Relocation::GetSourceSize() const
{
	switch(type & SourceTypeMask)
	{
	case Offset8:
		return 1;
	case Selector16:
	case Offset16:
		return 2;
	case Pointer32:
	case Offset32:
	case Relative32:
		return 4;
	case Pointer48:
		return 6;
	default:
		Linker::FatalError("Internal error: invalid relocation type");
	}
}

/* do not call this */
void LEFormat::Page::Relocation::DecrementSingleSourceOffset(size_t amount)
{
	assert(sources.size() == 1);
	sources[0].source -= amount;
}

bool LEFormat::Page::Relocation::IsSelector() const
{
	return (type & SourceTypeMask) == Selector16;
}

bool LEFormat::Page::Relocation::IsSourceList() const
{
	return type & SourceList;
}

bool LEFormat::Page::Relocation::IsAdditive() const
{
	return flags & Additive;
}

size_t LEFormat::Page::Relocation::GetTargetSize() const
{
	return flags & Target32 ? 4 : 2;
}

size_t LEFormat::Page::Relocation::GetAdditiveSize() const
{
	return flags & Additive32 ? 4 : 2;
}

size_t LEFormat::Page::Relocation::GetModuleSize() const
{
	return flags & Module16 ? 2 : 1;
}

size_t LEFormat::Page::Relocation::GetOrdinalSize() const
{
	return flags & Ordinal8 ? 1 : GetTargetSize();
}

uint16_t LEFormat::Page::Relocation::GetFirstSource() const
{
	return sources[0].source;
}

void LEFormat::Page::Relocation::CalculateSizes(compatibility_type compatibility)
{
	if(sources.size() != 1)
		type = source_type(type | SourceList);
	if(module > 0xFF)
		flags = flag_type(flags | Module16);
	switch(flags & FlagTypeMask)
	{
	case Internal:
		if(!IsSelector() && target > 0xFFFF)
			flags = flag_type(flags | Target32);
		break;
	case ImportOrdinal:
		if(target <= 0xFF && compatibility != CompatibleWatcom)
			flags = flag_type(flags | Ordinal8);
		else if(target > 0xFFFF)
			flags = flag_type(flags | Target32);
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = flag_type(flags | Additive32);
		break;
	case ImportName:
		if(target > 0xFFFF)
			flags = flag_type(flags | Target32);
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = flag_type(flags | Additive32);
		break;
	case Entry:
		if(IsAdditive() && addition > 0xFFFF) /* TODO: signed or unsigned? */
			flags = flag_type(flags | Additive32);
		break;
	}
}

size_t LEFormat::Page::Relocation::GetSize() const
{
	//CalculateSizes();
	size_t size = 2 + 2 * sources.size();
	if(IsSourceList())
	{
		/* it is
			count ... list of sources
		instead of
			source
		*/
		size += 1;
	}
	switch(flags & FlagTypeMask)
	{
	case Internal:
		size += GetModuleSize();
		if(!IsSelector())
			size += GetTargetSize();
		break;
	case ImportOrdinal:
		size += GetModuleSize() + GetTargetSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	case ImportName:
		size += GetModuleSize() + GetOrdinalSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	case Entry:
		size += GetModuleSize();
		if(IsAdditive())
			size += GetAdditiveSize();
		break;
	}
	return size;
}

LEFormat::Page::Relocation LEFormat::Page::Relocation::ReadFile(Linker::Reader& rd)
{
	Relocation relocation;
	relocation.type = source_type(rd.ReadUnsigned(1));
	relocation.flags = flag_type(rd.ReadUnsigned(1));
	uint8_t source_list_size;
	relocation.sources.clear();
	if(relocation.IsSourceList())
	{
		source_list_size = rd.ReadUnsigned(1);
	}
	else
	{
		source_list_size = 0;
		uint16_t source = rd.ReadUnsigned(2);
		relocation.sources.emplace_back(Chain{source});
	}
	switch(relocation.flags & FlagTypeMask)
	{
	case Internal:
		relocation.module = rd.ReadUnsigned(relocation.GetModuleSize());
		if(!relocation.IsSelector())
			relocation.target = rd.ReadUnsigned(relocation.GetTargetSize());
		break;
	case ImportOrdinal:
		relocation.module = rd.ReadUnsigned(relocation.GetModuleSize());
		relocation.target = rd.ReadUnsigned(relocation.GetTargetSize());
		if(relocation.IsAdditive())
			relocation.addition = rd.ReadUnsigned(relocation.GetAdditiveSize());
		break;
	case ImportName:
		relocation.module = rd.ReadUnsigned(relocation.GetModuleSize());
		relocation.target = rd.ReadUnsigned(relocation.GetOrdinalSize());
		if(relocation.IsAdditive())
			relocation.addition = rd.ReadUnsigned(relocation.GetAdditiveSize());
		break;
	case Entry:
		relocation.module = rd.ReadUnsigned(relocation.GetModuleSize());
		if(relocation.IsAdditive())
			relocation.addition = rd.ReadUnsigned(relocation.GetAdditiveSize());
		break;
	}
	if(relocation.IsSourceList())
	{
		for(uint8_t i = 0; i < source_list_size; i++)
		{
			uint16_t source = rd.ReadUnsigned(2);
			relocation.sources.emplace_back(Chain{source});
			// TODO: read chain
		}
	}
	return relocation;
}

void LEFormat::Page::Relocation::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(1, type);
	wr.WriteWord(1, flags);
	if(IsSourceList())
	{
		wr.WriteWord(1, sources.size());
	}
	else
	{
		assert(sources.size() == 1);
		wr.WriteWord(2, sources[0].source);
	}
	switch(flags & FlagTypeMask)
	{
	case Internal:
		wr.WriteWord(GetModuleSize(), module);
		if(!IsSelector())
			wr.WriteWord(GetTargetSize(), target);
		break;
	case ImportOrdinal:
		wr.WriteWord(GetModuleSize(), module);
		wr.WriteWord(GetTargetSize(), target);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	case ImportName:
		wr.WriteWord(GetModuleSize(), module);
		wr.WriteWord(GetOrdinalSize(), target);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	case Entry:
		wr.WriteWord(GetModuleSize(), module);
		if(IsAdditive())
			wr.WriteWord(GetAdditiveSize(), addition);
		break;
	}
	if(IsSourceList())
	{
		for(Chain source : sources)
		{
			wr.WriteWord(2, source.source);
			// TODO: write chain
		}
	}
}

LEFormat::Page LEFormat::Page::LEPage(uint16_t fixup_table_index, uint8_t type)
{
	return Page(fixup_table_index, type);
}

LEFormat::Page LEFormat::Page::LXPage(uint32_t offset, uint16_t size, uint8_t flags)
{
	return Page(offset, size, flags);
}

bool LEFormat::Entry::SameBundle(const Entry& other) const
{
	if(type != other.type)
		return false;
	switch(type)
	{
	case Unused:
	case Forwarder:
		return true;
	case Entry16:
	case CallGate286:
	case Entry32:
		return object == other.object;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

offset_t LEFormat::Entry::GetEntryHeadSize() const
{
	switch(type)
	{
	case Unused:
		return 2;
	case Entry16:
	case CallGate286:
	case Entry32:
		return 4;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

offset_t LEFormat::Entry::GetEntryBodySize() const
{
	switch(type)
	{
	case Unused:
		return 0;
	case Entry16:
		return 3;
	case CallGate286:
	case Entry32:
		return 5;
	case Forwarder:
		return 7;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

LEFormat::Entry LEFormat::Entry::ReadEntryHead(Linker::Reader& rd, uint8_t type)
{
	Entry entry;
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
	case CallGate286:
	case Entry32:
		entry.object = rd.ReadUnsigned(2);
		break;
	case Forwarder:
		rd.Skip(2); /* reserved */
		break;
	}
	return entry;
}

LEFormat::Entry LEFormat::Entry::ReadEntry(Linker::Reader& rd, uint8_t type, LEFormat::Entry& head)
{
	Entry entry;
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
		entry.object = head.object;
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		entry.offset = rd.ReadUnsigned(2);
		break;
	case CallGate286:
		entry.object = head.object;
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		entry.offset = rd.ReadUnsigned(2);
		rd.Skip(2); /* reserved - call gate */
		break;
	case Entry32:
		entry.object = head.object;
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		entry.offset = rd.ReadUnsigned(4);
		break;
	case Forwarder:
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		entry.object = rd.ReadUnsigned(2); /* module */
		entry.offset = rd.ReadUnsigned(4); /* ordinal or name */
		break;
	}
	return entry;
}

void LEFormat::Entry::WriteEntryHead(Linker::Writer& wr) const
{
	wr.WriteWord(1, type);
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
	case CallGate286:
	case Entry32:
		wr.WriteWord(2, object);
		break;
	case Forwarder:
		wr.WriteWord(2, 0); /* reserved */
		break;
	}
}

void LEFormat::Entry::WriteEntryBody(Linker::Writer& wr) const
{
	switch(type)
	{
	case Unused:
		break;
	case Entry16:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		break;
	case CallGate286:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		wr.WriteWord(2, 0); /* reserved - call gate */
		break;
	case Entry32:
		wr.WriteWord(1, flags);
		wr.WriteWord(4, offset);
		break;
	case Forwarder:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, object); /* module */
		wr.WriteWord(4, offset); /* ordinal or name */
		break;
	}
}

bool LEFormat::ModuleDirective::IsResident() const
{
	return (directive & ResidentFlagMask) != 0;
}

bool LEFormat::IsExtendedFormat() const
{
	return signature[1] == 'X';
}

bool LEFormat::IsLibrary() const
{
	return module_flags & Library;
}

bool LEFormat::IsDriver() const
{
	/* TODO: Windows only */
	return system == Windows;
}

bool LEFormat::IsOS2() const
{
	return IsExtendedFormat();
}

void LEFormat::ReadFile(Linker::Reader& rd)
{
	/* new header */
	file_offset = rd.Tell();
	rd.ReadData(signature);
	uint8_t byte_order = rd.ReadUnsigned(1);
	if(byte_order != 0 && byte_order != 1)
	{
		Linker::FatalError("Internal error: invalid byte order");
	}
	uint8_t word_order = rd.ReadUnsigned(1);
	if(word_order != 0 && word_order != 1)
	{
		Linker::FatalError("Internal error: invalid word order");
	}
	switch(byte_order)
	{
	case 0:
		switch(word_order)
		{
		case 0:
			endiantype = ::LittleEndian;
			break;
		case 1:
			endiantype = ::PDP11Endian;
			break;
		}
		break;
	case 1:
		switch(word_order)
		{
		case 0:
			endiantype = ::AntiPDP11Endian;
			break;
		case 1:
			endiantype = ::BigEndian;
			break;
		}
		break;
	}

	format_level = rd.ReadUnsigned(4);
	if(format_level != 0)
	{
		Linker::Error << "Error: unrecognized LE/LX format level " << format_level << ", ignoring" << std::endl;
	}

	cpu = cpu_type(rd.ReadUnsigned(2));
	system = system_type(rd.ReadUnsigned(2));
	module_version = rd.ReadUnsigned(4);
	module_flags = rd.ReadUnsigned(4);
	page_count = rd.ReadUnsigned(4);
	eip_object = rd.ReadUnsigned(4);
	eip_value = rd.ReadUnsigned(4);
	esp_object = rd.ReadUnsigned(4);
	esp_value = rd.ReadUnsigned(4);
	page_size = rd.ReadUnsigned(4);
	page_offset_shift = rd.ReadUnsigned(4); /* or size of last page */
	fixup_section_size = rd.ReadUnsigned(4);
	fixup_section_checksum = rd.ReadUnsigned(4);
	loader_section_size = rd.ReadUnsigned(4);
	loader_section_checksum = rd.ReadUnsigned(4);
	object_table_offset = rd.ReadUnsigned(4);
	if(object_table_offset != 0)
	{
		object_table_offset += file_offset;
	}
	uint32_t object_count = rd.ReadUnsigned(4);
	object_page_table_offset = rd.ReadUnsigned(4);
	if(object_page_table_offset != 0)
	{
		object_page_table_offset += file_offset;
	}
	object_iterated_pages_offset = rd.ReadUnsigned(4);
	if(object_iterated_pages_offset != 0)
	{
		object_iterated_pages_offset += file_offset;
	}
	resource_table_offset = rd.ReadUnsigned(4);
	if(resource_table_offset != 0)
	{
		resource_table_offset += file_offset;
	}
	resource_table_entry_count = rd.ReadUnsigned(4);
	resident_name_table_offset = rd.ReadUnsigned(4);
	if(resident_name_table_offset != 0)
	{
		resident_name_table_offset += file_offset;
	}
	entry_table_offset = rd.ReadUnsigned(4);
	if(entry_table_offset != 0)
	{
		entry_table_offset += file_offset;
	}
	module_directives_offset = rd.ReadUnsigned(4);
	if(module_directives_offset != 0)
	{
		module_directives_offset += file_offset;
	}
	uint32_t module_directives_count = rd.ReadUnsigned(4);
	fixup_page_table_offset = rd.ReadUnsigned(4);
	if(fixup_page_table_offset != 0)
	{
		fixup_page_table_offset += file_offset;
	}
	fixup_record_table_offset = rd.ReadUnsigned(4);
	if(fixup_record_table_offset != 0)
	{
		fixup_record_table_offset += file_offset;
	}
	imported_module_table_offset = rd.ReadUnsigned(4);
	if(imported_module_table_offset != 0)
	{
		imported_module_table_offset += file_offset;
	}
	uint32_t imported_module_count = rd.ReadUnsigned(4);
	imported_procedure_table_offset = rd.ReadUnsigned(4);
	if(imported_procedure_table_offset != 0)
	{
		imported_procedure_table_offset += file_offset;
	}

	per_page_checksum_offset = rd.ReadUnsigned(4);
	data_pages_offset = rd.ReadUnsigned(4);
	preload_page_count = rd.ReadUnsigned(4);
	nonresident_name_table_offset = rd.ReadUnsigned(4);
	nonresident_name_table_size = rd.ReadUnsigned(4);
	nonresident_name_table_checksum = rd.ReadUnsigned(4);
	automatic_data = rd.ReadUnsigned(4);
	debug_info_offset = rd.ReadUnsigned(4);
	debug_info_size = rd.ReadUnsigned(4);
	instance_preload_page_count = rd.ReadUnsigned(4);
	instance_demand_page_count = rd.ReadUnsigned(4);
	heap_size = rd.ReadUnsigned(4);
	stack_size = rd.ReadUnsigned(4);
	rd.Skip(8);
	vxd_version_info_resource_offset = rd.ReadUnsigned(4);
	vxd_version_info_resource_length = rd.ReadUnsigned(4);
	vxd_device_id = rd.ReadUnsigned(2);
	vxd_ddk_version = rd.ReadUnsigned(2);

	file_size = rd.Tell();

	/*** Loader Section ***/
	/* Object Table */
	rd.Seek(object_table_offset);
	for(uint32_t i = 0; i < object_count; i++)
	{
		Object object;
		object.size = rd.ReadUnsigned(4);
		object.address = rd.ReadUnsigned(4);
		object.flags = Object::flag_type(rd.ReadUnsigned(4));
		object.page_table_index = rd.ReadUnsigned(4);
		object.page_entry_count = rd.ReadUnsigned(4);
		rd.Skip(4);
		objects.emplace_back(object);
	}

	file_size = std::max(file_size, rd.Tell());

	/* Object Page Table */
	rd.Seek(object_page_table_offset);
	pages.push_back(Page());
	if(IsExtendedFormat())
	{
		for(uint32_t i = 0; i < page_count; i++)
		{
			Page page;
			page.lx.offset = offset_t(rd.ReadUnsigned(4)) << page_offset_shift;
			page.lx.size = rd.ReadUnsigned(2);
			page.lx.flags = rd.ReadUnsigned(2);
			pages.push_back(page);
		}
	}
	else
	{
		for(uint32_t i = 0; i < page_count; i++)
		{
			Page page;
			page.le.fixup_table_index = rd.ReadUnsigned(3, ::BigEndian);
			page.le.type = rd.ReadUnsigned(1);
			pages.push_back(page);
		}
	}
	pages.push_back(Page());

	file_size = std::max(file_size, rd.Tell());

	/* Resource Table */
	rd.Seek(resource_table_offset);
	for(uint32_t i = 0; i < resource_table_entry_count; i++)
	{
		Resource resource;
		resource.type = rd.ReadUnsigned(2);
		resource.name = rd.ReadUnsigned(2);
		resource.size = rd.ReadUnsigned(4);
		resource.object = rd.ReadUnsigned(2);
		resource.offset = rd.ReadUnsigned(4);
		AddResource(resource);
	}

	file_size = std::max(file_size, rd.Tell());

	/* Resident Name Table */
	rd.Seek(resident_name_table_offset);
	while(true)
	{
		uint8_t length = rd.ReadUnsigned(1);
		if(length == 0)
			break;
		Name name;
		name.name = rd.ReadData(length);
		name.ordinal = rd.ReadUnsigned(2);
		resident_names.emplace_back(name);
	}

	file_size = std::max(file_size, rd.Tell());

	/* Entry Table */
	rd.Seek(entry_table_offset);
	while(true)
	{
		uint8_t entry_count = rd.ReadUnsigned(1);
		if(entry_count == 0)
			break;
		uint8_t type = rd.ReadUnsigned(1);
		Entry head = Entry::ReadEntryHead(rd, type);
		for(uint8_t i = 0; i < entry_count; i ++)
		{
			entries.emplace_back(Entry::ReadEntry(rd, type, head));
			entries.back().same_bundle = i != 0;
		}
	}

	file_size = std::max(file_size, rd.Tell());

	/* Module Format Directives Table */
	if(module_directives_count != 0)
	{
		rd.Seek(module_directives_offset);
		for(uint32_t i = 0; i < module_directives_count; i++)
		{
			ModuleDirective directive;
			directive.directive = ModuleDirective::directive_number(rd.ReadUnsigned(2));
			directive.length = rd.ReadUnsigned(2);
			directive.offset = rd.ReadUnsigned(4);
			if(directive.IsResident())
				directive.offset += file_size;
			module_directives.emplace_back(directive);
		}
		file_size = std::max(file_size, rd.Tell());
	}

	/* Resident Directives */
	// TODO

	/* Per-page Checksum */
	if(per_page_checksum_offset != 0)
	{
		rd.Seek(per_page_checksum_offset);
		for(Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			page.checksum = rd.ReadUnsigned(4);
		}
		file_size = std::max(file_size, rd.Tell());
	}

	/*** Fixup Section ***/
	/* Fixup Page Table */
	rd.Seek(fixup_page_table_offset);
	for(Page& page : pages)
	{
		if(&page == &pages.front())
			continue;
		page.fixup_offset = rd.ReadUnsigned(4);
	}

	file_size = std::max(file_size, rd.Tell());

	/* Fixup Record Table */
	for(uint32_t i = 1; i < pages.size() - 1; i++)
	{
		Page& page = pages[i];
		//if(&page == &pages.front() || &page == &pages.back())
		//	continue;
		rd.Seek(fixup_record_table_offset + page.fixup_offset);
		offset_t end = fixup_record_table_offset + pages[i + 1].fixup_offset;
		while(rd.Tell() < end)
		{
			Page::Relocation relocation = Page::Relocation::ReadFile(rd);
			page.relocations[relocation.GetFirstSource()] = relocation;
		}
	}

	file_size = std::max(file_size, rd.Tell());

	/* Import Module Name Table */
	rd.Seek(imported_module_table_offset);
	while(rd.Tell() < imported_procedure_table_offset && imported_modules.size() < imported_module_count)
	{
		uint8_t length = rd.ReadUnsigned(1);
		std::string name = rd.ReadData(length);
		imported_modules.emplace_back(name);
	}

	file_size = std::max(file_size, rd.Tell());

	/* Import Procedure Name Table */
	rd.Seek(imported_procedure_table_offset);
	offset_t imported_procedure_table_end;
	if(!IsExtendedFormat() && per_page_checksum_offset != 0)
		imported_procedure_table_end = per_page_checksum_offset;
	else
		imported_procedure_table_end = fixup_page_table_offset + fixup_section_size;
	while(rd.Tell() < imported_procedure_table_end)
	{
		uint8_t length = rd.ReadUnsigned(1);
		std::string name = rd.ReadData(length);
		imported_procedures.emplace_back(name);
	}

	file_size = std::max(file_size, rd.Tell());

	/*** Page Data ***/
	if(!IsExtendedFormat())
		rd.Seek(data_pages_offset);
	for(uint32_t i = 1; i <= page_count; i++)
	{
		Page& page = pages[i];
		uint16_t size = GetPageSize(i);
		if(IsExtendedFormat())
			rd.Seek(data_pages_offset + page.lx.offset);
		// TODO: other types than legal physical page
		page.image = Linker::Buffer::ReadFromFile(rd, size);
//		Linker::Debug << "Debug: page " << i << std::endl;
//		assert(pages[i].image != nullptr);
		file_size = std::max(file_size, rd.Tell());
	}

	for(auto& object : objects)
	{
		std::shared_ptr<PageSet> image = std::make_shared<PageSet>();
		for(uint32_t i = object.page_table_index; i < object.page_table_index + object.page_entry_count; i++)
		{
//			Linker::Debug << "Debug: page " << i << std::endl;
			assert(pages[i].image != nullptr);
			image->pages.push_back(pages[i].image);
		}
		object.image = image;
	}

	/*** Non-Resident ***/
	/* Non-Resident Name Table */
	if(nonresident_name_table_offset != 0)
	{
		rd.Seek(nonresident_name_table_offset);
		while(true)
		{
			uint8_t length = rd.ReadUnsigned(1);
			if(length == 0)
				break;
			Name name;
			name.name = rd.ReadData(length);
			name.ordinal = rd.ReadUnsigned(2);
			nonresident_names.emplace_back(name);
		}

		file_size = std::max(file_size, rd.Tell());
	}

	/* TODO */
}

offset_t LEFormat::ImageSize() const
{
	return file_size;
}

offset_t LEFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = endiantype;
	stub.WriteStubImage(wr);

	/* new header */
	wr.Seek(file_offset);
	wr.WriteData(signature);
	switch(endiantype)
	{
	case ::LittleEndian:
		wr.WriteData(2, "\0\0");
		break;
	case ::BigEndian:
		wr.WriteData(2, "\1\1");
		break;
	case ::PDP11Endian:
		wr.WriteData(2, "\0\1");
		break;
	case ::AntiPDP11Endian:
		wr.WriteData(2, "\1\0");
		break;
	default:
		Linker::FatalError("Internal error: invalid endianness");
	}
	wr.WriteWord(4, format_level);
	wr.WriteWord(2, cpu);
	wr.WriteWord(2, system);
	wr.WriteWord(4, module_version);
	wr.WriteWord(4, module_flags);
	wr.WriteWord(4, page_count); /* page 0 is fake, final page is only used in the page table */
	wr.WriteWord(4, eip_object);
	wr.WriteWord(4, eip_value);
	wr.WriteWord(4, esp_object);
	wr.WriteWord(4, esp_value);
	wr.WriteWord(4, page_size); /* page size */
	wr.WriteWord(4, page_offset_shift); /* or size of last page */
	wr.WriteWord(4, fixup_section_size);
	wr.WriteWord(4, fixup_section_checksum);
	wr.WriteWord(4, loader_section_size);
	wr.WriteWord(4, loader_section_checksum);
	wr.WriteWord(4, object_table_offset - file_offset);
	wr.WriteWord(4, objects.size());
	wr.WriteWord(4, object_page_table_offset - file_offset);
	wr.WriteWord(4, object_iterated_pages_offset != 0 ? object_iterated_pages_offset - file_offset : 0);
	wr.WriteWord(4, resource_table_offset - file_offset);
	wr.WriteWord(4, resource_table_entry_count);
	wr.WriteWord(4, resident_name_table_offset - file_offset);
	wr.WriteWord(4, entry_table_offset - file_offset);
	wr.WriteWord(4, module_directives_offset != 0 ? module_directives_offset - file_offset : 0);
	wr.WriteWord(4, module_directives.size());
	wr.WriteWord(4, fixup_page_table_offset - file_offset);
	wr.WriteWord(4, fixup_record_table_offset - file_offset);
	wr.WriteWord(4, imported_module_table_offset - file_offset);
	wr.WriteWord(4, imported_modules.size());
	wr.WriteWord(4, imported_procedure_table_offset - file_offset);
	wr.WriteWord(4, per_page_checksum_offset);
	wr.WriteWord(4, data_pages_offset);
	wr.WriteWord(4, preload_page_count);
	wr.WriteWord(4, nonresident_name_table_offset);
	wr.WriteWord(4, nonresident_name_table_size);
	wr.WriteWord(4, nonresident_name_table_checksum);
	wr.WriteWord(4, automatic_data);
	wr.WriteWord(4, debug_info_offset);
	wr.WriteWord(4, debug_info_size);
	wr.WriteWord(4, instance_preload_page_count);
	wr.WriteWord(4, instance_demand_page_count);
	wr.WriteWord(4, heap_size);
	wr.WriteWord(4, stack_size);

	wr.Skip(8);
	wr.WriteWord(4, vxd_version_info_resource_offset);
	wr.WriteWord(4, vxd_version_info_resource_length);
	wr.WriteWord(2, vxd_device_id);
	wr.WriteWord(2, vxd_ddk_version);

	/*** Loader Section ***/
	wr.Seek(file_offset + 0xC4);

	/* Object Table */
	assert(wr.Tell() == object_table_offset);
	for(const Object& object : objects)
	{
		if(auto pointer = dynamic_cast<Linker::Segment *>(object.image.get()))
			assert(object.size == pointer->TotalSize());
		wr.WriteWord(4, object.size);
		wr.WriteWord(4, object.address);
		wr.WriteWord(4, object.flags);
		wr.WriteWord(4, object.page_table_index);
		wr.WriteWord(4, object.page_entry_count);
		wr.WriteWord(4, 0);
	}

	/* Object Page Table */
	assert(wr.Tell() == object_page_table_offset);
	if(IsExtendedFormat())
	{
		for(const Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(4, page.lx.offset >> page_offset_shift);
			wr.WriteWord(2, page.lx.size);
			wr.WriteWord(2, page.lx.flags);
		}
	}
	else
	{
		for(const Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(3, page.le.fixup_table_index, ::BigEndian);
			wr.WriteWord(1, page.le.type);
		}
	}

	/* Resource Table */
	assert(wr.Tell() == resource_table_offset);
	// TODO: untested
	for(auto it1 : resources)
	{
		for(auto it2 : it1.second)
		{
			wr.WriteWord(2, it2.second.type);
			wr.WriteWord(2, it2.second.name);
			wr.WriteWord(4, it2.second.size);
			wr.WriteWord(2, it2.second.object);
			wr.WriteWord(4, it2.second.offset);
		}
	}

	/* Resident Name Table */
	assert(wr.Tell() == resident_name_table_offset);
	for(const Name& name : resident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name);
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	/* Entry Table */
	assert(wr.Tell() == entry_table_offset);
//offset_t _ = wr.Tell();
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		wr.WriteWord(1, entry_count);
		entries[entry_index].WriteEntryHead(wr);
		for(size_t entry_offset = 0; entry_offset < entry_count; entry_offset ++)
		{
			entries[entry_index + entry_offset].WriteEntryBody(wr);
		}
		entry_index += entry_count;
//Linker::Debug << "Debug: Write " << wr.Tell() - _ << std::endl; _ = Tell();
	}
	wr.WriteWord(1, 0);

	/* Module Format Directives Table */
	if(module_directives.size() != 0)
	{
		// TODO: untested
		assert(wr.Tell() == module_directives_offset);
		for(const ModuleDirective& directive : module_directives)
		{
			wr.WriteWord(2, directive.directive);
			wr.WriteWord(2, directive.length);
			wr.WriteWord(4, directive.offset - (directive.IsResident() ? file_offset : 0));
		}
	}

	/* Resident Directives */
	// TODO

	/* Per-page Checksum (LX) */
	if(IsExtendedFormat() && per_page_checksum_offset != 0)
	{
		assert(wr.Tell() == per_page_checksum_offset);
		for(const Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(4, page.checksum);
		}
	}

	/*** Fixup Section ***/
	/* Fixup Page Table */
	assert(wr.Tell() == fixup_page_table_offset);
	for(const Page& page : pages)
	{
		if(&page == &pages.front())
			continue;
		wr.WriteWord(4, page.fixup_offset);
	}

	/* Fixup Record Table */
	assert(wr.Tell() == fixup_record_table_offset);
//size_t _ = wr.Tell();
	for(const Page& page : pages)
	{
		if(&page == &pages.front() || &page == &pages.back())
			continue;
//Linker::Debug << "Debug: Receive page (before) " << page.relocations.size() << std::endl;
		for(auto& it : page.relocations)
		{
			if(it.second.ComesBefore())
				it.second.WriteFile(wr);
//Linker::Debug << "Debug: Receive " << wr.Tell() - _ << std::endl; _ = Tell();
		}
//Linker::Debug << "Debug: Receive page (after) " << page.relocations.size() << std::endl;
		for(auto& it : page.relocations)
		{
			if(!it.second.ComesBefore())
				it.second.WriteFile(wr);
//Linker::Debug << "Debug: Receive " << wr.Tell() - _ << std::endl; _ = Tell();
		}
	}

	/* Import Module Name Table */
	assert(wr.Tell() == imported_module_table_offset);
	for(const std::string& name : imported_modules)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name);
	}

	/* Import Procedure Name Table */
	assert(wr.Tell() == imported_procedure_table_offset);
	for(const std::string& name : imported_procedures)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name);
	}

	/* Per-page Checksum (LE) */
	if(!IsExtendedFormat() && per_page_checksum_offset != 0)
	{
		assert(wr.Tell() == per_page_checksum_offset);
		for(const Page& page : pages)
		{
			if(&page == &pages.front() || &page == &pages.back())
				continue;
			wr.WriteWord(4, page.checksum);
		}
	}

	/*** Page Data ***/
	uint32_t data_offset = data_pages_offset;
	for(const Object& object : objects)
	{
		if(!IsExtendedFormat())
		{
			wr.Seek(data_offset);
			data_offset += object.page_entry_count * page_size;
		}
		for(uint32_t page_index = object.page_table_index; page_index < object.page_table_index + object.page_entry_count; page_index++)
		{
			const Page& page = pages[page_index];
			if(IsExtendedFormat())
				wr.Seek(data_pages_offset + page.lx.offset);
			page.image->WriteFile(wr);
		}
#if 0
		/* TODO: is this still needed? */
		if(!IsExtendedFormat() && &object != &objects.back())
		{
			/* TODO: fill in */
		}
#endif
	}

	/*** Non-Resident ***/
	/* Non-Resident Name Table */
	for(const Name& name : nonresident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name);
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);
	/* Non-Resident Directives */

	return offset_t(-1);
}

void LEFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_windows1252);

	dump.SetTitle("LE/LX format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.Display(dump);

	static const std::map<offset_t, std::string> endian_descriptions =
	{
		{ ::LittleEndian, "little endian byte ordering, little endian word ordering" },
		{ ::PDP11Endian, "little endian byte ordering, big endian word ordering" },
		{ ::AntiPDP11Endian, "big endian byte ordering, little endian word ordering" },
		{ ::BigEndian, "big endian byte ordering, big endian word ordering" },
	};
	Dumper::Region header_region("New header", file_offset, object_table_offset - file_offset, 8);
	header_region.AddField("Signature", Dumper::StringDisplay::Make("'"), std::string(signature.data(), 2));
	header_region.AddField("Byte order", Dumper::ChoiceDisplay::Make(endian_descriptions), offset_t(endiantype));
	header_region.AddField("Format level", Dumper::HexDisplay::Make(4), offset_t(format_level));
	static const std::map<offset_t, std::string> cpu_descriptions =
	{
		{ I286, "Intel 80286" },
		{ I386, "Intel 80386" },
		{ I486, "Intel 80486" },
		{ I586, "Intel Pentium (80586)" },
		{ I860_N10, "Intel i860 (N10)" },
		{ I860_N11, "Intel i860 (N11)" },
		{ MIPS1, "MIPS Mark I (R2000, R3000)" },
		{ MIPS2, "MIPS Mark II (R6000)" },
		{ MIPS3, "MIPS Mark III (R4000)" },
	};
	header_region.AddField("CPU", Dumper::ChoiceDisplay::Make(cpu_descriptions, Dumper::HexDisplay::Make(4)), offset_t(cpu));
	static const std::map<offset_t, std::string> system_descriptions =
	{
		{ OS2, "Microsoft/IBM OS/2" },
		{ Windows, "Microsoft Windows" },
		{ MSDOS4, "Multitasking/\"European\" Microsoft MS-DOS 4.0" },
		{ Windows386, "Microsoft Windows 386" },
	};
	header_region.AddField("System", Dumper::ChoiceDisplay::Make(system_descriptions, Dumper::HexDisplay::Make(4)), offset_t(system));
	header_region.AddField("Module version", Dumper::HexDisplay::Make(8), offset_t(module_version));
	static const std::map<offset_t, std::string> gui_descriptions =
	{
		{ 1, "incompatible with GUI" },
		{ 2, "compatible with GUI" },
		{ 3, "uses GUI API" },
	};
	static const std::map<offset_t, std::string> module_type_descriptions =
	{
		{ 0, "program module" },
		{ 1, "library module" },
		{ 3, "protected memory library module" },
		{ 4, "physical device driver module" },
		{ 5, "virtual device driver module" },
	};
	header_region.AddField("Module flags",
		Dumper::BitFieldDisplay::Make(8)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("per-process library initialization"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("internal fixups have been applied"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("external fixups have been applied"), true)
			->AddBitField(8, 3, Dumper::ChoiceDisplay::Make(gui_descriptions), false)
			->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("module not loadable"), true)
			->AddBitField(15, 3, Dumper::ChoiceDisplay::Make(module_type_descriptions), "module type")
			->AddBitField(19, 1, Dumper::ChoiceDisplay::Make("multi-processor unsafe"), true)
			->AddBitField(30, 1, Dumper::ChoiceDisplay::Make("per-process library termination"), true),
		offset_t(module_flags));
	header_region.AddField("Automatic data object", Dumper::DecDisplay::Make(), offset_t(automatic_data));
	header_region.AddField("Entry (EIP)", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(eip_object), offset_t(eip_value));
	header_region.AddField("Stack top (ESP)", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(esp_object), offset_t(esp_value));
	header_region.AddOptionalField("Heap size", Dumper::HexDisplay::Make(8), offset_t(heap_size));
	header_region.AddOptionalField("Stack size", Dumper::HexDisplay::Make(8), offset_t(stack_size));
	if(IsExtendedFormat())
	{
		header_region.AddField("Page offset shift", Dumper::HexDisplay::Make(8), offset_t(page_offset_shift));
	}
	else
	{
		header_region.AddField("Last page size", Dumper::HexDisplay::Make(8), offset_t(last_page_size));
	}
	header_region.AddOptionalField("VxD Device ID", Dumper::HexDisplay::Make(4), offset_t(vxd_device_id));
	header_region.AddOptionalField("VxD DDK version", Dumper::HexDisplay::Make(4), offset_t(vxd_ddk_version));
	header_region.Display(dump);
	if(vxd_version_info_resource_length != 0)
	{
		Dumper::Region vxd_version_info_region("VxD version info resource", vxd_version_info_resource_offset, vxd_version_info_resource_length, 8);
		vxd_version_info_region.Display(dump);
	}

	Dumper::Region loader_section_region("Loader section", object_table_offset, loader_section_size, 8);
	loader_section_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(8), offset_t(loader_section_checksum));
	loader_section_region.Display(dump);

	Dumper::Region object_table_region("Object table", object_table_offset, objects.size() * 24, 8);
	object_table_region.Display(dump);

	offset_t i = 0;
	for(auto& object : objects)
	{
		Dumper::Region object_region("Object", object_table_offset + i * 24, 24, 8);
		object_region.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		object_region.AddOptionalField("File size", Dumper::HexDisplay::Make(8), offset_t(object.image ? object.image->ImageSize() : 0));
		object_region.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(object.size));
		object_region.AddField("Base address", Dumper::HexDisplay::Make(8), offset_t(object.address));
		static const std::map<offset_t, std::string> type_descriptions =
		{
			{ 1, "contains zero filled pages" },
			{ 2, "resident" },
			{ 3, "resident and contiguous" },
		};
		object_region.AddField("Flags",
			Dumper::BitFieldDisplay::Make()
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("readable"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("writable"), true)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("executable"), true)
				->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("resource"), true)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("discardable"), true)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("shared"), true)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("contains preload pages"), true)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("contains invalid pages"), true)
				->AddBitField(8, 2, Dumper::ChoiceDisplay::Make(type_descriptions), true)
				->AddBitField(10, 1, Dumper::ChoiceDisplay::Make("resident and long-lockable"), true)
				->AddBitField(11, 1, Dumper::ChoiceDisplay::Make("IBM Microkernel extension"), true)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("16:16 alias required"), true)
				->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("big/default bit set"), true)
				->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("conforming code segment"), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("I/O privilege level"), true),
			offset_t(object.flags));
		object_region.AddField("Page table index", Dumper::HexDisplay::Make(8), offset_t(object.page_table_index));
		object_region.AddField("Page table entry count", Dumper::DecDisplay::Make(), offset_t(object.page_entry_count));
		object_region.Display(dump);
		i++;
	}

	Dumper::Region object_page_table_region("Object page table", object_page_table_offset, page_count * (IsExtendedFormat() ? 8 : 4), 8);
	object_page_table_region.AddField("Page count", Dumper::HexDisplay::Make(8), offset_t(page_count));
	object_page_table_region.AddField("Page size", Dumper::HexDisplay::Make(8), offset_t(page_size));
	object_page_table_region.AddField("Iterated pages offset", Dumper::HexDisplay::Make(8), offset_t(object_iterated_pages_offset));
	// TODO
	object_page_table_region.Display(dump);

	if(resource_table_entry_count != 0)
	{
		Dumper::Region resource_table_region("Resource table", resource_table_offset, resource_table_entry_count * 14, 8);
		// TODO
		resource_table_region.Display(dump);

		// TODO: resources
	}

	Dumper::Region resident_name_table_region("Resident name table", resident_name_table_offset, entry_table_offset - resident_name_table_offset, 8);
	// TODO
	resident_name_table_region.Display(dump);

	// TODO: resident_names

	Dumper::Region entry_table_region("Entry table", entry_table_offset, 0 /* TODO */, 8);
	// TODO
	entry_table_region.Display(dump);

	// TODO: entries

	if(module_directives.size() != 0)
	{
		Dumper::Region module_directives_table_region("Module format directives table", module_directives_offset, module_directives.size() * 8, 8);
		// TODO
		module_directives_table_region.Display(dump);

		// TODO: module_directives
	}

	if(per_page_checksum_offset != 0)
	{
		Dumper::Region perpage_checksum_table_region("Per-page checksum", per_page_checksum_offset, page_count * 4, 8);
		// TODO
		perpage_checksum_table_region.Display(dump);
	}

	Dumper::Region fixup_section_region("Fixup section", fixup_page_table_offset, fixup_section_size, 8);
	fixup_section_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(8), offset_t(fixup_section_checksum));
	// TODO
	fixup_section_region.Display(dump);

	Dumper::Region fixup_page_table_region("Fixup page table", fixup_page_table_offset, (page_count + 1) * 4, 8);
	// TODO
	fixup_page_table_region.Display(dump);

	Dumper::Region fixup_record_table_region("Fixup record table", fixup_record_table_offset, imported_module_table_offset - fixup_record_table_offset, 8);
	// TODO
	fixup_record_table_region.Display(dump);

	Dumper::Region import_module_name_table_region("Import module name table", imported_module_table_offset, imported_procedure_table_offset - imported_module_table_offset, 8);
	// TODO
	import_module_name_table_region.Display(dump);

	// TODO: imported_modules

	offset_t imported_procedure_table_end;
	if(!IsExtendedFormat() && per_page_checksum_offset != 0)
		imported_procedure_table_end = per_page_checksum_offset;
	else
		imported_procedure_table_end = fixup_page_table_offset + fixup_section_size;
	Dumper::Region import_procedure_name_table_region("Import procedure name table", imported_procedure_table_offset, imported_procedure_table_end - imported_procedure_table_offset, 8);
	// TODO
	import_procedure_name_table_region.Display(dump);

	// TODO: imported_procedures

	Dumper::Region page_data_region("Page data", data_pages_offset, 0 /* TODO */, 8);
	page_data_region.AddField("Preload page count", Dumper::DecDisplay::Make(), offset_t(preload_page_count));
	page_data_region.AddField("Instance preload page count", Dumper::DecDisplay::Make(), offset_t(instance_preload_page_count));
	page_data_region.AddField("Instance demand page count", Dumper::DecDisplay::Make(), offset_t(instance_demand_page_count));
	// TODO
	page_data_region.Display(dump);

	i = 1;
	for(const Page& page : pages)
	{
		if(&page == &pages.front() || &page == &pages.back())
			continue;

		offset_t object_number = 0;
		for(auto& object : objects)
		{
			if(object.page_table_index <= i && i < object.page_table_index + object.page_entry_count)
				break;
			object_number++;
		}

		Dumper::Block page_block("Page", GetPageOffset(i), page.image->AsImage(),
			object_number < objects.size() ? objects[object_number].address + (i - objects[object_number].page_table_index) * page_size : 0,
			8);
		page_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		page_block.AddField("Object", Dumper::DecDisplay::Make(), offset_t(object_number + 1));
		static const std::map<offset_t, std::string> flags_descriptions =
		{
			{ 0, "legal physical page" },
			{ 1, "iterated page" },
			{ 2, "illegal page" },
			{ 3, "zero filled page" },
			{ 4, "range of pages" },
			{ 5, "compressed page" },
		};
		if(IsExtendedFormat())
		{
			page_block.AddField("Flags", Dumper::ChoiceDisplay::Make(flags_descriptions), offset_t(page.lx.flags));
		}
		else
		{
			// TODO: format unclear
		}
		page_block.AddField("Fixup offset", Dumper::HexDisplay::Make(8), offset_t(fixup_page_table_offset + page.fixup_offset));
		// TODO: relocations
		page_block.AddOptionalField("Checksum", Dumper::HexDisplay::Make(8), offset_t(page.checksum));
		page_block.Display(dump);
		i++;
	}

	if(nonresident_name_table_size != 0)
	{
		Dumper::Region nonresident_name_table_region("Non-resident name table", nonresident_name_table_offset, nonresident_name_table_size, 8);
		nonresident_name_table_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(8), offset_t(nonresident_name_table_checksum));
		// TODO
		nonresident_name_table_region.Display(dump);

		// TODO: nonresident_names
	}

	if(debug_info_size != 0)
	{
		Dumper::Region debug_info_region("Debug info", debug_info_offset, debug_info_size, 8);
		// TODO
		debug_info_region.Display(dump);
	}
}

offset_t LEFormat::GetPageOffset(uint32_t index) const
{
	return
		IsExtendedFormat()
		? data_pages_offset + pages[index].lx.offset // TODO: iterated page offset
		: data_pages_offset + page_size * (pages[index].le.fixup_table_index - 1);
}

offset_t LEFormat::GetPageSize(uint32_t index) const
{
	return
		IsExtendedFormat()
		? pages[index].lx.size
		: index == page_count
			? last_page_size
			: page_size;
}

std::shared_ptr<LEFormat> LEFormat::CreateConsoleApplication(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, GUIAware, true);
	case Windows386:
		return std::make_shared<LEFormat>(system, Library | NoExternalFixup, false); /* TODO: actually, a driver */
	case DOS4G: /* not an actual type */
		return std::make_shared<LEFormat>(OS2, GUIAware, false);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::CreateGUIApplication(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, GUI, true);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::CreateLibraryModule(system_type system)
{
	switch(system)
	{
	case OS2:
		return std::make_shared<LEFormat>(system, Library | NoInternalFixup, true);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::shared_ptr<LEFormat> LEFormat::SimulateLinker(compatibility_type compatibility)
{
	this->compatibility = compatibility;
	switch(compatibility)
	{
	case CompatibleNone:
		break;
	case CompatibleWatcom:
		break;
	case CompatibleMicrosoft:
		/* TODO */
		break;
	case CompatibleGNU:
		/* TODO */
		break;
	/* TODO: others */
	}
	return shared_from_this();
}

bool LEFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool LEFormat::FormatSupportsLibraries() const
{
	return true;
}

unsigned LEFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		return Linker::Section::Stack;
	}
	else if(section_name == ".heap" || section_name.rfind(".heap.", 0) == 0)
	{
		return Linker::Section::Heap;
	}
	else
	{
		return 0;
	}
}

LEFormat::Resource& LEFormat::AddResource(Resource& resource)
{
	return resources[resource.type][resource.name] = resource;
}

void LEFormat::GetRelocationOffset(Object& object, size_t offset, size_t& page_index, uint16_t& page_offset)
{
	page_index = object.page_table_index + (offset - object.address) / page_size;
	page_offset = offset & (page_size - 1);
}

void LEFormat::AddRelocation(Object& object, unsigned type, unsigned flags, size_t offset, uint16_t module, uint32_t target, uint32_t addition)
{
	size_t page_index;
	uint16_t page_offset;
	GetRelocationOffset(object, offset, page_index, page_offset);
	Page::Relocation rel = Page::Relocation(type, flags, page_offset, module, target, addition);
#if 0
	Linker::Debug << "Debug: PAGES[" << page_index << "] <- " << page_offset << std::endl;
	for(auto& it : pages[page_index].relocations)
	{
		Linker::Debug << "Debug:\t" << it.first << "!" << std::endl;
	}
#endif
	pages[page_index].relocations[page_offset] = rel;
	if(page_offset + rel.GetSourceSize() > page_size)
	{
		/* crosses page boundaries */
		rel.DecrementSingleSourceOffset(page_size);
		pages[page_index + 1].relocations[page_offset - page_size] = rel;
	}
}

unsigned LEFormat::GetDefaultObjectFlags() const
{
	if(IsExtendedFormat())
		return 0;
	else
		return Object::PreloadPages;
}

void LEFormat::AddObject(const Object& object)
{
	objects.push_back(object);
	std::shared_ptr<Linker::Segment> image = std::dynamic_pointer_cast<Linker::Segment>(object.image);
	object_index[image] = objects.size() - 1;
	objects.back().size = image->TotalSize();
	objects.back().address = image->base_address;
}

uint16_t LEFormat::FetchImportedModuleName(std::string name)
{
	auto it = std::find(imported_modules.begin(), imported_modules.end(), name);
	if(it == imported_modules.end())
	{
		uint16_t offset = imported_modules.size() + 1;
		Linker::Debug << "Debug: New imported module name: " << name << " = " << offset << std::endl;
		imported_modules.push_back(name);
		return offset;
	}
	else
	{
		return it - imported_modules.begin() + 1;
	}
}

uint16_t LEFormat::FetchImportedProcedureName(std::string name)
{
	auto it = imported_procedure_name_offsets.find(name);
	if(it == imported_procedure_name_offsets.end())
	{
		uint16_t offset = imported_procedure_names_length;
		Linker::Debug << "Debug: New imported procedure name: " << name << " = " << offset << std::endl;
		imported_procedures.push_back(name);
		imported_procedure_name_offsets[name] = offset;
		imported_procedure_names_length += 1 + name.size();
		return offset;
	}
	else
	{
//Linker::Debug << "Debug: Procedure " << name << " is " << it->second << std::endl;
		return it->second;
	}
}

uint16_t LEFormat::MakeEntry(Linker::Position value)
{
	uint16_t index;
	for(index = 0; index < entries.size(); index ++)
	{
		if(entries[index].type == Entry::Unused)
		{
			break;
		}
	}
	return MakeEntry(index, value);
}

uint16_t LEFormat::MakeEntry(uint16_t index, Linker::Position value)
{
	if(index >= entries.size())
	{
		entries.resize(index + 1);
	}
	if(entries[index].type != Entry::Unused)
	{
		Linker::Error << "Error: entry #" << (index + 1) << " is redefined, ignoring" << std::endl;
		return index;
	}
	uint16_t object = object_index[value.segment];
	/* TODO: other flags? */
	entries[index] = Entry(Entry::Entry32, object + 1, Entry::Exported, value.address - value.segment->base_address); /* TODO: 16-bit */
	return index;
}

uint8_t LEFormat::CountBundles(size_t entry_index) const
{
	size_t entry_count;
	for(entry_count = 1; entry_count < 0xFF && entry_index + entry_count < entries.size(); entry_count++)
	{
		if(!entries[entry_index].SameBundle(entries[entry_index + entry_count]))
		{
			break;
		}
	}
	return entry_count;
}

static Linker::OptionDescription<offset_t> p_base_address("base_address", "Starting address of binary image");
static Linker::OptionDescription<offset_t> p_align("align", "Alignment of memory objects");

std::vector<Linker::OptionDescription<void> *> LEFormat::ParameterNames =
{
	&p_base_address,
	&p_align,
};

std::vector<Linker::OptionDescription<void> *> LEFormat::GetLinkerScriptParameterNames()
{
	return ParameterNames;
}

std::shared_ptr<Linker::OptionCollector> LEFormat::GetOptions()
{
	return std::make_shared<LEOptionCollector>();
}

void LEFormat::SetOptions(std::map<std::string, std::string>& options)
{
	LEOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
	/* TODO */
}

void LEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	/* TODO: stack, heap */

	if(segment->name == ".heap")
	{
		heap = segment;
	}
	else if(segment->name == ".stack")
	{
		stack = segment;
	}
	else
	{
		std::shared_ptr<Linker::Section> section = segment->sections[0];
		unsigned flags = GetDefaultObjectFlags() | Object::BigSegment;
		if(section->IsReadable())
			flags |= Object::Readable;
		if(section->IsWritable())
			flags |= Object::Writable;
		if(section->IsExecutable())
			flags |= Object::Executable;
		AddObject(Object(segment, flags));

		if(segment->name == ".data")
			automatic_data = objects.size();
	}
}

std::unique_ptr<Script::List> LEFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?base_address?;
	all ".code" or ".text" or ".rodata" or ".eh_frame";
};

".data"
{
	at align(here, ?align?);
	all ".data";
	all ".bss" or ".comm";
};

for ".heap"
{
	at align(here, ?align?);
	all any;
};

for not ".stack"
{
	at align(here, ?align?);
	all any;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void LEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void LEFormat::ProcessModule(Linker::Module& module)
{
	if(system == Windows386)
	{
		linker_parameters["align"] = Linker::Location(0x1000);
		linker_parameters["base_address"] = Linker::Location();
	}
	else
	{
		linker_parameters["align"] = Linker::Location(0x10000);
		linker_parameters["base_address"] = Linker::Location(0x10000);
	}

	Link(module);

	resident_names.push_back(Name{module_name, 0});
	FetchImportedProcedureName("");

	offset_t page_offset = 0;
	pages.push_back(Page());
	for(Object& object : objects)
	{
		offset_t object_offset = 0;
		object.page_table_index = pages.size();
//		Linker::Debug << "Debug: " << object.image->data_size << " / " << page_size << std::endl;
		offset_t object_size = object.image->ImageSize();
		object.page_entry_count = ::AlignTo(object_size, page_size) / page_size;

		for(size_t i = 0; i < object.page_entry_count; i++)
		{
			if(IsExtendedFormat())
			{
				uint32_t current_page_size;
				if(i == object.page_entry_count - 1)
				{
					current_page_size = object_size & (page_size - 1);
					if(current_page_size == 0)
						current_page_size = page_size;
				}
				else
				{
					current_page_size = page_size;
				}
				pages.push_back(Page::LXPage(page_offset, current_page_size, Page::Preload));
				pages.back().image = std::make_shared<SegmentPage>(object.image, object_offset, current_page_size);
				page_offset += current_page_size;
				object_offset += current_page_size;
			}
			else
			{
				uint32_t fixup_table_index = pages.size();
				pages.push_back(Page::LEPage(fixup_table_index, 0)); //Page::Relocations));
				pages.back().image = std::make_shared<SegmentPage>(object.image, page_size * (fixup_table_index - object.page_table_index),
					&object == &objects.back() && i == object.page_entry_count - 1 ? object_size & (page_size - 1) : page_size);
				/* TODO: change fixup_table_index to 0 unless a relocation is present */
				/* Idea: set fixup_table_index after going through all relocations */
			}
		}
	}

	pages.push_back(Page());

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string library;
		if(symbol.LoadLibraryName(library))
		{
			FetchImportedModuleName(library);
		}
	}

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string name;
		if(symbol.LoadName(name))
		{
			FetchImportedProcedureName(name);
		}
	}

	/* first make entries for those symbols exported by ordinals, or those that have hints */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(it.first.LoadOrdinalOrHint(ordinal))
		{
			MakeEntry(ordinal - 1, it.second.GetPosition());
			if(it.first.IsExportedByOrdinal())
			{
				std::string internal_name = "";
				it.first.LoadName(internal_name);
				nonresident_names.push_back(Name{internal_name, ordinal});
			}
		}
	}

	/* then make entries for those symbols exported by name */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(!it.first.LoadOrdinalOrHint(ordinal))
		{
			ordinal = MakeEntry(it.second.GetPosition()) + 1;
		}
		if(!it.first.IsExportedByOrdinal())
		{
			std::string exported_name = "";
			it.first.LoadName(exported_name);
			resident_names.push_back(Name{exported_name, ordinal});
		}
	}

	for(Linker::Relocation& rel : module.relocations)
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
				Linker::Position position = rel.source.GetPosition();
				Object& source_object = objects[object_index[position.segment]];
				uint8_t target_object = object_index[resolution.target] + 1;
				int type = Page::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				if(type == Page::Relocation::Selector16)
				{
					/* Segment relocation to internal reference 0:0 */
					AddRelocation(source_object, type, Page::Relocation::Internal, position.address, target_object);
					rel.WriteWord(0);
					continue;
				}
				else if(!(IsOS2() && !IsLibrary()))
				{
					AddRelocation(source_object, type, Page::Relocation::Internal,
							position.address, target_object, resolution.value - resolution.target->base_address);
					if(IsOS2())
						rel.WriteWord(resolution.value);
					else
						rel.WriteWord(resolution.value - resolution.target->base_address);
				}
				else
				{
					/* OS/2 application */
					rel.WriteWord(resolution.value);
				}
				/* TODO: if not additive? */
			}
			else
			{
				rel.WriteWord(resolution.value);
			}
		}
		else
		{
			if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
			{
				Linker::Position position = rel.source.GetPosition();
				Object& source_object = objects[object_index[position.segment]];

				std::string library, name;
				uint16_t ordinal;
				int type = Page::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				else if(type == Page::Relocation::Selector16)
				{
					/* Segment relocation to internal reference 0:0 */
					AddRelocation(source_object, type, Page::Relocation::Internal, position.address, 1);
					rel.WriteWord(0);
					continue;
				}
				if(symbol->GetImportedName(library, name))
				{
					AddRelocation(source_object, type, Page::Relocation::ImportName,
						position.address, FetchImportedModuleName(library), FetchImportedProcedureName(name));
					rel.WriteWord(rel.IsRelative() ? 0 : resolution.value);
					continue;
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					AddRelocation(source_object, type, Page::Relocation::ImportOrdinal,
						position.address, FetchImportedModuleName(library), ordinal);
					rel.WriteWord(rel.IsRelative() ? 0 : resolution.value);
					continue;
				}
			}
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
	}

	/* TODO: allocate stack instead */
	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		esp_object = object_index[position.segment] + 1;
		esp_value = position.address - position.segment->base_address;
	}
	else if(automatic_data != 0 && module.FindSection(".stack") != nullptr && module.FindSection(".stack")->Size() != 0)
	{
		esp_object = automatic_data;
		esp_value = objects[automatic_data - 1].size;
	}
	else if(automatic_data != 0 && system == OS2 && !IsLibrary())
	{
		if(system == OS2 && !IsLibrary())
		{
			offset_t stack_size = 0x1000; /* TODO: make into a parameter */
			std::dynamic_pointer_cast<Linker::Segment>(objects[automatic_data - 1].image)->zero_fill += stack_size; // not important, just for consistency
			objects[automatic_data - 1].size += stack_size;
		}
		esp_object = automatic_data;
		esp_value = objects[automatic_data - 1].size;
	}
	else
	{
		/* top of automatic data segment */
		esp_object = automatic_data;
		esp_value = 0;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		eip_object = object_index[position.segment] + 1;
		eip_value = position.address - position.segment->base_address;
	}
	else
	{
		eip_object = 1;
		eip_value = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void LEFormat::CalculateValues()
{
	/* TODO: where does the stack value come from? */
	switch(system)
	{
	case OS2:
		stack_size = 0x1000;
		heap_size = 0;
		break;
	case Windows386:
		stack_size = 0;
		heap_size = 0;
		break;
	default:
		{
			std::ostringstream message;
			message << "Fatal error: " << system;
			Linker::FatalError(message.str());
		}
	}

	if(IsLibrary() || IsDriver())
		stack_size = esp_object = esp_value = 0;

	if(system == Windows386 && compatibility == CompatibleWatcom)
	{
		vxd_ddk_version = 0x4; /* TODO: Watcom? */
	}

	/* TODO */

	if(IsExtendedFormat())
	{
		page_offset_shift = 0;
	}
	else
	{
		last_page_size = objects.back().image->ImageSize() & (page_size - 1);
#if 0
		for(Object& object : objects)
		{
			Linker::Debug << "Debug: " << object.image->name << " -> " << object.image->data_size << std::endl;
		}
#endif
	}

	file_offset = stub.GetStubImageSize();

	object_table_offset = file_offset + 0xC4;

	object_page_table_offset = object_table_offset + 0x18 * objects.size();
	object_iterated_pages_offset = 0;

	resource_table_offset = object_page_table_offset + (IsExtendedFormat() ? 8 : 4) * (pages.size() - 2);
	resource_table_entry_count = 0; /* TODO: resources */
	uint32_t resource_table_size = 0; /* TODO */
	resident_name_table_offset = resource_table_offset + resource_table_size;
	uint32_t resident_name_table_length = 1;
	for(Name& name : resident_names)
	{
		resident_name_table_length += 3 + name.name.size();
	}
	entry_table_offset = resident_name_table_offset + resident_name_table_length;
	uint32_t entry_table_length = 1; /* terminating zero */
//offset_t _ = entry_table_length;
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		entry_table_length += entries[entry_index].GetEntryHeadSize() + entry_count * entries[entry_index].GetEntryBodySize();
//Linker::Debug << "Debug: Count " << entry_table_length - _ << std::endl; _ = entry_table_length;
		entry_index += entry_count;
	}

	fixup_page_table_offset = entry_table_offset + entry_table_length;
	fixup_record_table_offset = fixup_page_table_offset + 4 * (pages.size() - 1); /* including terminator entry (first entry fake because pages are 1 based) */

	offset_t fixup_offset = 0;
	page_count = pages.size() - 2;
	for(Page& page : pages)
	{
		if(&page == &pages.front())
			continue;
		page.fixup_offset = fixup_offset;
		if(&page == &pages.back())
			continue;
//Linker::Debug << "Debug: Expect page " << page.relocations.size() << std::endl;
		for(auto it : page.relocations)
		{
			it.second.CalculateSizes(compatibility);
			fixup_offset += it.second.GetSize();
//Linker::Debug << "Debug: Expect " << it.second.GetSize() << std::endl;
		}
	}

	imported_module_table_offset = fixup_record_table_offset + fixup_offset;

	uint32_t imported_module_table_length = 0;
	for(std::string& name : imported_modules)
	{
		imported_module_table_length += 1 + name.size();
	}

	imported_procedure_table_offset = imported_module_table_offset + imported_module_table_length;

	uint32_t imported_procedure_table_length = 0;
	for(std::string& name : imported_procedures)
	{
		imported_procedure_table_length += 1 + name.size();
	}

	uint32_t imported_procedure_table_end = imported_procedure_table_offset + imported_procedure_table_length;

	loader_section_size = fixup_page_table_offset - object_table_offset;
	fixup_section_size = imported_procedure_table_end - fixup_page_table_offset;

	data_pages_offset = imported_procedure_table_end;
	if(compatibility == CompatibleWatcom && (page_offset_shift < 2 || !IsExtendedFormat()))
	{
		data_pages_offset = ::AlignTo(data_pages_offset, 4); /* TODO: Watcom? */
	}
	else if(IsExtendedFormat())
	{
		data_pages_offset = ::AlignTo(data_pages_offset, 1 << page_offset_shift);
	}

	uint32_t pages_offset = data_pages_offset;
	for(Object& object : objects)
	{
		if(!IsExtendedFormat())
		{
			pages_offset = ::AlignTo(pages_offset - data_pages_offset, page_size) + data_pages_offset;
		}
		object.data_pages_offset = pages_offset;
		pages_offset += object.image->ImageSize();
	}

	if(nonresident_names.size() == 0)
	{
		nonresident_name_table_offset = 0;
		nonresident_name_table_size = 0;
	}
	else
	{
		nonresident_name_table_offset = pages_offset;
		nonresident_name_table_size = 1;
		for(Name& name : nonresident_names)
		{
			nonresident_name_table_size += 3 + name.name.size();
		}
	}

	file_size = pages_offset + nonresident_name_table_size;
}

void LEFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	program_name = filename;
	size_t ix = filename.rfind('.');
	if(ix != std::string::npos)
		module_name = filename.substr(0, ix);
	else
		module_name = filename;
	if(module.cpu != Linker::Module::I386)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 80386 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string LEFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe";
}

