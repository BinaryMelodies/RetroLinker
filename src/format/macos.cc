
#include <cstring>
#include "macos.h"
#include "../linker/options.h"
#include "../linker/position.h"
#include "../linker/reader.h"
#include "../linker/resolution.h"
#include "../linker/section.h"

using namespace Apple;

uint32_t Apple::OSTypeToUInt32(const OSType& type)
{
	return
		(uint32_t(uint8_t(type[0])) << 24)
		| (uint32_t(uint8_t(type[1])) << 16)
		| (uint32_t(uint8_t(type[2])) << 8)
		| uint32_t(uint8_t(type[3]));
}

void Apple::UInt32ToOSType(OSType& type, uint32_t value)
{
	type[0] = value >> 24;
	type[1] = value >> 16;
	type[2] = value >> 8;
	type[3] = value;
}

// MacintoshResourceFileFormat

void MacintoshResourceFileFormat::SetOptions(std::map<std::string, std::string>& options)
{
	/* TODO */
}

std::vector<Linker::OptionDescription<void>> MacintoshResourceFileFormat::MemoryModelNames =
{
	Linker::OptionDescription<void>("default", "Normal model, symbols in zero-filled sectioned must be accessed as A5 relative addresses (\"A5 world\")"),
	Linker::OptionDescription<void>("tiny", "Tiny model, symbols in zero-filled sections are placed in the CODE segment"),
};

std::vector<Linker::OptionDescription<void>> MacintoshResourceFileFormat::GetMemoryModelNames()
{
	return MemoryModelNames;
}

void MacintoshResourceFileFormat::SetModel(std::string model)
{
	if(model == "" || model == "default")
	{
		Linker::Debug << "Debug: new memory model: default" << std::endl;
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "tiny")
	{
		Linker::Debug << "Debug: new memory model: tiny" << std::endl;
		memory_model = MODEL_TINY;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_DEFAULT;
	}
}

void MacintoshResourceFileFormat::Resource::Dump(Dumper::Dumper& dump) const
{
	Dump(dump, 0);
}

int MacintoshResourceFileFormat::Resource::GetDisplayOptions() const
{
	return Dumper::Resource;
}

void MacintoshResourceFileFormat::Resource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	std::unique_ptr<Dumper::Region> resource_region = CreateRegion("Resource", file_offset, ImageSize(), 8);
	resource_region->AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type, 4));
	resource_region->AddField("ID", Dumper::HexDisplay::Make(4), offset_t(id));
	if(name)
		resource_region->AddField("Name", Dumper::StringDisplay::Make("\""), *name);
	resource_region->AddField("Attributes", Dumper::HexDisplay::Make(2), offset_t(attributes));
	AddFields(dump, *resource_region, file_offset);
	resource_region->Display(dump, GetDisplayOptions());
}

void MacintoshResourceFileFormat::Resource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
}

std::unique_ptr<Dumper::Region> MacintoshResourceFileFormat::Resource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Region>(name, offset, length, display_width);
}

void MacintoshResourceFileFormat::GenericResource::CalculateValues()
{
}

offset_t MacintoshResourceFileFormat::GenericResource::ImageSize() const
{
	return image->ImageSize();
}

void MacintoshResourceFileFormat::GenericResource::ReadFile(Linker::Reader& rd)
{
	image = Linker::Buffer::ReadFromFile(rd);
}

void MacintoshResourceFileFormat::GenericResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	image = Linker::Buffer::ReadFromFile(rd, length);
}

offset_t MacintoshResourceFileFormat::GenericResource::WriteFile(Linker::Writer& wr) const
{
	return image->WriteFile(wr);
}

std::unique_ptr<Dumper::Region> MacintoshResourceFileFormat::GenericResource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset, image->AsImage(), 0, display_width);
}

void MacintoshResourceFileFormat::JumpTableCodeResource::CalculateValues()
{
	jump_table_offset = 32;
	if(far_entries.size() == 0)
	{
		above_a5 = 0x30 + 8 * near_entries.size();
	}
	else
	{
		above_a5 = 0x30 + 8 + 8 * (near_entries.size() + far_entries.size());
	}
}

offset_t MacintoshResourceFileFormat::JumpTableCodeResource::ImageSize() const
{
	if(far_entries.size() == 0)
	{
		return 16 + 8 * near_entries.size();
	}
	else
	{
		return 24 + 8 * (near_entries.size() + far_entries.size());
	}
}

void MacintoshResourceFileFormat::JumpTableCodeResource::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void MacintoshResourceFileFormat::JumpTableCodeResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	above_a5 = rd.ReadUnsigned(4);
	below_a5 = rd.ReadUnsigned(4);
	uint32_t total_entry_size = rd.ReadUnsigned(4);
	jump_table_offset = rd.ReadUnsigned(4);
	uint32_t i;
	for(i = 0; i < total_entry_size; i += 8)
	{
		Entry entry;
		entry.offset = rd.ReadUnsigned(2);
		uint16_t _move_data_sp = rd.ReadUnsigned(2); // MOVE_DATA_SP
		entry.segment = rd.ReadUnsigned(2);
		uint16_t _loadseg = rd.ReadUnsigned(2); // LOADSEG
		if(entry.offset == 0 && _move_data_sp == 0xFFFF && entry.segment == 0 && _loadseg == 0)
		{
			break;
		}
		near_entries.push_back(entry);
	}

	for(; i < total_entry_size; i += 8)
	{
		Entry entry;
		entry.segment = rd.ReadUnsigned(2);
		rd.Skip(2); // LOADSEG
		entry.offset = rd.ReadUnsigned(4);
		far_entries.push_back(entry);
	}
}

offset_t MacintoshResourceFileFormat::JumpTableCodeResource::WriteFile(Linker::Writer& wr) const
{
	wr.WriteWord(4, above_a5);
	wr.WriteWord(4, below_a5);
	if(far_entries.size() == 0)
	{
		wr.WriteWord(4, 8 * near_entries.size());
	}
	else
	{
		wr.WriteWord(4, 8 + 8 * (near_entries.size() + far_entries.size()));
	}
	wr.WriteWord(4, jump_table_offset);
	for(const Entry& entry : near_entries)
	{
		wr.WriteWord(2, entry.offset);
		wr.WriteWord(2, MOVE_DATA_SP);
		wr.WriteWord(2, entry.segment);
		wr.WriteWord(2, LOADSEG);
	}
	if(far_entries.size() != 0)
	{
		wr.WriteData(8, "\0\0\xFF\xFF\0\0\0\0");
		for(const Entry& entry : far_entries)
		{
			wr.WriteWord(2, entry.segment);
			wr.WriteWord(2, LOADSEG);
			wr.WriteWord(4, entry.offset);
		}
	}

	return ImageSize();
}

void MacintoshResourceFileFormat::JumpTableCodeResource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
	region.AddField("Above A5", Dumper::HexDisplay::Make(8), offset_t(above_a5));
	region.AddField("Below A5", Dumper::HexDisplay::Make(8), offset_t(below_a5));
	offset_t jump_table_size;
	if(far_entries.size() == 0)
	{
		jump_table_size = 8 * near_entries.size();
	}
	else
	{
		jump_table_size = 8 + 8 * (near_entries.size() + far_entries.size());
	}
	region.AddField("Jump table size", Dumper::HexDisplay::Make(8), offset_t(jump_table_size));
	region.AddField("Jump table offset", Dumper::HexDisplay::Make(8), offset_t(32));
}

int MacintoshResourceFileFormat::JumpTableCodeResource::GetDisplayOptions() const
{
	return Dumper::Header | Dumper::Image | Dumper::Export; // not technically exported, entries behave similarly to export tables
}

void MacintoshResourceFileFormat::JumpTableCodeResource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	Resource::Dump(dump, file_offset);

	unsigned i = 0;
	for(auto& entry : near_entries)
	{
		Dumper::Entry entry_entry("Entry", i + 1, file_offset + 16 + i * 8);
		entry_entry.AddField("Type", Dumper::ChoiceDisplay::Make("near"), offset_t(true));
		entry_entry.AddField("Value", Dumper::SegmentedDisplay::Make(4), offset_t(entry.segment), offset_t(entry.offset));
		entry_entry.Display(dump, GetDisplayOptions());
		i++;
	}

	// skip one value for separator
	i++;

	for(auto& entry : far_entries)
	{
		Dumper::Entry entry_entry("Entry", i + 1, file_offset + 16 + i * 8);
		entry_entry.AddField("Type", Dumper::ChoiceDisplay::Make("near"), offset_t(true));
		entry_entry.AddField("Value", Dumper::SegmentedDisplay::Make(8), offset_t(entry.segment), offset_t(entry.offset));
		entry_entry.Display(dump, GetDisplayOptions());
		i++;
	}
}

void MacintoshResourceFileFormat::CodeResource::CalculateValues()
{
	if(Linker::Segment * segment = dynamic_cast<Linker::Segment *>(image.get()))
	{
		zero_fill = segment->zero_fill;
	}

	near_entry_count = near_entries.size();
	far_entry_count = far_entries.size();
	if(!is_far)
	{
		resource_size = 4 + image->ImageSize() + zero_fill;
	}
	else
	{
		a5_relocation_offset = 0x28 + ImageSize() + zero_fill;
		segment_relocation_offset = a5_relocation_offset + MeasureRelocations(a5_relocations);
		resource_size = segment_relocation_offset + MeasureRelocations(segment_relocations);
	}
}

offset_t MacintoshResourceFileFormat::CodeResource::ImageSize() const
{
	return resource_size;
}

uint32_t MacintoshResourceFileFormat::CodeResource::MeasureRelocations(std::set<uint32_t>& relocations) const
{
	uint32_t count = 2;
	uint32_t last_relocation = 0;
	for(uint32_t relocation : relocations)
	{
		uint32_t offset = relocation - last_relocation;
		if(offset < 0x100)
		{
			count ++;
		}
		else if(offset < 0x10000)
		{
			count += 2;
		}
		else
		{
			count += 5;
		}
		last_relocation = relocation;
	}
	return count;
}

void MacintoshResourceFileFormat::CodeResource::ReadRelocations(Linker::Reader& rd, std::set<uint32_t>& relocations) const
{
	/* TODO: test */
	uint32_t last_relocation = 0;
	while(true)
	{
		uint32_t offset = rd.ReadUnsigned(1);
		if(offset == 0)
		{
			if((rd.ReadUnsigned(1) & 0x80) == 0)
			{
				break;
			}
			rd.Skip(-1);
			offset = rd.ReadUnsigned(4) & ~0x80000000;
		}
		else if((offset & 0x80) != 0)
		{
			rd.Skip(-1);
			offset = rd.ReadUnsigned(2) & ~0x8000;
		}
		last_relocation += offset << 1;
		relocations.insert(last_relocation);
	}
}

void MacintoshResourceFileFormat::CodeResource::WriteRelocations(Linker::Writer& wr, const std::set<uint32_t>& relocations) const
{
	/* TODO: test */
	uint32_t last_relocation = 0;
	for(uint32_t relocation : relocations)
	{
		uint32_t offset = relocation - last_relocation;
		if(offset < 0x100)
		{
			wr.WriteWord(1, offset >> 1);
		}
		else if(offset < 0x10000)
		{
			wr.WriteWord(2, 0x8000 | (offset >> 1));
		}
		else
		{
			wr.WriteWord(1, 0);
			wr.WriteWord(4, 0x80000000 | (offset >> 1));
		}
		last_relocation = relocation;
	}
	wr.WriteWord(2, 0);
}

void MacintoshResourceFileFormat::CodeResource::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void MacintoshResourceFileFormat::CodeResource::ReadFile(Linker::Reader& rd, offset_t length)
{
	resource_size = length;
	first_near_entry_offset = rd.ReadUnsigned(2);
	near_entry_count = rd.ReadUnsigned(2);
	if(first_near_entry_offset == 0xFFFF && near_entry_count == 0)
	{
		is_far = true;
		first_near_entry_offset = rd.ReadUnsigned(4);
		near_entry_count = rd.ReadUnsigned(4);
		first_far_entry_offset = rd.ReadUnsigned(4);
		far_entry_count = rd.ReadUnsigned(4);
		a5_relocation_offset = rd.ReadUnsigned(4);
		a5_address = rd.ReadUnsigned(4);
		segment_relocation_offset = rd.ReadUnsigned(4);
		base_address = rd.ReadUnsigned(4);
		rd.Skip(4);
		image = Linker::Buffer::ReadFromFile(rd, a5_relocation_offset - 0x28);
		ReadRelocations(rd, a5_relocations);
		ReadRelocations(rd, segment_relocations);
	}
	else
	{
		is_far = false;
		image = Linker::Buffer::ReadFromFile(rd, length - 4);
	}
}

offset_t MacintoshResourceFileFormat::CodeResource::WriteFile(Linker::Writer& wr) const
{
	if(!is_far)
	{
		wr.WriteWord(2, first_near_entry_offset);
		wr.WriteWord(2, near_entry_count);
		image->WriteFile(wr);
		wr.Skip(zero_fill);
	}
	else
	{
		wr.WriteData(4, "\xFF\xFF\0\0");
		wr.WriteWord(4, first_near_entry_offset);
		wr.WriteWord(4, near_entry_count);
		wr.WriteWord(4, first_far_entry_offset);
		wr.WriteWord(4, far_entry_count);
		wr.WriteWord(4, a5_relocation_offset);
		wr.WriteWord(4, a5_address);
		wr.WriteWord(4, segment_relocation_offset);
		wr.WriteWord(4, base_address);
		wr.WriteWord(4, 0);
		image->WriteFile(wr);
		WriteRelocations(wr, a5_relocations);
		WriteRelocations(wr, segment_relocations);
	}

	return ImageSize();
}

void MacintoshResourceFileFormat::CodeResource::AddFields(Dumper::Dumper& dump, Dumper::Region& region, offset_t file_offset) const
{
	region.AddField("Type", Dumper::ChoiceDisplay::Make("far", "near"), offset_t(is_far));
	region.AddField("Near entry count", Dumper::DecDisplay::Make(), offset_t(near_entry_count));
	region.AddField("First near entry offset", Dumper::HexDisplay::Make(is_far ? 8 : 4), offset_t(first_near_entry_offset));
	if(is_far)
	{
		region.AddField("Far entry count", Dumper::DecDisplay::Make(), offset_t(far_entry_count));
		region.AddField("First far entry offset", Dumper::HexDisplay::Make(8), offset_t(first_far_entry_offset));
		region.AddField("A5 relocation offset", Dumper::HexDisplay::Make(8), offset_t(a5_relocation_offset));
		region.AddField("A5 value", Dumper::HexDisplay::Make(8), offset_t(a5_address));
		region.AddField("Segment relocation offset", Dumper::HexDisplay::Make(8), offset_t(segment_relocation_offset));
		region.AddField("Segment address", Dumper::HexDisplay::Make(8), offset_t(base_address));
	}

}

int MacintoshResourceFileFormat::CodeResource::GetDisplayOptions() const
{
	return Dumper::Header | Dumper::Image;
}

void MacintoshResourceFileFormat::CodeResource::Dump(Dumper::Dumper& dump, offset_t file_offset) const
{
	Resource::Dump(dump, file_offset);
	// TODO: print relocations
}

std::unique_ptr<Dumper::Region> MacintoshResourceFileFormat::CodeResource::CreateRegion(std::string name, offset_t offset, offset_t length, unsigned display_width) const
{
	return std::make_unique<Dumper::Block>(name, offset + (is_far ? 0x28 : 4), image->AsImage(), base_address, display_width);
}

void MacintoshResourceFileFormat::AddResource(std::shared_ptr<Resource> resource)
{
	uint32_t typeval = OSTypeToUInt32(resource->type);
	resources[typeval][resource->id] = resource;
}

void MacintoshResourceFileFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".a5world")
	{
		a5world = segment;
	}
	else if(segment->sections.size() == 0)
	{
		return;
	}
	else if(!(segment->sections.front()->GetFlags() & Linker::Section::Resource))
	{
		std::shared_ptr<CodeResource> codeN = std::make_shared<CodeResource>(codes.size() + 1, jump_table);
		codeN->image = segment;
		codes.push_back(codeN);
		segments[segment] = codeN;
		AddResource(codeN);
	}
	else
	{
		std::shared_ptr<Linker::Section> section = segment->sections.front();
		/* other resources */
		Linker::ResourceIdentifier_String * type = std::get_if<std::string>(&section->resource_type);
		if(type == nullptr || type->size() != 4)
		{
			Linker::Error << "Error: resources are expected to have a 4-character type" << std::endl;
			return;
		}
		Linker::ResourceIdentifier_Integer * id = std::get_if<Linker::ResourceIdentifier_Integer>(&section->resource_id);
		if(id == nullptr || *id > 0xFFFF)
		{
			Linker::Error << "Error: resources are expected to have a 16-bit ID" << std::endl;
			return;
		}
		Linker::Debug << "Debug: Adding resource type " << *type << ", id " << *id << std::endl;
		std::shared_ptr<GenericResource> rsrc = std::make_shared<GenericResource>(type->c_str(), *id);
//		rsrc->resource = std::make_shared<Linker::Segment>(".rsrc");
//		rsrc->resource->Append(section);
		rsrc->image = segment;
		AddResource(rsrc);
	}
}

std::unique_ptr<Script::List> MacintoshResourceFileFormat::GetScript(Linker::Module& module)
{
	/* TODO: make placing .comm/.bss data inside the .a5world optional */

	static const char * SimpleScript = R"(
".a5world"
{
	all ".comm" align 2;
	all zero and not ".a5world" align 2;
#	all ".globals" align 2; # TODO: not currently used
	all ".a5world" align 2;
	align 2;
} at -size of ".a5world";

".code"
{
	at 0;
	all ".code" or ".text" or ".data" or ".rodata"
		align 2;
	align 2;
};

for not resource
{
	at 0;
	all any
		align 2;
	align 2;
};

# TODO: resources
for any
{
	at 0;
	all any;
};
)";

	static const char * TinyScript = R"(
".a5world"
{
	all ".a5world" align 2;
	align 2;
} at -size of ".a5world";

".code"
{
	at 0;
	all ".code" or ".text" or ".data" or ".rodata"
		align 2;
	all not resource align 2;
	all ".comm" align 2;
	all zero align 2;
	align 2;
};

for any
{
	at 0;
	all any;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		switch(memory_model)
		{
		case MODEL_DEFAULT:
		default:
			return Script::parse_string(SimpleScript);
		case MODEL_TINY:
			return Script::parse_string(TinyScript);
		}
	}
}

void MacintoshResourceFileFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void MacintoshResourceFileFormat::ProcessModule(Linker::Module& module)
{
	jump_table = std::make_shared<JumpTableCodeResource>();
	AddResource(jump_table);

for(auto section : module.Sections())
{
	Linker::Debug << "Debug: " << *section << std::endl;
}
	Link(module);
	jump_table->below_a5 = a5world->zero_fill;
	Linker::Debug << "Debug: Setting the A5 world to " << a5world->zero_fill << std::endl;

	uint32_t entry_offset = 0;
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		if(position.segment != codes[0]->image)
		{
			Linker::Error << "Error: entry point must be in `.code' segment, using offset .code:0 instead" << std::endl;
		}
		else
		{
			entry_offset = position.address;
		}
	}
	/* must be the first entry */
	codes[0]->near_entries.insert(entry_offset);

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(resolution.target != nullptr)
		{
			/* TODO: how do we deal with relocations? */
			/* idea: jsr method_name(a5) can be replaced by an entry */
		}
	}

	/* must be the first entry */
	jump_table->near_entries.push_back(JumpTableCodeResource::Entry{1, entry_offset});

	for(auto& resource : codes)
	{
		/* since the first entry is already loaded, we have to skip it */
		resource->first_near_entry_offset = resource == codes[0] ? 0 : jump_table->near_entries.size() * 8;
		for(uint16_t entry : resource->near_entries)
		{
			if(resource == codes[0] && entry == entry_offset)
				continue; /* already inserted */
			jump_table->near_entries.push_back(JumpTableCodeResource::Entry{1, entry}); /* TODO: segment number */
		}
	}
	for(auto& resource : codes)
	{
		if(resource->far_entries.size() == 0)
			continue;
		resource->first_far_entry_offset = jump_table->far_entries.size() * 8;
		//jump_table->far_entries.insert(jump_table->far_entries.end(), resource->far_entries.begin(), resource->far_entries.end());
		for(uint32_t entry : resource->far_entries)
		{
			jump_table->far_entries.push_back(JumpTableCodeResource::Entry{1, entry}); /* TODO: segment number */
		}
	}
}

void MacintoshResourceFileFormat::CalculateValues()
{
	data_length = 0;
	uint32_t name_list_length = 0;
	uint16_t reference_list_offset = 2 + resources.size() * 8;
	resource_types.clear();
	resource_names.clear();
	for(auto it : resources)
	{
		if(it.second.size() == 0)
			continue;

		ResourceType type;
		UInt32ToOSType(type.type, it.first);
		type.offset = reference_list_offset;
		reference_list_offset += 12 * it.second.size();
		name_list_offset += 12 * it.second.size();
		for(auto it2 : it.second)
		{
			ResourceReference reference;
			reference.id = it2.first;
			std::shared_ptr<Resource> resource = it2.second;
			reference.data = resource;
			reference.data_offset = data_length;
			resource->CalculateValues();
			data_length += 4 + resource->ImageSize();
			if(resource->name)
			{
				resource_names.push_back(*resource->name);
				reference.name_offset = name_list_length;
				name_list_length += 1 + resource->name->size();
			}
			else
			{
				reference.name_offset = 0xFFFF;
			}
			reference.attributes = resource->attributes;
			type.references.push_back(reference);
		}

		resource_types.push_back(type);
	}

	resource_type_list_offset = 28;
	name_list_offset = resource_type_list_offset + reference_list_offset;
	if(data_offset < 16)
		data_offset = 0x0100;
	map_offset = data_offset + data_length;
	map_length = name_list_offset + name_list_length;
}

offset_t MacintoshResourceFileFormat::ImageSize() const
{
	return std::max(data_offset + data_length, map_offset + map_length);
}

void MacintoshResourceFileFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian; /* in case we write the resource fork directly, without an AppleSingle/AppleDouble wrapper */
	offset_t read_offset = rd.Tell();
	data_offset = rd.ReadUnsigned(4);
	map_offset = rd.ReadUnsigned(4);
	data_length = rd.ReadUnsigned(4);
	map_length = rd.ReadUnsigned(4);

	rd.Seek(read_offset + map_offset + 22);
	attributes = rd.ReadUnsigned(2);
	resource_type_list_offset = rd.ReadUnsigned(2);
	name_list_offset = rd.ReadUnsigned(2);

	rd.Seek(read_offset + map_offset + resource_type_list_offset);
	offset_t resource_count = offset_t(rd.ReadUnsigned(2)) + 1;

	/* type list */
	for(offset_t i = 0; i < resource_count; i++)
	{
		ResourceType type;
		rd.ReadData(4, type.type);
		type.count = uint32_t(rd.ReadUnsigned(2)) + 1;
		type.offset = rd.ReadUnsigned(2);
		resource_types.push_back(type);
	}

	/* reference list */
	for(auto& type : resource_types)
	{
		rd.Seek(read_offset + map_offset + resource_type_list_offset + type.offset);
		for(offset_t i = 0; i < type.count; i++)
		{
			ResourceReference reference;
			reference.id = rd.ReadUnsigned(2);
			reference.name_offset = rd.ReadUnsigned(2);
			reference.data_offset = rd.ReadUnsigned(4);
			reference.attributes = reference.data_offset >> 24;
			reference.data_offset &= 0x00FFFFFF;
			rd.Skip(4);
			type.references.push_back(reference);
		}
	}

	/* name list */
	rd.Seek(read_offset + map_offset + name_list_offset);
	// first read all the names
	while(rd.Tell() < read_offset + map_offset + map_length)
	{
		uint8_t size = rd.ReadUnsigned(1);
		resource_names.push_back(rd.ReadData(size));
	}

	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			// read resource data
			rd.Seek(read_offset + data_offset + reference.data_offset);
			reference.data = ReadResource(rd, type, reference);

			// read resource name
			if(reference.name_offset != 0xFFFF)
			{
				rd.Seek(read_offset + map_offset + name_list_offset + reference.name_offset);
				uint8_t size = rd.ReadUnsigned(1);
				reference.name = rd.ReadData(size);
			}

			// register this resource for convenience
			AddResource(reference.data);
		}
	}
}

offset_t MacintoshResourceFileFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian; /* in case we write the resource fork directly, without an AppleSingle/AppleDouble wrapper */
	offset_t write_offset = wr.Tell();
	wr.WriteWord(4, data_offset);
	wr.WriteWord(4, map_offset);
	wr.WriteWord(4, data_length);
	wr.WriteWord(4, map_length);
	/* data start */
	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			std::shared_ptr<Resource> resource = reference.data;
			wr.Seek(write_offset + data_offset + reference.data_offset);
			wr.WriteWord(4, resource->ImageSize());
			resource->WriteFile(wr);
		}
	}
	/* map start */
	wr.Seek(write_offset + map_offset + 22);
	wr.WriteWord(2, attributes);
	wr.WriteWord(2, resource_type_list_offset);
	wr.WriteWord(2, name_list_offset);
	wr.Seek(write_offset + map_offset + resource_type_list_offset);
	wr.WriteWord(2, resources.size() - 1);
	/* type list */
	for(auto& type : resource_types)
	{
		if(type.references.size() == 0)
			continue;
		wr.WriteData(4, type.type);
		wr.WriteWord(2, type.references.size() - 1);
		wr.WriteWord(2, type.offset);
	}
	/* reference list */
	for(auto& type : resource_types)
	{
		wr.Seek(write_offset + map_offset + resource_type_list_offset + type.offset);
		for(auto& reference : type.references)
		{
			wr.WriteWord(2, reference.id);
			wr.WriteWord(2, reference.name_offset);
			wr.WriteWord(4, (reference.data_offset & 0x00FFFFFF) | (reference.attributes << 24));
			wr.Skip(4);
		}
	}
	/* name list */
	wr.Seek(write_offset + map_offset + name_list_offset);
	for(auto& name : resource_names)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name);
	}

	return ImageSize();
}

void MacintoshResourceFileFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_macroman);

	dump.SetTitle("Macintosh resource fork format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump, Dumper::Header);

	Dumper::Region data_region("Resource data", file_offset + data_offset, data_length, 8);
	data_region.Display(dump, Dumper::Header);

	Dumper::Region map_region("Resource map", file_offset + map_offset, map_length, 8);
	map_region.AddField("Attributes", Dumper::HexDisplay::Make(4), offset_t(attributes));
	map_region.Display(dump, Dumper::Header);

	offset_t resource_type_list_size = 2 + resource_types.size() * 8;
	for(auto& type : resource_types)
	{
		resource_type_list_size += type.references.size() * 12;
	}
	Dumper::Region resource_type_list_region("Resource type list", file_offset + map_offset + resource_type_list_offset, resource_type_list_size, 8);
	resource_type_list_region.Display(dump, Dumper::Header);

	unsigned i = 0;
	for(auto& type : resource_types)
	{
		Dumper::Entry resource_type_entry("Resource type", i + 1, file_offset + map_offset + resource_type_list_offset + i * 8, 8);
		resource_type_entry.AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type.type, 4));
		resource_type_entry.Display(dump, Dumper::Header | (memcmp(type.type, "CODE", 4) == 0 ? Dumper::Image : Dumper::Resource));

		unsigned j = 0;
		for(auto& reference : type.references)
		{
			Dumper::Entry resource_reference_entry("Resource reference", j + 1, file_offset + map_offset + resource_type_list_offset + type.offset + j * 12, 8);
			resource_reference_entry.AddField("OSType", Dumper::StringDisplay::Make(4, "'"), std::string(type.type, 4));
			resource_reference_entry.AddField("ID", Dumper::HexDisplay::Make(4), offset_t(reference.id));
			if(reference.name)
				resource_reference_entry.AddField("Name", Dumper::StringDisplay::Make("\""), *reference.name);
			resource_reference_entry.AddOptionalField("Name offset", Dumper::HexDisplay::Make(4), offset_t(reference.name_offset != 0xFFFF ? file_offset + map_offset + name_list_offset + reference.name_offset : 0));
			resource_reference_entry.AddField("Attributes", Dumper::HexDisplay::Make(2), offset_t(reference.attributes));
			resource_reference_entry.AddField("Data offset", Dumper::HexDisplay::Make(8), offset_t(reference.data_offset));
			resource_reference_entry.AddField("Data length", Dumper::HexDisplay::Make(8), offset_t(reference.data->ImageSize()));
			resource_reference_entry.Display(dump, Dumper::Header | (memcmp(type.type, "CODE", 4) == 0 ? Dumper::Image : Dumper::Resource));
			j++;
		}

		i++;
	}

	/*offset_t resource_name_list_size = 0;
	for(auto& name : resource_names)
	{
		resource_name_list_size += name.size() + 1;
	}*/
	Dumper::Region resource_name_list_region("Resource name list", file_offset + map_offset + name_list_offset, resource_type_list_size, 8);
	resource_name_list_region.Display(dump, Dumper::Header | Dumper::String);

	offset_t current_offset = file_offset + map_offset + name_list_offset;
	i = 0;
	for(auto& name : resource_names)
	{
		Dumper::Entry name_entry("Name", i + 1, current_offset, 8);
		name_entry.AddField("Value", Dumper::StringDisplay::Make("'"), name);
		name_entry.Display(dump, Dumper::String);
		current_offset += name.size() + 1;
		i++;
	}

	for(auto& type : resource_types)
	{
		for(auto& reference : type.references)
		{
			reference.data->Dump(dump, file_offset + data_offset + reference.data_offset);
		}
	}

	// TODO: display all resource data
}

void MacintoshResourceFileFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string MacintoshResourceFileFormat::GetDefaultExtension(Linker::Module& module) const
{
	return "a.out";
}

std::shared_ptr<MacintoshResourceFileFormat::Resource> MacintoshResourceFileFormat::ReadResource(Linker::Reader& rd, const ResourceType& type, const ResourceReference& reference)
{
	std::shared_ptr<Resource> resource = nullptr;
	uint32_t length = rd.ReadUnsigned(4);
	switch(OSTypeToUInt32(type.type))
	{
	case CodeResource::OSType:
		if(reference.id == 0)
			resource = std::make_shared<JumpTableCodeResource>();
		else
			resource = std::make_shared<CodeResource>(reference.id);
		break;
	default:
		resource = std::make_shared<GenericResource>(type.type, reference.id);
		break;
	}
	resource->name = reference.name;
	resource->ReadFile(rd, length);
	return resource;
}

// MacBinary

uint16_t MacBinary::crc_step[256];

void MacBinary::CRC_Initialize() const
{
	crc = 0; // 0x1021;
	for(int byte = 0; byte < 256; byte++)
	{
		uint16_t value = byte << 8;
		for(int shift = 0; shift < 8; shift++)
		{
			if((value & 0x8000))
			{
				value = (value << 1) ^ 0x1021;
			}
			else
			{
				value <<= 1;
			}
		}
		crc_step[byte] = value;
	}
}

void MacBinary::CRC_Step(uint8_t byte) const
{
	crc = (crc << 8) ^ crc_step[(crc >> 8) ^ byte];
}

void MacBinary::Skip(Linker::Writer& wr, size_t count) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(0);
	}
	wr.Skip(count);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, const void * data) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(static_cast<const char *>(data)[i]);
	}
	wr.WriteData(count, data);
}

void MacBinary::WriteData(Linker::Writer& wr, size_t count, std::string text) const
{
	for(size_t i = 0; i < count; i++)
	{
		CRC_Step(i < text.size() ? text[i] : 0);
	}
	wr.WriteData(count, text);
}

void MacBinary::WriteWord(Linker::Writer& wr, size_t bytes, uint64_t value) const
{
	std::vector<uint8_t> data(bytes);
	::WriteWord(bytes, bytes, data.data(), value, EndianType::BigEndian);
	WriteData(wr, bytes, data.data());
}

void MacBinary::ReadHeader(Linker::Reader& rd)
{
	rd.Skip(1);
	if(apple_single == nullptr)
	{
		apple_single = std::make_shared<AppleSingleDouble>();
	}
	uint8_t name_size = rd.ReadUnsigned(1);
	if(name_size > 63)
	{
		Linker::Warning << "Warning: Invalid name size found, truncating: " << name_size << std::endl;
		name_size = 63;
	}
	auto real_name = std::dynamic_pointer_cast<RealName>(apple_single->GetRealName());
	real_name->name = rd.ReadData(name_size);
	rd.Skip(63 - name_size);
	auto finder_info = std::dynamic_pointer_cast<FinderInfo>(apple_single->GetFinderInfo());
	rd.ReadData(4, finder_info->Type);
	rd.ReadData(4, finder_info->Creator);
	finder_info->Flags = rd.ReadUnsigned(1) << 8;
	rd.Skip(1);
	finder_info->Location.x = rd.ReadUnsigned(2);
	finder_info->Location.y = rd.ReadUnsigned(2);
	rd.Skip(2); // TODO: window/folder info?
	attributes = rd.ReadUnsigned(1);
	rd.Skip(1);
	data_fork_length = rd.ReadUnsigned(4);
	resource_fork_length = rd.ReadUnsigned(4);
	creation = rd.ReadUnsigned(4); // TODO: maybe these 2 could be stored in a file field?
	modification = rd.ReadUnsigned(4);
	// Get Info extension
	comment_length = rd.ReadUnsigned(2);
	if(comment_length != 0 && version < MACBIN1_GETINFO)
	{
		version = MACBIN1_GETINFO;
	}
	// MacBinary II
	uint8_t flags_low_byte = rd.ReadUnsigned(1);
	finder_info->Flags |= flags_low_byte;
	if(flags_low_byte != 0 && version < MACBIN2)
	{
		version = MACBIN2;
	}
	// MacBinary III
	auto signature = rd.ReadData(4);
	if(signature == "mBIN")
	{
		// TODO: script of file and extended Finder flags
	}
	rd.Skip(14); // TODO:
	secondary_header_size = rd.ReadUnsigned(2);
	uint8_t actual_version = rd.ReadUnsigned(1);
	if(actual_version != 0)
	{
		version = version_t(actual_version);
	}
	minimum_version = version_t(rd.ReadUnsigned(1));
	crc = rd.ReadUnsigned(2);
}

void MacBinary::WriteHeader(Linker::Writer& wr) const
{
	CRC_Initialize();
	WriteWord(wr, 1, 0);
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_RealName))
	{
		const std::string& name = std::dynamic_pointer_cast<const RealName>(entry)->name;
		WriteWord(wr, 1, name.size() > 63 ? 63 : name.size());
		WriteData(wr, 63, name);
	}
	else
	{
		WriteWord(wr, 1, generated_file_name.size() > 63 ? 63 : generated_file_name.size());
		WriteData(wr, 63, generated_file_name);
	}
	std::shared_ptr<const FinderInfo> finder_info = nullptr;
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_FinderInfo))
	{
		finder_info = std::dynamic_pointer_cast<const FinderInfo>(entry);
		WriteData(wr, 4, finder_info->Type);
		WriteData(wr, 4, finder_info->Creator);
		WriteWord(wr, 1, finder_info->Flags >> 8);
		WriteWord(wr, 1, 0);
		WriteWord(wr, 2, finder_info->Location.y);
		WriteWord(wr, 2, finder_info->Location.x);
		WriteWord(wr, 2, 0); /* window/folder info */
	}
	else
	{
		WriteData(wr, 16, "");
	}
	WriteWord(wr, 1, attributes);
	WriteWord(wr, 1, 0);
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		WriteWord(wr, 4, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 4, 0);
	}
	WriteWord(wr, 4, creation);
	WriteWord(wr, 4, modification);
	if(version < MACBIN1_GETINFO)
	{
		return;
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_Comment))
	{
		WriteWord(wr, 2, entry->ImageSize());
	}
	else
	{
		WriteWord(wr, 2, 0);
	}
	if(version < MACBIN2)
	{
		return;
	}
	if(finder_info != nullptr)
	{
		WriteWord(wr, 1, finder_info->Flags & 0xFF);
	}
	else
	{
		WriteWord(wr, 1, 0);
	}
	if(version >= MACBIN3)
	{
		WriteData(wr, 4, "mBIN");
		WriteWord(wr, 1, 0); /* script of file */
		WriteWord(wr, 1, 0); /* extended Finder flags */
		Skip(wr, 8);
	}
	else
	{
		Skip(wr, 14);
	}
	WriteWord(wr, 4, 0); /* unpacked file size */
	WriteWord(wr, 2, secondary_header_size);
	WriteWord(wr, 1, version);
	WriteWord(wr, 1, minimum_version);
	wr.WriteWord(2, crc);
}

void MacBinary::CalculateValues()
{
	attributes = apple_single->ReadMacintoshAttributes();
	creation = apple_single->ReadCreationDate();
	modification = apple_single->ReadModificationDate();
	apple_single->CalculateValues();
}

void MacBinary::ReadFile(Linker::Reader& rd)
{
	ReadHeader(rd);
	rd.Seek(::AlignTo(0x80 + secondary_header_size, 0x80));
	/* secondary header */
	if(data_fork_length != 0)
	{
		auto data_fork = dynamic_pointer_cast<DataFork>(apple_single->GetDataFork());
		// TODO: check format
		auto image = Linker::Buffer::ReadFromFile(rd, data_fork_length);
		data_fork->image = image;
		rd.Seek(::AlignTo(rd.Tell(), 0x80));
	}
	if(resource_fork_length != 0)
	{
		auto resource_fork = dynamic_pointer_cast<ResourceFork>(apple_single->GetResourceFork());
		// TODO: check format
		auto mac_rsrc = std::make_shared<MacintoshResourceFileFormat>();
		mac_rsrc->ReadFile(rd);
		resource_fork->image = mac_rsrc;
		rd.Seek(::AlignTo(rd.Tell(), 0x80));
	}
	if(comment_length != 0)
	{
		//auto comment = dynamic_pointer_cast<Comment>(apple_single->GetComment());
		// TODO
	}
}

offset_t MacBinary::WriteFile(Linker::Writer& wr) const
{
	WriteHeader(wr);
	wr.Seek(::AlignTo(0x80 + secondary_header_size, 0x80));
	/* secondary header */
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		entry->WriteFile(wr);
		wr.AlignTo(0x80);
	}
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		entry->WriteFile(wr);
		wr.AlignTo(0x80);
	}
	if(version >= MACBIN1_GETINFO)
	{
		if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_Comment))
		{
			entry->WriteFile(wr);
			wr.AlignTo(0x80);
		}
	}

	return offset_t(-1);
}

void MacBinary::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_macroman);

	dump.SetTitle("MacBinary format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump, Dumper::Header);

	Dumper::Region header_region("Header", file_offset, 0x80, 8);
	std::string real_name = "";
	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_RealName))
	{
		real_name = std::dynamic_pointer_cast<const RealName>(entry)->name;
	}
	header_region.AddField("Real name", Dumper::StringDisplay::Make("'"), real_name);
	auto finder_info = std::dynamic_pointer_cast<FinderInfo>(apple_single->GetFinderInfo());
	if(finder_info)
	{
		header_region.AddField("OS Type", Dumper::StringDisplay::Make(4, "'"), std::string(finder_info->Type));
		header_region.AddField("Creator", Dumper::StringDisplay::Make(4, "'"), std::string(finder_info->Creator));
		header_region.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(finder_info->Flags)); // TODO: should be a bit field
		header_region.AddField("Location.x", Dumper::DecDisplay::Make(), offset_t(finder_info->Location.x));
		header_region.AddField("Location.x", Dumper::DecDisplay::Make(), offset_t(finder_info->Location.y));
	}
	header_region.AddField("Attributes", Dumper::HexDisplay::Make(4), offset_t(attributes)); // TODO: should be a bit field
	header_region.AddField("Creation", Dumper::DecDisplay::Make(), offset_t(creation)); // TODO: format
	header_region.AddField("Modification", Dumper::DecDisplay::Make(), offset_t(modification)); // TODO: format
	// TODO: MacBinary III "mBIN" field present, script of file and extended Finder flags
	header_region.AddOptionalField("Modification", Dumper::DecDisplay::Make(), offset_t(modification)); // TODO: format
	header_region.AddField("Version", Dumper::DecDisplay::Make(), offset_t(version < MACBIN2 ? 0 : version));
	header_region.AddField("Minimum version", Dumper::DecDisplay::Make(), offset_t(minimum_version));
	header_region.AddField("CRC", Dumper::HexDisplay::Make(4), offset_t(crc));
	header_region.Display(dump, Dumper::Header);

	if(secondary_header_size != 0)
	{
		Dumper::Region secondary_header_region("Secondary header", file_offset + 0x80, secondary_header_size, 8);
		secondary_header_region.Display(dump, Dumper::Header);
	}

	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_DataFork))
	{
		entry->Dump(dump);
	}

	if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_ResourceFork))
	{
		entry->Dump(dump);
	}
}

// MacintoshOutput

bool MacintoshOutput::AddSupplementaryOutputFormat(std::string subformat)
{
	if(subformat == "rsrc")
	{
		Linker::Debug << "Debug: Requested to generate resource fork under .rsrc" << std::endl;
		produce = produce_format_t(produce | PRODUCE_RESOURCE_FORK);
	}
	else if(subformat == "finf")
	{
		Linker::Debug << "Debug: Requested to generate Finder Info file under .finf" << std::endl;
		produce = produce_format_t(produce | PRODUCE_FINDER_INFO);
	}
	else if(subformat == "double" || subformat == "appledouble")
	{
		Linker::Debug << "Debug: Requested to generate AppleDouble" << std::endl;
		produce = produce_format_t(produce | PRODUCE_APPLE_DOUBLE);
		/* TODO: versions */
	}
	else if(subformat == "mbin" || subformat == "macbin" || subformat == "macbinary")
	{
		Linker::Debug << "Debug: Requested to generate MacBinary" << std::endl;
		produce = produce_format_t(produce | PRODUCE_MAC_BINARY);
		/* TODO: versions */
	}
	else
	{
		return false;
	}
	return true;
}

void MacintoshOutput::OnContainerCreated() { }

void MacintoshOutput::OnCalculateValues()
{
	// TODO: error
}

void MacintoshOutput::OnReadFile(Linker::Reader& rd)
{
	// TODO: error
}

offset_t MacintoshOutput::OnWriteFile(Linker::Writer& wr) const
{
	// TODO: error
	return offset_t(-1);
}

void MacintoshOutput::OnDump(Dumper::Dumper& dump) const
{
	// TODO: error
}

void MacintoshOutput::GenerateFiles(std::string filename, std::shared_ptr<Contents> data_fork, std::shared_ptr<Contents> resource_fork)
{
	container = CONTAINER_NONE; // initial setting

	if((data_fork != nullptr && (target != TARGET_DATA_FORK || produce != 0))
	|| (resource_fork != nullptr && (target != TARGET_RESOURCE_FORK || (produce & ~PRODUCE_RESOURCE_FORK) != 0)))
	{
		// if anything other than a single data fork or single resource fork is required, create an AppleSingleDouble container
		container = CONTAINER_APPLE_SINGLE;
		apple_single = std::make_shared<AppleSingleDouble>(target == TARGET_APPLE_SINGLE ? AppleSingleDouble::SINGLE : AppleSingleDouble::DOUBLE,
			apple_single_double_version, home_file_system);
		if(data_fork != nullptr)
		{
			apple_single->AppendEntry(std::make_shared<DataFork>(data_fork));
		}
		if(resource_fork != nullptr)
		{
			apple_single->AppendEntry(std::make_shared<ResourceFork>(resource_fork));
		}
		OnContainerCreated();
	}

	if(target == TARGET_MAC_BINARY || (produce & PRODUCE_MAC_BINARY) != 0)
	{
		// the presence of a MacBinary container implies presence of an AppleSingleDouble container
		container = CONTAINER_MAC_BINARY;
		mac_binary = std::make_shared<MacBinary>(apple_single, macbinary_version, macbinary_minimum_version);
		mac_binary->generated_file_name = filename;
	}

	if(target == TARGET_APPLE_SINGLE)
	{
		apple_single->GetDataFork();
	}

	switch(container)
	{
	case CONTAINER_NONE:
		OnCalculateValues();
		break;
	case CONTAINER_APPLE_SINGLE:
		apple_single->CalculateValues();
		break;
	case CONTAINER_MAC_BINARY:
		mac_binary->CalculateValues();
		break;
	}

	std::ofstream out;
	Linker::Writer wr(::BigEndian);
	switch(target)
	{
	case TARGET_NONE:
		break;
	case TARGET_DATA_FORK:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		if(data_fork != nullptr)
		{
			wr.out = &out;
			data_fork->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_RESOURCE_FORK:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		if(resource_fork != nullptr)
		{
			wr.out = &out;
			resource_fork->WriteFile(wr);
		}
		out.close();
		break;
	case TARGET_APPLE_SINGLE:
	case TARGET_APPLE_DOUBLE:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		apple_single->WriteFile(wr);
		out.close();
		break;
	case TARGET_MAC_BINARY:
		out.open(filename, std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		mac_binary->WriteFile(wr);
		out.close();
		break;
	}

	if((produce & PRODUCE_RESOURCE_FORK))
	{
		Linker::Debug << "Debug: Generating resource fork under .rsrc" << std::endl;
		std::error_code err;
		std::filesystem::path path = std::filesystem::path(filename);
		path = path.parent_path() / ".rsrc" / path.filename();
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no resource fork file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(resource_fork != nullptr)
			{
				wr.out = &out;
				resource_fork->WriteFile(wr);
			}
			out.close();
		}
	}

	if((produce & PRODUCE_FINDER_INFO))
	{
		Linker::Debug << "Debug: Generating Finder Info file under .finf" << std::endl;
		std::error_code err;
		std::filesystem::path path = std::filesystem::path(filename);
		path = path.parent_path() / ".finf" / path.filename();
		if(!std::filesystem::create_directory(path.parent_path(), err) && err != std::errc(0))
		{
			Linker::Error << "Error: Unable to create folder " << path.parent_path() << ", no Finder Info file will be generated" << std::endl;
		}
		else
		{
			out.open(path.string(), std::ios_base::out | std::ios_base::binary);
			if(auto entry = apple_single->FindEntry(AppleSingleDouble::ID_FinderInfo))
			{
				wr.out = &out;
				entry->WriteFile(wr);
			}
			out.close();
		}
	}

	if((produce & PRODUCE_APPLE_DOUBLE))
	{
		Linker::Debug << "Debug: Generating AppleDouble" << std::endl;
		std::ofstream out;
		out.open(apple_single->GetUNIXDoubleFilename(filename), std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		if(target != TARGET_APPLE_SINGLE)
		{
			apple_single->WriteFile(wr);
		}
		else
		{
			AppleSingleDouble apple_double(*apple_single, AppleSingleDouble::DOUBLE);
			apple_double.WriteFile(wr);
		}
		out.close();
	}

	if((produce & PRODUCE_MAC_BINARY))
	{
		Linker::Debug << "Debug: Generating MacBinary" << std::endl;
		std::ofstream out;
		out.open(
			target == TARGET_NONE ? filename : filename + ".mbin",
			std::ios_base::out | std::ios_base::binary);
		wr.out = &out;
		mac_binary->WriteFile(wr);
		out.close();
	}
}

void MacintoshOutput::ReadFile(Linker::Reader& rd)
{
	apple_single = nullptr;
	mac_binary = nullptr;

	switch(target)
	{
	case TARGET_DATA_FORK:
	case TARGET_RESOURCE_FORK:
		container = CONTAINER_NONE;
		OnReadFile(rd);
		break;
	case TARGET_APPLE_SINGLE:
	case TARGET_APPLE_DOUBLE:
		{
			container = CONTAINER_APPLE_SINGLE;
			apple_single = std::make_shared<AppleSingleDouble>();
			apple_single->ReadFile(rd);
		}
		break;
	case TARGET_MAC_BINARY:
		{
			container = CONTAINER_MAC_BINARY;
			mac_binary = std::make_shared<MacBinary>();
			mac_binary->ReadFile(rd);
		}
		break;
	case TARGET_NONE:
		if((produce & PRODUCE_APPLE_DOUBLE))
		{
			container = CONTAINER_APPLE_SINGLE;
			apple_single = std::make_shared<AppleSingleDouble>();
			apple_single->ReadFile(rd);
		}
		else if((produce & PRODUCE_MAC_BINARY))
		{
			container = CONTAINER_MAC_BINARY;
			mac_binary = std::make_shared<MacBinary>();
			mac_binary->ReadFile(rd);
		}
		else
		{
			container = CONTAINER_NONE;
			OnReadFile(rd);
		}
		break;
	}
}

offset_t MacintoshOutput::WriteFile(Linker::Writer& wr) const
{
	switch(container)
	{
	case CONTAINER_NONE:
		return OnWriteFile(wr);
	case CONTAINER_APPLE_SINGLE:
		return apple_single->WriteFile(wr);
	case CONTAINER_MAC_BINARY:
		return mac_binary->WriteFile(wr);
	default:
		Linker::FatalError("Internal error: file not loaded");
	}
}

void MacintoshOutput::Dump(Dumper::Dumper& dump) const
{
	switch(container)
	{
	case CONTAINER_NONE:
		OnDump(dump);
		break;
	case CONTAINER_APPLE_SINGLE:
		apple_single->Dump(dump);
		break;
	case CONTAINER_MAC_BINARY:
		mac_binary->Dump(dump);
		break;
	default:
		Linker::FatalError("Internal error: file not loaded");
	}
}

// Classic68KDriver

bool Classic68KDriver::FormatSupportsResources() const
{
	return true;
}

void Classic68KDriver::SetOptions(std::map<std::string, std::string>& options)
{
	this->options = options;
}

std::vector<Linker::OptionDescription<void>> Classic68KDriver::GetMemoryModelNames()
{
	MacintoshResourceFileFormat tmp;
	return tmp.GetMemoryModelNames();
}

void Classic68KDriver::SetModel(std::string model)
{
	this->model = model;
}

void Classic68KDriver::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	this->script_file = script_file;
	this->script_options = options;
}

void Classic68KDriver::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu != Linker::Module::M68K)
	{
		Linker::Error << "Error: Format only supports Motorola 68000 binaries" << std::endl;
	}

	resource_fork = std::make_shared<MacintoshResourceFileFormat>();
	resource_fork->SetOptions(options);
	resource_fork->SetModel(model);
	resource_fork->SetLinkScript(script_file, script_options);

	resource_fork->ProcessModule(module);

	GenerateFiles(filename, nullptr, resource_fork);
}

void Classic68KDriver::OnContainerCreated()
{
	// if an AppleSingleDouble container is created, we need to allocate the FinderInfo entry
	// this is also needed if no actual AppleSingle/AppleDouble file is created, as this might be placed under the .finf directory

	finder_info = std::dynamic_pointer_cast<FinderInfo>(apple_single->GetFinderInfo());
	if(finder_info != nullptr)
	{
		finder_info->SetTypeAndCreator("APPL", "????");
	}
}

void Classic68KDriver::OnCalculateValues()
{
	resource_fork->CalculateValues(); // TODO: untested
}

void Classic68KDriver::OnReadFile(Linker::Reader& rd)
{
	if(target == TARGET_RESOURCE_FORK || (produce & PRODUCE_RESOURCE_FORK) != 0)
	{
		// parse as resource fork, but only if this option is allowed
		resource_fork = std::make_shared<MacintoshResourceFileFormat>();
		resource_fork->ReadFile(rd);
	}
	else
	{
		Linker::FatalError("Fatal error: Reading the specified format is not supported");
	}
}

offset_t Classic68KDriver::OnWriteFile(Linker::Writer& wr) const
{
	return resource_fork->WriteFile(wr);
}

void Classic68KDriver::OnDump(Dumper::Dumper& dump) const
{
	resource_fork->Dump(dump);
}

void Classic68KDriver::ReadFile(Linker::Reader& rd)
{
	// reading a Classic 68K Mac OS executable cannot be done via its data fork
	if(target == TARGET_DATA_FORK)
	{
		target = TARGET_NONE;
	}
	MacintoshOutput::ReadFile(rd);
}

std::string Classic68KDriver::GetDefaultExtension(Linker::Module& module) const
{
	switch(target)
	{
	case TARGET_NONE:
	case TARGET_DATA_FORK:
		return "a.out";
	default:
		return Linker::OutputFormat::GetDefaultExtension(module);
	}
}

std::string Classic68KDriver::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	switch(target)
	{
	case TARGET_NONE:
	case TARGET_DATA_FORK:
		return filename;
	case TARGET_RESOURCE_FORK:
		return filename + ".res"; // A/UX convention (see A/UX Toolbox: Macintosh ROM Interface)
	case TARGET_APPLE_SINGLE:
		return filename + ".as"; // used by CiderPress
	case TARGET_APPLE_DOUBLE:
		return filename + ".ad"; // understood by Retro68
	case TARGET_MAC_BINARY:
		return filename + ".bin"; // understood by Retro68
	default:
		return filename; // should not happen
	}
}

