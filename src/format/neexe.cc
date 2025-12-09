
#include "neexe.h"
#include "mzexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace Microsoft;

const std::map<offset_t, std::string> OS2::resource_type_id_descriptions =
{
	{ OS2::RT_POINTER, "RT_POINTER" },
	{ OS2::RT_BITMAP, "RT_BITMAP" },
	{ OS2::RT_MENU, "RT_MENU" },
	{ OS2::RT_DIALOG, "RT_DIALOG" },
	{ OS2::RT_STRING, "RT_STRING" },
	{ OS2::RT_FONTDIR, "RT_FONTDIR" },
	{ OS2::RT_FONT, "RT_FONT" },
	{ OS2::RT_ACCELERATOR, "RT_ACCELERATOR" },
	{ OS2::RT_RCDATA, "RT_RCDATA" },
	{ OS2::RT_MESSAGE, "RT_MESSAGE" },
	{ OS2::RT_DLGINCLUDE, "RT_DLGINCLUDE" },
	{ OS2::RT_VKEYTBL, "RT_VKEYTBL" },
	{ OS2::RT_KEYTBL, "RT_KEYTBL" },
	{ OS2::RT_CHARTBL, "RT_CHARTBL" },
	{ OS2::RT_DISPLAYINFO, "RT_DISPLAYINFO" },
	{ OS2::RT_FKASHORT, "RT_FKASHORT" },
	{ OS2::RT_FKALONG, "RT_FKALONG" },
	{ OS2::RT_HELPTABLE, "RT_HELPTABLE" },
	{ OS2::RT_HELPSUBTABLE, "RT_HELPSUBTABLE" },
	{ OS2::RT_FDDIR, "RT_FDDIR" },
	{ OS2::RT_FD, "RT_FD" },
};

const std::map<offset_t, std::string> Windows::resource_type_id_descriptions =
{
	{ Windows::RT_CURSOR, "RT_CURSOR" },
	{ Windows::RT_BITMAP, "RT_BITMAP" },
	{ Windows::RT_ICON, "RT_ICON" },
	{ Windows::RT_MENU, "RT_MENU" },
	{ Windows::RT_DIALOG, "RT_DIALOG" },
	{ Windows::RT_STRING, "RT_STRING" },
	{ Windows::RT_FONTDIR, "RT_FONTDIR" },
	{ Windows::RT_FONT, "RT_FONT" },
	{ Windows::RT_ACCELERATOR, "RT_ACCELERATOR" },
	{ Windows::RT_RCDATA, "RT_RCDATA" },
	{ Windows::RT_MESSAGETABLE, "RT_MESSAGETABLE" },
	{ Windows::RT_GROUP_CURSOR, "RT_GROUP_CURSOR" },
	{ Windows::RT_GROUP_ICON, "RT_GROUP_ICON" },
	{ Windows::RT_VERSION, "RT_VERSION" },
	{ Windows::RT_DLGINCLUDE, "RT_DLGINCLUDE" },
	{ Windows::RT_PLUGPLAY, "RT_PLUGPLAY" },
	{ Windows::RT_VXD, "RT_VXD" },
	{ Windows::RT_ANICURSOR, "RT_ANICURSOR" },
	{ Windows::RT_ANIICON, "RT_ANIICON" },
	{ Windows::RT_HTML, "RT_HTML" },
	{ Windows::RT_MANIFEST, "RT_MANIFEST" },
};

const std::map<offset_t, std::string> Windows::flagged_resource_type_id_descriptions =
{
	{ 0x8000 | Windows::RT_CURSOR, "RT_CURSOR" },
	{ 0x8000 | Windows::RT_BITMAP, "RT_BITMAP" },
	{ 0x8000 | Windows::RT_ICON, "RT_ICON" },
	{ 0x8000 | Windows::RT_MENU, "RT_MENU" },
	{ 0x8000 | Windows::RT_DIALOG, "RT_DIALOG" },
	{ 0x8000 | Windows::RT_STRING, "RT_STRING" },
	{ 0x8000 | Windows::RT_FONTDIR, "RT_FONTDIR" },
	{ 0x8000 | Windows::RT_FONT, "RT_FONT" },
	{ 0x8000 | Windows::RT_ACCELERATOR, "RT_ACCELERATOR" },
	{ 0x8000 | Windows::RT_RCDATA, "RT_RCDATA" },
	{ 0x8000 | Windows::RT_MESSAGETABLE, "RT_MESSAGETABLE" },
	{ 0x8000 | Windows::RT_GROUP_CURSOR, "RT_GROUP_CURSOR" },
	{ 0x8000 | Windows::RT_GROUP_ICON, "RT_GROUP_ICON" },
	{ 0x8000 | Windows::RT_VERSION, "RT_VERSION" },
	{ 0x8000 | Windows::RT_DLGINCLUDE, "RT_DLGINCLUDE" },
	{ 0x8000 | Windows::RT_PLUGPLAY, "RT_PLUGPLAY" },
	{ 0x8000 | Windows::RT_VXD, "RT_VXD" },
	{ 0x8000 | Windows::RT_ANICURSOR, "RT_ANICURSOR" },
	{ 0x8000 | Windows::RT_ANIICON, "RT_ANIICON" },
	{ 0x8000 | Windows::RT_HTML, "RT_HTML" },
	{ 0x8000 | Windows::RT_MANIFEST, "RT_MANIFEST" },
};

NEFormat::Segment::Relocation::source_type NEFormat::Segment::Relocation::GetType(Linker::Relocation& rel)
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
			return Offset32;
		default:
			return source_type(-1);
		}
	}
}

size_t NEFormat::Segment::Relocation::GetSize() const
{
	switch(type)
	{
	case Offset8:
		return 1;
	case Selector16:
	case Offset16:
		return 2;
	case Pointer32:
	case Offset32:
		return 4;
	case Pointer48:
		return 6;
	default:
		return 0;
	}
}

void NEFormat::Segment::AddRelocation(const Relocation& rel)
{
	if(rel.offsets.size() == 0)
		return;
	relocations_map[rel.offsets[0]] = rel;
}

void NEFormat::Segment::Dump(Dumper::Dumper& dump, unsigned index, bool isos2) const
{
	Dumper::Block segment_block("Segment", data_offset, image->AsImage(), 0, 8);
	segment_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
	segment_block.AddField("Memory size", Dumper::HexDisplay::Make(4), offset_t(total_size));
	segment_block.AddField("Flags",
		Dumper::BitFieldDisplay::Make(4)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data", "code"), false)
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("allocated"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("loaded/real mode"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("iterated"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("movable", "fixed"), false)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("pure", "impure"), false)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("load on call", "preload"), false)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make((flags & Segment::Data) != 0 ? "read only" : "execute only"), true)
			->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("relocations present"), true)
			->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("debugging information present/conforming code segment"), true)
			->AddBitField(10, 2, "descriptor privilege level", Dumper::HexDisplay::Make(1))
			->AddBitField(12, 4, "discard priority", Dumper::HexDisplay::Make(1))
			->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("discardable"), true)
			->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("32-bit"), true)
			->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("huge segment"), true)
			->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("RESRC_HIGH"), true),
		offset_t(flags));
	for(auto& relocation : relocations)
	{
		for(uint16_t offset : relocation.offsets)
		{
			segment_block.AddSignal(offset, relocation.GetSize());
		}
	}
	segment_block.Display(dump);
	unsigned i = 0;
	static const std::map<offset_t, std::string> type_descriptions =
	{
		{ Relocation::Offset8, "8-bit offset" },
		{ Relocation::Selector16, "16-bit selector" },
		{ Relocation::Pointer32, "16:16-bit pointer" },
		{ Relocation::Offset16, "16-bit offset" },
		{ Relocation::Pointer48, "16:32-bit pointer" },
		{ Relocation::Offset32, "32-bit offset" },
	};
	static const std::map<offset_t, std::string> target_descriptions =
	{
		{ Relocation::Internal, "internal" },
		{ Relocation::ImportOrdinal, "import by ordinal" },
		{ Relocation::ImportName, "import by name" },
		{ Relocation::OSFixup, "operating system" },
	};
	for(auto& relocation : relocations)
	{
		unsigned j = 0;
		for(uint16_t offset : relocation.offsets)
		{
			if(j == 0)
			{
				Dumper::Entry rel_entry("Relocation", i + 1, data_offset + image->ImageSize() + i * 8, 8);
				rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(relocation.type));
				rel_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation.GetSize()));
				rel_entry.AddField("Offset", Dumper::HexDisplay::Make(4), offset_t(offset));
				rel_entry.AddField("Flags",
					Dumper::BitFieldDisplay::Make(2)
						->AddBitField(0, 2, Dumper::ChoiceDisplay::Make(target_descriptions), false)
						->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("additive"), true),
					offset_t(relocation.flags));
				switch(relocation.flags & Relocation::TargetTypeMask)
				{
				case Relocation::Internal:
					if((relocation.module & 0xFF) == 0xFF)
					{
						rel_entry.AddField("Entry", Dumper::HexDisplay::Make(4), offset_t(relocation.target));
						rel_entry.AddField("Location", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(4)), offset_t(relocation.actual_segment), offset_t(relocation.actual_offset));
						rel_entry.AddOptionalField("Exported name", Dumper::StringDisplay::Make(), relocation.import_name);
					}
					else
					{
						rel_entry.AddField("Location", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(4)), offset_t(relocation.module & 0xFF), offset_t(relocation.target));
					}
					break;
				case Relocation::ImportOrdinal:
					rel_entry.AddField("Module number", Dumper::DecDisplay::Make(), offset_t(relocation.module));
					rel_entry.AddOptionalField("Module", Dumper::StringDisplay::Make(), relocation.module_name);
					rel_entry.AddField("Ordinal", Dumper::HexDisplay::Make(4), offset_t(relocation.target));
					break;
				case Relocation::ImportName:
					rel_entry.AddField("Module number", Dumper::DecDisplay::Make(), offset_t(relocation.module));
					rel_entry.AddOptionalField("Module", Dumper::StringDisplay::Make(), relocation.module_name);
					rel_entry.AddField("Name offset", Dumper::HexDisplay::Make(4), offset_t(relocation.target));
					rel_entry.AddOptionalField("Name", Dumper::StringDisplay::Make(), relocation.import_name);
					break;
				case Relocation::OSFixup:
					{
						static const std::map<offset_t, std::string> fixup_descriptions =
						{
							{ Relocation::FIARQQ, "FIARQQ/FJARQQ (DS prefixed fp)" },
							{ Relocation::FISRQQ, "FISRQQ/FJSRQQ (SS prefixed fp)" },
							{ Relocation::FICRQQ, "FICRQQ/FJCRQQ (CS prefixed fp)" },
							{ Relocation::FIERQQ, "FIERQQ (ES prefixed fp)" },
							{ Relocation::FIDRQQ, "FIDRQQ (unprefixed fp)" },
							{ Relocation::FIWRQQ, "FIWRQQ (fp wait)" },
						};
						rel_entry.AddField("Fixup type", Dumper::ChoiceDisplay::Make(fixup_descriptions, Dumper::HexDisplay::Make(4)), offset_t(relocation.module));
					}
					break;
				}
				if((relocation.flags & Relocation::Additive) != 0)
				{
					rel_entry.AddOptionalField("Addend", Dumper::HexDisplay::Make(4), offset_t(image->AsImage()->ReadUnsigned(2, offset, ::LittleEndian)));
				}
				rel_entry.Display(dump);
			}
			else
			{
				Dumper::Entry rel_entry("Chained", j, data_offset + relocation.offsets[j - 1], 8);
				rel_entry.AddField("Offset", Dumper::HexDisplay::Make(4), offset_t(offset));
				rel_entry.Display(dump);
			}
			j++;
		}
		i++;
	}
}

void NEFormat::Resource::Dump(Dumper::Dumper& dump, unsigned index, bool isos2) const
{
	if(isos2)
	{
		Dumper::Block resource_block("Resource", data_offset, image->AsImage(), 0, 8);
		resource_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
		resource_block.AddField("Type ID", Dumper::ChoiceDisplay::Make(OS2::resource_type_id_descriptions, Dumper::HexDisplay::Make(4)), offset_t(type_id));
		resource_block.AddField("Resource ID", Dumper::HexDisplay::Make(4), offset_t(id));
		resource_block.AddField("Memory size", Dumper::HexDisplay::Make(4), offset_t(total_size));
		resource_block.AddField("Flags",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("allocated"), true)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("loaded/real mode"), true)
				->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("iterated"), true)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("movable", "fixed"), false)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("pure", "impure"), false)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("load on call", "preload"), false)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make((flags & Segment::Data) != 0 ? "read only" : "execute only"), true)
				->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("relocations present"), true)
				->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("debugging information present/conforming code segment"), true)
				->AddBitField(10, 2, "descriptor privilege level", Dumper::HexDisplay::Make(1), true)
				->AddBitField(12, 4, "discard priority", Dumper::HexDisplay::Make(1), true)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("discardable"), true)
				->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("32-bit"), true)
				->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("huge segment"), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("RESRC_HIGH"), true),
			offset_t(flags));
		for(auto& relocation : relocations)
		{
			for(uint16_t offset : relocation.offsets)
			{
				resource_block.AddSignal(offset, relocation.GetSize());
			}
		}
		resource_block.Display(dump);
		// TODO: print out relocations
	}
	else
	{
		Dumper::Block resource_block("Resource", data_offset, image->AsImage(), 0, 8);
		resource_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(index + 1));
		resource_block.AddField("Type ID", Dumper::ChoiceDisplay::Make(Windows::flagged_resource_type_id_descriptions, Dumper::HexDisplay::Make(4)), offset_t(type_id));
		resource_block.AddOptionalField("Type ID name", Dumper::StringDisplay::Make(), type_id_name);
		resource_block.AddField("Resource ID", Dumper::HexDisplay::Make(4), offset_t(id));
		resource_block.AddOptionalField("Resource ID name", Dumper::StringDisplay::Make(), id_name);
		resource_block.AddField("Flags",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("data"), true)
				->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("allocated"), true)
				->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("loaded/real mode"), true)
				->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("iterated"), true)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("movable", "fixed"), false)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("pure", "impure"), false)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("load on call", "preload"), false)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make((flags & Segment::Data) != 0 ? "read only" : "execute only"), true)
				->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("relocations present"), true)
				->AddBitField(9, 1, Dumper::ChoiceDisplay::Make("debugging information present/conforming code segment"), true)
				->AddBitField(10, 2, "descriptor privilege level", Dumper::HexDisplay::Make(1), true)
				->AddBitField(12, 4, "discard priority", Dumper::HexDisplay::Make(1), true)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("discardable"), true)
				->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("32-bit"), true)
				->AddBitField(14, 1, Dumper::ChoiceDisplay::Make("huge segment"), true)
				->AddBitField(15, 1, Dumper::ChoiceDisplay::Make("RESRC_HIGH"), true),
			offset_t(flags));
		resource_block.Display(dump);
	}
}

offset_t NEFormat::Entry::GetEntrySize() const
{
	/* Note: a bundle has at least 2 more bytes */
	switch(type)
	{
	case Unused:
		return 0;
	case Fixed:
		return 3;
	case Movable:
		return 6;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

uint8_t NEFormat::Entry::GetIndicatorByte() const
{
	switch(type)
	{
	case Unused:
		return 0x00;
	case Fixed:
		return segment;
	case Movable:
		return 0xFF;
	default:
		Linker::FatalError("Internal error: invalid entry type");
	}
}

NEFormat::Entry NEFormat::Entry::ReadEntry(Linker::Reader& rd, uint8_t indicator_byte)
{
	Entry entry;
	switch(indicator_byte)
	{
	case 0x00:
		entry.type = Unused;
		break;
	case 0xFF:
		entry.type = Movable;
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		rd.Skip(2);
		entry.segment = rd.ReadUnsigned(1);
		entry.offset = rd.ReadUnsigned(2);
		break;
	default:
		entry.type = Fixed;
		entry.segment = indicator_byte;
		entry.flags = Entry::flag_type(rd.ReadUnsigned(1));
		entry.offset = rd.ReadUnsigned(2);
		break;
	}
	return entry;
}

void NEFormat::Entry::WriteEntry(Linker::Writer& wr) const
{
	switch(type)
	{
	case Unused:
		break;
	case Fixed:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, offset);
		break;
	case Movable:
		wr.WriteWord(1, flags);
		wr.WriteWord(2, INT_3Fh);
		wr.WriteWord(1, segment);
		wr.WriteWord(2, offset);
		break;
	}
}

bool NEFormat::IsLibrary() const
{
	return application_flags & LIBRARY;
}

bool NEFormat::IsOS2() const
{
	return (system & ~PharLap) == OS2;
}

void NEFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();

	/* New header */

	rd.ReadData(signature);
	linker_version.major = rd.ReadUnsigned(1);
	linker_version.minor = rd.ReadUnsigned(1);
	entry_table_offset = rd.ReadUnsigned(2);
	if(entry_table_offset != 0)
		entry_table_offset += file_offset;
	entry_table_length = rd.ReadUnsigned(2);
	crc32 = rd.ReadUnsigned(4);
	program_flags = program_flag_type(rd.ReadUnsigned(1));
	application_flags = application_flag_type(rd.ReadUnsigned(1));
	automatic_data = rd.ReadUnsigned(2);
	heap_size = rd.ReadUnsigned(2);
	stack_size = rd.ReadUnsigned(2);
	ip = rd.ReadUnsigned(2);
	cs = rd.ReadUnsigned(2);
	sp = rd.ReadUnsigned(2);
	ss = rd.ReadUnsigned(2);
	uint16_t segment_count = rd.ReadUnsigned(2);
	uint16_t module_reference_count = rd.ReadUnsigned(2);
	nonresident_name_table_length = rd.ReadUnsigned(2);
	segment_table_offset = rd.ReadUnsigned(2);
	if(segment_table_offset != 0)
	{
		segment_table_offset += file_offset;
	}
	resource_table_offset = rd.ReadUnsigned(2);
	if(resource_table_offset != 0)
	{
		resource_table_offset += file_offset;
	}
	resident_name_table_offset = rd.ReadUnsigned(2);
	if(resident_name_table_offset != 0)
	{
		resident_name_table_offset += file_offset;
	}
	module_reference_table_offset = rd.ReadUnsigned(2);
	if(module_reference_table_offset != 0)
	{
		module_reference_table_offset += file_offset;
	}
	imported_names_table_offset = rd.ReadUnsigned(2);
	if(imported_names_table_offset != 0)
	{
		imported_names_table_offset += file_offset;
	}
	nonresident_name_table_offset = rd.ReadUnsigned(4);
	movable_entry_count = rd.ReadUnsigned(2);
	sector_shift = rd.ReadUnsigned(2);
	resource_count = rd.ReadUnsigned(2);
	system = system_type(rd.ReadUnsigned(1));
	additional_flags = additional_flag_type(rd.ReadUnsigned(1));
	fast_load_area_offset = rd.ReadUnsigned(2);
	fast_load_area_length = rd.ReadUnsigned(2);
	code_swap_area_length = rd.ReadUnsigned(2);
	windows_version.minor = rd.ReadUnsigned(1);
	windows_version.major = rd.ReadUnsigned(1);

	file_size = rd.Tell();

	uint32_t i;

	/* Segment table */
	rd.Seek(segment_table_offset);
	segments.clear();
	// under OS/2, we read the resource segments separately
	uint16_t actual_segment_count = IsOS2() ? segment_count - resource_count : segment_count;
	for(i = 0; i < actual_segment_count; i++)
	{
		Segment segment;
		segment.data_offset = offset_t(rd.ReadUnsigned(2)) << sector_shift;
		segment.image_size = rd.ReadUnsigned(2);
		if(segment.data_offset != 0 && segment.image_size == 0)
		{
			segment.image_size = 0x10000;
		}
		segment.flags = Segment::flag_type(rd.ReadUnsigned(2));
		segment.total_size = rd.ReadUnsigned(2);
		if(segment.total_size == 0)
		{
			segment.total_size = 0x10000;
		}
		segments.emplace_back(segment);
	}

	if(IsOS2())
	{
		// resource information is also stored in the segment table
		for(i = 0; i < resource_count; i++)
		{
			Resource resource;
			resource.data_offset = offset_t(rd.ReadUnsigned(2)) << sector_shift;
			resource.image_size = rd.ReadUnsigned(2);
			if(resource.data_offset != 0 && resource.image_size == 0)
			{
				resource.image_size = 0x10000;
			}
			resource.flags = Segment::flag_type(rd.ReadUnsigned(2));
			resource.total_size = rd.ReadUnsigned(2);
			if(resource.total_size == 0)
			{
				resource.total_size = 0x10000;
			}
			resources.emplace_back(resource);
		}
	}

	file_size = std::max(file_size, rd.Tell());

	/* Resource table */
	resource_types.clear();
	if(IsOS2())
	{
		if(resource_count != 0)
		{
			rd.Seek(resource_table_offset);
			for(i = 0; i < resource_count; i++)
			{
				resources[i].type_id = rd.ReadUnsigned(2);
				resources[i].id = rd.ReadUnsigned(2);
			}
		}
	}
	else
	{
		if(resource_table_offset != 0 && resource_table_offset != resident_name_table_offset)
		{
			rd.Seek(resource_table_offset);
			resource_shift = rd.ReadUnsigned(2);
			while(true)
			{
				uint16_t type_id = rd.ReadUnsigned(2);
				if(type_id == 0)
					break;
				ResourceType rtype;
				rtype.type_id = type_id;
				uint16_t count = rd.ReadUnsigned(2);
				rd.Skip(4);
				for(i = 0; i < count; i++)
				{
					Resource resource;
					resource.type_id = type_id;
					resource.data_offset = offset_t(rd.ReadUnsigned(2)) << resource_shift;
					// the official Microsoft documentation mistakenly claims that this length is in bytes
					resource.image_size = offset_t(rd.ReadUnsigned(2)) << resource_shift;
					resource.flags = Resource::flag_type(rd.ReadUnsigned(2));
					resource.id = rd.ReadUnsigned(2);
					resource.handle = rd.ReadUnsigned(2);
					resource.usage = rd.ReadUnsigned(2);
					rtype.resources.emplace_back(resource);
				}
				resource_types.emplace_back(rtype);
			}
		}

		resource_strings.clear();
		while(true)
		{
			uint8_t length = rd.ReadUnsigned(1);
			if(length == 0)
				break;
			resource_strings.emplace_back(rd.ReadData(length));
		}

		for(auto& rtype : resource_types)
		{
			if((rtype.type_id & 0x8000) == 0)
			{
				rd.Seek(resource_table_offset + rtype.type_id);
				uint8_t length = rd.ReadUnsigned(1);
				rtype.type_id_name = rd.ReadData(length);
			}
			for(auto& resource : rtype.resources)
			{
				resource.type_id_name = rtype.type_id_name;
				if((resource.id & 0x8000) == 0)
				{
					rd.Seek(resource_table_offset + resource.id);
					uint8_t length = rd.ReadUnsigned(1);
					resource.id_name = rd.ReadData(length);
				}
			}
		}
	}

	file_size = std::max(file_size, rd.Tell());

	/* Resident name table */
	rd.Seek(resident_name_table_offset);
	resident_names.clear();
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

	/* Module reference table */
	rd.Seek(module_reference_table_offset);
	module_references.clear();
	for(i = 0; i < module_reference_count; i++)
	{
		module_references.emplace_back(ModuleReference(rd.ReadUnsigned(2)));
	}

	file_size = std::max(file_size, rd.Tell());

	// Load module names for convenience

	for(auto& module : module_references)
	{
		rd.Seek(imported_names_table_offset + module.name_offset);
		uint8_t length = rd.ReadUnsigned(1);
		module.name = rd.ReadData(length);
	}

	/* Imported name table */
	rd.Seek(imported_names_table_offset);
	imported_names.clear();
	while(rd.Tell() < entry_table_offset)
	{
		uint8_t length = rd.ReadUnsigned(1);
		imported_names.emplace_back(rd.ReadData(length));
	}

	file_size = std::max(file_size, rd.Tell());

	/* Entry table */
	rd.Seek(entry_table_offset);
	entries.clear();
	while(rd.Tell() < entry_table_offset + entry_table_length)
	{
		size_t entry_count = rd.ReadUnsigned(1);
		if(entry_count == 0)
			break;
		uint8_t indicator_byte = rd.ReadUnsigned(1);
		for(i = 0; i < entry_count; i ++)
		{
			entries.emplace_back(Entry::ReadEntry(rd, indicator_byte));
			entries.back().same_bundle = i != 0;
		}
	}

	file_size = std::max(file_size, rd.Tell());

	/* Nonresident name table */
	rd.Seek(nonresident_name_table_offset);
	nonresident_names.clear();
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

	/* Segment data */
	for(Segment& segment : segments)
	{
		if(segment.data_offset != 0)
		{
			rd.Seek(segment.data_offset);
			segment.image = Linker::Buffer::ReadFromFile(rd, segment.image_size);
		}
		else
		{
			segment.image = std::make_shared<Linker::Buffer>();
		}
		if((segment.flags & Segment::Relocations) != 0)
		{
			segment.relocations.clear();
			uint16_t count = rd.ReadUnsigned(2);
			for(i = 0; i < count; i++)
			{
				Segment::Relocation relocation;
				relocation.type = Segment::Relocation::source_type(rd.ReadUnsigned(1));
				relocation.flags = Segment::Relocation::flag_type(rd.ReadUnsigned(1));
				relocation.offsets.push_back(rd.ReadUnsigned(2));
				relocation.module = rd.ReadUnsigned(2);
				relocation.target = rd.ReadUnsigned(2);
				if((relocation.flags & Segment::Relocation::Additive) == 0)
				{
					uint16_t offset = relocation.offsets[0];
					auto image = segment.image->AsImage();
					while(true)
					{
						uint16_t new_offset = image->GetByte(offset) | (image->GetByte(offset + 1) << 8);
						if(new_offset == 0xFFFF)
							break;
						//Linker::Debug << "Debug: chained relocation from offset " << std::hex << offset << " to " << new_offset << std::endl;
						relocation.offsets.push_back(new_offset);
						offset = new_offset;
					}
				}
				segment.relocations.emplace_back(relocation);
			}
		}
		file_size = std::max(file_size, rd.Tell());
	}

	/* Resource data */
	if(IsOS2())
	{
		for(Resource& resource : resources)
		{
			if(resource.data_offset != 0)
			{
				rd.Seek(resource.data_offset);
				resource.image = Linker::Buffer::ReadFromFile(rd, resource.image_size);
			}
			file_size = std::max(file_size, rd.Tell());
		}
	}
	else
	{
		for(ResourceType& rtype : resource_types)
		{
			for(Resource& resource : rtype.resources)
			{
				if(resource.data_offset != 0)
				{
					rd.Seek(resource.data_offset);
					resource.image = Linker::Buffer::ReadFromFile(rd, resource.image_size);
				}
				file_size = std::max(file_size, rd.Tell());
			}
		}
	}

	/* Load entry descriptions for readability */

	for(auto& name : resident_names)
	{
		if(0 < name.ordinal && name.ordinal <= entries.size())
		{
			entries[name.ordinal - 1].export_state = Entry::ExportByName;
			entries[name.ordinal - 1].entry_name = name.name;
		}
	}

	for(auto& name : nonresident_names)
	{
		if(0 < name.ordinal && name.ordinal <= entries.size())
		{
			entries[name.ordinal - 1].export_state = Entry::ExportByOrdinal;
			entries[name.ordinal - 1].entry_name = name.name;
		}
	}

	/* Load relocation descriptions for readability */
	for(auto& segment : segments)
	{
		for(auto& relocation : segment.relocations)
		{
			if((relocation.flags & Segment::Relocation::TargetTypeMask) == Segment::Relocation::ImportOrdinal
			|| (relocation.flags & Segment::Relocation::TargetTypeMask) == Segment::Relocation::ImportName)
			{
				if(0 < relocation.module && relocation.module <= module_references.size())
				{
					relocation.module_name = module_references[relocation.module - 1].name;
				}
			}

			if((relocation.flags & Segment::Relocation::TargetTypeMask) == Segment::Relocation::ImportName)
			{
				rd.Seek(imported_names_table_offset + relocation.target);
				uint8_t length = rd.ReadUnsigned(1);
				relocation.import_name = rd.ReadData(length);
			}

			if((relocation.flags & Segment::Relocation::TargetTypeMask) == Segment::Relocation::Internal && (relocation.module & 0xFF) == 0xFF)
			{
				if(0 < relocation.target && relocation.target <= entries.size())
				{
					relocation.actual_segment = entries[relocation.target - 1].segment;
					relocation.actual_offset = entries[relocation.target - 1].offset;
					relocation.import_name = entries[relocation.target - 1].entry_name;
				}
			}
		}
	}
}

offset_t NEFormat::ImageSize() const
{
	return file_size;
}

offset_t NEFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	stub.WriteStubImage(wr);
	/* new header */
	wr.Seek(file_offset);
	wr.WriteData(signature);
	wr.WriteWord(1, linker_version.major);
	wr.WriteWord(1, linker_version.minor);
	wr.WriteWord(2, entry_table_offset - file_offset);
	wr.WriteWord(2, entry_table_length);
	wr.WriteWord(4, crc32);
	wr.WriteWord(1, program_flags);
	wr.WriteWord(1, application_flags);
	wr.WriteWord(2, automatic_data);
	wr.WriteWord(2, heap_size);
	wr.WriteWord(2, stack_size);
	wr.WriteWord(2, ip);
	wr.WriteWord(2, cs);
	wr.WriteWord(2, sp);
	wr.WriteWord(2, ss);
	wr.WriteWord(2, segments.size());
	wr.WriteWord(2, module_references.size());
	wr.WriteWord(2, nonresident_name_table_length);
	wr.WriteWord(2, segment_table_offset - file_offset);
	wr.WriteWord(2, resource_table_offset - file_offset);
	wr.WriteWord(2, resident_name_table_offset - file_offset);
	wr.WriteWord(2, module_reference_table_offset - file_offset);
	wr.WriteWord(2, imported_names_table_offset - file_offset);
	wr.WriteWord(4, nonresident_name_table_offset);
	wr.WriteWord(2, movable_entry_count);
	wr.WriteWord(2, sector_shift);
	wr.WriteWord(2, resource_count);
	wr.WriteWord(1, system);
	wr.WriteWord(1, additional_flags);
	wr.WriteWord(2, fast_load_area_offset);
	wr.WriteWord(2, fast_load_area_length);
	wr.WriteWord(2, code_swap_area_length);
	/* following Watcom */
	wr.WriteWord(1, windows_version.minor);
	wr.WriteWord(1, windows_version.major);

	/* Segment table */
	wr.Seek(segment_table_offset);
	for(const Segment& segment : segments)
	{
		wr.WriteWord(2, segment.data_offset >> sector_shift);
		wr.WriteWord(2, segment.image->ImageSize());
		wr.WriteWord(2, segment.flags);
		wr.WriteWord(2, segment.total_size);
	}

	/* Resource table */
	if(resource_types.size() != 0)
	{
		// TODO: test this out
		wr.Seek(resource_table_offset);
		for(auto& rtype : resource_types)
		{
			wr.WriteWord(2, rtype.type_id);
			wr.WriteWord(2, rtype.resources.size());
			wr.Skip(4);
			for(auto& resource : rtype.resources)
			{
				wr.WriteWord(2, resource.image->ImageSize());
				wr.WriteWord(2, resource.flags);
				wr.WriteWord(2, resource.id);
				wr.WriteWord(2, resource.handle);
				wr.WriteWord(2, resource.usage);
			}
		}
		wr.WriteWord(2, 0);
		for(auto& string : resource_strings)
		{
			wr.WriteWord(1, string.size());
			wr.WriteData(string);
		}
		wr.WriteWord(1, 0);
	}

	/* Resident name table */
	wr.Seek(resident_name_table_offset);
	for(const Name& name : resident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name.c_str());
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	/* Module reference table */
	wr.Seek(module_reference_table_offset);
	for(auto& module : module_references)
	{
		wr.WriteWord(2, module.name_offset);
	}

	/* Imported name table */
	wr.Seek(imported_names_table_offset);
	for(const std::string& name : imported_names)
	{
		wr.WriteWord(1, name.size());
		wr.WriteData(name.size(), name.c_str());
	}

	/* Entry table */
	wr.Seek(entry_table_offset);
	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		wr.WriteWord(1, entry_count);
		wr.WriteWord(1, entries[entry_index].GetIndicatorByte());
		for(size_t entry_offset = 0; entry_offset < entry_count; entry_offset ++)
		{
			entries[entry_index + entry_offset].WriteEntry(wr);
		}
		entry_index += entry_count;
	}
	wr.WriteWord(2, 0);

	/* Nonresident name table */
	wr.Seek(nonresident_name_table_offset);
	for(const Name& name : nonresident_names)
	{
		wr.WriteWord(1, name.name.size());
		wr.WriteData(name.name.size(), name.name.c_str());
		wr.WriteWord(2, name.ordinal);
	}
	wr.WriteWord(1, 0);

	for(const Segment& segment : segments)
	{
		wr.Seek(segment.data_offset);
		segment.image->WriteFile(wr);
		if(segment.relocations.size() != 0)
		{
			wr.WriteWord(2, segment.relocations.size());
			for(auto& it : segment.relocations)
			{
				wr.WriteWord(1, it.type);
				wr.WriteWord(1, it.flags);
				wr.WriteWord(2, it.offsets[0]);
				wr.WriteWord(2, it.module);
				wr.WriteWord(2, it.target);
				// TODO: check that this works
				for(unsigned i = 1; i < it.offsets.size(); i++)
				{
					wr.Seek(segment.data_offset + it.offsets[i - 1]);
					wr.WriteWord(2, it.offsets[i]);
				}
			}
		}
	}

	/* Resource data */
	// TODO: needs testing
	if(IsOS2())
	{
		for(const Resource& resource : resources)
		{
			if(resource.image != nullptr)
			{
				wr.Seek(resource.data_offset);
				resource.image->WriteFile(wr);
			}
		}
	}
	else
	{
		for(const ResourceType& rtype : resource_types)
		{
			for(const Resource& resource : rtype.resources)
			{
				if(resource.image != nullptr)
				{
					wr.Seek(resource.data_offset);
					resource.image->WriteFile(wr);
				}
			}
		}
	}

	return offset_t(-1);
}

void NEFormat::Dump(Dumper::Dumper& dump) const
{
	offset_t i;
	offset_t current_offset;

	dump.SetEncoding(Dumper::Block::encoding_windows1252);

	dump.SetTitle("NE format");
	Dumper::Region file_region("File", file_offset, file_size, 8);
	file_region.Display(dump);

	Dumper::Region header_region("New header", file_offset, segment_table_offset - file_offset, 8);
	header_region.AddField("Signature", Dumper::StringDisplay::Make("'"), std::string(signature.data(), 2));
	header_region.AddField("Version", Dumper::VersionDisplay::Make(), offset_t(linker_version.major), offset_t(linker_version.minor));
	header_region.AddOptionalField("Checksum", Dumper::HexDisplay::Make(8), offset_t(crc32));
	header_region.AddOptionalField("Automatic data segment", Dumper::DecDisplay::Make(), offset_t(automatic_data));
	header_region.AddField("Initial heap size", Dumper::HexDisplay::Make(4), offset_t(heap_size));
	header_region.AddField("Initial stack size", Dumper::HexDisplay::Make(4), offset_t(stack_size));
	header_region.AddField("Entry point (CS:IP)", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(cs), offset_t(ip));
	header_region.AddField("Initial stack (SS:SP)", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(ss), offset_t(sp));

	static const std::map<offset_t, std::string> os_descriptions =
	{
		{ OS2, "Microsoft&IBM OS/2 1.0 - 1.3" },
		{ Windows, "Microsoft Windows 1.0 - 3.11" },
		{ MSDOS4, "Multitasking/\"European\" Microsoft MS-DOS 4.0" },
		{ Windows386, "Windows386" },
		{ BorlandOSS, "Borland Operating System Services" },
		{ OS2 | PharLap, "Pharlap 286|DOS-Extender, OS/2 simulation" },
		{ Windows | PharLap, "Pharlap 286|DOS-Extender, Windows simulation" },
	};
	header_region.AddField("Operating system", Dumper::ChoiceDisplay::Make(os_descriptions), offset_t(system));

	static const std::map<offset_t, std::string> data_count_descriptions =
	{
		{ NODATA, "NODATA - No automatic data segment" },
		{ SINGLEDATA, "SINGLEDATA - Single automatic data segment instance shared (DLLs)" },
		{ MULTIPLEDATA, "MULTIPLEDATA - Separate automatic data segment instances shared (EXEs)" },
	};
	header_region.AddField("Program flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 2, Dumper::ChoiceDisplay::Make(data_count_descriptions), false)
			->AddBitField(2, 1,
				IsLibrary()
					? Dumper::ChoiceDisplay::Make("per-process library initialization", "global library initialization")
					: Dumper::ChoiceDisplay::Make("per-process library initialization"),
				true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("protected mode only"), true)
			->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("(Windows) uses LIM EMS directly/(OS/2) 8086 instructions"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("(Windows) each instance uses separate EMS bank/(OS/2) requires 80286"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("(Windows) for DLLs, global memory above EMS line/(OS/2) requires 80386"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("requires 80x87"), true),
		offset_t(program_flags));
	static const std::map<offset_t, std::string> gui_descriptions =
	{
		{ FULLSCREEN, "Fullscreen" },
		{ GUI_AWARE, "GUI compatible" },
		{ GUI, "GUI" },
	};
	header_region.AddField("Application flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 2, Dumper::ChoiceDisplay::Make(gui_descriptions), false)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("(Windows) first segment contains code to load application/(OS/2) Family Application (runs on DOS as well)"), true)
			->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("errors detected during linking"), true)
			->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("non-conforming program, no valid stack maintained/private DLL"), true)
			->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("library (DLL)", "application (EXE)"), false),
		offset_t(application_flags));
	header_region.AddField("Additional flags",
		Dumper::BitFieldDisplay::Make(2)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("(OS/2) support long filenames"), true)
		// the official Microsoft documentation mixes up these two fields
			->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("Windows 2.x application supporting proportional fonts"), true)
			->AddBitField(2, 1, Dumper::ChoiceDisplay::Make("Windows 2.x application running in protected mode"), true)
			->AddBitField(3, 1, Dumper::ChoiceDisplay::Make("(Windows) contains fast load area"), true),
		offset_t(additional_flags));
	header_region.AddOptionalField(IsOS2() ? "Offset to return thunks" : "Offset to fast load area", Dumper::HexDisplay::Make(8), offset_t(fast_load_area_offset) << sector_shift);
	header_region.AddOptionalField(IsOS2() ? "Offset to segment reference thunks" : "Offset to fast load length", Dumper::HexDisplay::Make(8), offset_t(fast_load_area_length) << sector_shift);
	header_region.AddOptionalField("Minimum code swap area size", Dumper::HexDisplay::Make(4), offset_t(code_swap_area_length));
	header_region.AddOptionalField("Minimal Windows version", Dumper::VersionDisplay::Make(), offset_t(windows_version.major), offset_t(windows_version.minor));
	header_region.Display(dump);

	offset_t segment_count = segments.size();
	if(IsOS2())
	{
		segment_count += resources.size();
	}
	Dumper::Region segment_table_region("Segment table", segment_table_offset, segment_count * 8, 8);
	if(IsOS2())
	{
		segment_table_region.AddField("Entry count", Dumper::DecDisplay::Make(), offset_t(segment_count));
	}
	segment_table_region.AddField("Segment count", Dumper::DecDisplay::Make(), offset_t(segments.size()));
	segment_table_region.AddField("Sector shift count", Dumper::DecDisplay::Make(), offset_t(sector_shift));
	segment_table_region.Display(dump);

	if(IsOS2())
	{
		Dumper::Region resource_table_region("Resource table", resource_table_offset, resource_count * 4, 8);
		resource_table_region.AddField("Resource count", Dumper::DecDisplay::Make(), offset_t(resource_count));
		resource_table_region.Display(dump);
	}
	else
	{
		Dumper::Region resource_table_region("Resource table", resource_table_offset, resident_name_table_offset - resource_table_offset, 8);
		resource_table_region.AddField("Resource count", Dumper::DecDisplay::Make(), offset_t(resource_count));
		resource_table_region.AddField("Sector shift count", Dumper::DecDisplay::Make(), offset_t(resource_shift));
		resource_table_region.Display(dump);

		// calculate the offset of the first string
		current_offset = resource_table_offset + 4;
		for(auto& rtype : resource_types)
		{
			current_offset += 8 + 12 * rtype.resources.size();
		}

		i = 0;
		for(auto& string : resource_strings)
		{
			Dumper::Entry string_entry("String", i + 1, current_offset, 8);
			string_entry.AddField("Name", Dumper::StringDisplay::Make("\""), string);
			string_entry.Display(dump);
			current_offset += string.size() + 1;
			i++;
		}
	}

	Dumper::Region resident_name_table_region("Resident name table", resident_name_table_offset, module_reference_table_offset - resident_name_table_offset, 8);
	resident_name_table_region.Display(dump);

	i = 0;
	current_offset = resident_name_table_offset;
	for(auto& name : resident_names)
	{
		Dumper::Entry name_entry("Name", i + 1, current_offset, 8);
		name_entry.AddField("Name", Dumper::StringDisplay::Make("'"), name.name);
		name_entry.AddField("Ordinal", Dumper::HexDisplay::Make(4), offset_t(name.ordinal));
		name_entry.Display(dump);
		current_offset += name.name.size() + 3;
		i++;
	}

	Dumper::Region module_reference_table_region("Module reference table", module_reference_table_offset, module_references.size(), 8);
	module_reference_table_region.Display(dump);
	i = 0;
	for(auto& module : module_references)
	{
		Dumper::Entry name_entry("Module", i + 1, module_reference_table_offset + i * 2, 8);
		name_entry.AddField("Name", Dumper::StringDisplay::Make(), module.name);
		name_entry.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(imported_names_table_offset + module.name_offset));
		name_entry.Display(dump);
		i++;
	}

	Dumper::Region imported_names_table_region("Imported names table", imported_names_table_offset, entry_table_offset - imported_names_table_offset, 8);
	imported_names_table_region.Display(dump);

	i = 0;
	current_offset = imported_names_table_offset;
	for(auto& name : imported_names)
	{
		Dumper::Entry name_entry("Name", i + 1, current_offset, 8);
		name_entry.AddField("Name", Dumper::StringDisplay::Make("'"), name);
		name_entry.Display(dump);
		current_offset += name.size() + 1;
		i++;
	}

	Dumper::Region entry_table_region("Entry table", entry_table_offset, entry_table_length, 8);
	entry_table_region.AddField("Movable entry count", Dumper::DecDisplay::Make(), offset_t(movable_entry_count));
	entry_table_region.Display(dump);

	i = 0;
	current_offset = entry_table_offset;
	static const std::map<offset_t, std::string> type_descriptions =
	{
		{ Entry::Unused, "Unused" },
		{ Entry::Fixed, "Fixed" },
		{ Entry::Movable, "Movable" },
	};
	static const std::map<offset_t, std::string> export_descriptions =
	{
		{ Entry::ExportByName, "by name" },
		{ Entry::ExportByOrdinal, "by ordinal" },
	};

	for(auto& entry : entries)
	{
		Dumper::Entry call_entry("Entry", i + 1, current_offset, 8);
		call_entry.AddField("Ordinal", Dumper::HexDisplay::Make(4), offset_t(i + 1));
		call_entry.AddField("Type", Dumper::ChoiceDisplay::Make(type_descriptions), offset_t(entry.type));
		if(entry.type != Entry::Unused)
		{
			if(entry.segment == Entry::CONSTANT_VALUE) // TODO: make dynamic?
				call_entry.AddField("Value", Dumper::HexDisplay::Make(4), offset_t(entry.offset));
			else
				call_entry.AddField("Location", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(4)), offset_t(entry.segment), offset_t(entry.offset));
			call_entry.AddField("Flags",
				Dumper::BitFieldDisplay::Make(2)
					->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("exported"), true)
					->AddBitField(1, 1, Dumper::ChoiceDisplay::Make("shared data"), true),
				offset_t(entry.flags));
			if(entry.export_state != Entry::NotExported)
			{
				call_entry.AddField("Export", Dumper::ChoiceDisplay::Make(export_descriptions), offset_t(entry.export_state));
				call_entry.AddField("Name", Dumper::StringDisplay::Make(), entry.entry_name);
			}
		}
		call_entry.Display(dump);
		current_offset += entry.GetEntrySize();
		if(!entry.same_bundle)
			current_offset += 2;
		i++;
	}

	Dumper::Region nonresident_name_table_region("Non-resident name table", nonresident_name_table_offset, nonresident_name_table_length, 8);
	nonresident_name_table_region.Display(dump);

	i = 0;
	current_offset = nonresident_name_table_offset;
	for(auto& name : nonresident_names)
	{
		Dumper::Entry name_entry("Name", i + 1, current_offset, 8);
		name_entry.AddField("Name", Dumper::StringDisplay::Make("'"), name.name);
		name_entry.AddField("Ordinal", Dumper::HexDisplay::Make(4), offset_t(name.ordinal));
		name_entry.Display(dump);
		current_offset += name.name.size() + 3;
		i++;
	}

	i = 0;
	for(auto& segment : segments)
	{
		segment.Dump(dump, i, IsOS2());
		i++;
	}

	if(IsOS2())
	{
		for(auto& resource : resources)
		{
			resource.Dump(dump, i, true);
			i++;
		}
	}
	else
	{
		i = 0;
		for(auto& rtype : resource_types)
		{
			for(auto& resource : rtype.resources)
			{
				resource.Dump(dump, i, false);
				i++;
			}
		}
	}
}

void NEFormat::SetTargetDefaults()
{
	switch(output)
	{
	case OUTPUT_CON:
		switch(system)
		{
		case OS2:
			program_flags = MULTIPLEDATA;
			application_flags = GUI_AWARE;
			break;
		case MSDOS4:
			program_flags = program_flag_type(MULTIPLEDATA | GLOBAL_INITIALIZATION);
			application_flags = application_flag_type(0);
			break;
		default:
			Linker::FatalError("Error: invalid target system");
		}
		break;
	case OUTPUT_GUI:
		switch(system)
		{
		case OS2:
			program_flags = MULTIPLEDATA;
			application_flags = GUI;
			break;
		case Windows:
			program_flags = MULTIPLEDATA;
			application_flags = GUI_AWARE;
			break;
		default:
			Linker::FatalError("Error: invalid target system");
		}
		break;
	case OUTPUT_DLL:
		switch(system)
		{
		case OS2:
		case Windows:
			program_flags = SINGLEDATA;
			application_flags = application_flag_type(GUI_AWARE | LIBRARY);
			break;
		case MSDOS4:
			program_flags = program_flag_type(SINGLEDATA | GLOBAL_INITIALIZATION);
			application_flags = LIBRARY;
			break;
		default:
			Linker::FatalError("Error: invalid target system");
		}
		break;
	}
}

bool NEFormat::FormatSupportsSegmentation() const
{
	return true;
}

bool NEFormat::FormatIs16bit() const
{
	return true;
}

bool NEFormat::FormatIsProtectedMode() const
{
	/* Some operating systems supporting NE can run in real as well as protected mode, so we must assume there are no paragraph references */
	return true;
}

bool NEFormat::FormatSupportsLibraries() const
{
	return true;
}

unsigned NEFormat::FormatAdditionalSectionFlags(std::string section_name) const
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

std::shared_ptr<NEFormat> NEFormat::SimulateLinker(compatibility_type compatibility)
{
	this->compatibility = compatibility;
	switch(compatibility)
	{
	case CompatibleNone:
		break;
	case CompatibleWatcom:
		linker_version = version{5, 1};
		break;
	case CompatibleMicrosoft:
		// TODO
		break;
	case CompatibleBorland:
		/* TODO */
		break;
	}
	return shared_from_this();
}

std::shared_ptr<NEFormat> NEFormat::CreateConsoleApplication(system_type system)
{
	auto fmt = std::make_shared<NEFormat>(system, OUTPUT_CON);
	fmt->SetTargetDefaults();
	return fmt;
}

std::shared_ptr<NEFormat> NEFormat::CreateGUIApplication(system_type system)
{
	auto fmt = std::make_shared<NEFormat>(system, OUTPUT_GUI);
	fmt->SetTargetDefaults();
	return fmt;
}

std::shared_ptr<NEFormat> NEFormat::CreateLibraryModule(system_type system)
{
	auto fmt = std::make_shared<NEFormat>(system, OUTPUT_DLL);
	fmt->SetTargetDefaults();
	return fmt;
}

unsigned NEFormat::GetCodeSegmentFlags() const
{
	switch(system)
	{
	case OS2:
		return 3 << Segment::PrivilegeLevelShift;
	case Windows:
		if(IsLibrary())
			return Segment::Movable | Segment::Shareable | Segment::Preload | (3 << Segment::PrivilegeLevelShift);
		else
			return Segment::Preload | (3 << Segment::PrivilegeLevelShift);
	case MSDOS4:
		return Segment::Movable | Segment::Shareable | Segment::Preload | (0 << Segment::PrivilegeLevelShift);
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

unsigned NEFormat::GetDataSegmentFlags() const
{
	switch(system)
	{
	case MSDOS4:
		if(IsLibrary())
			return Segment::Movable | Segment::Data | Segment::Preload | (0 << Segment::PrivilegeLevelShift);
		else
			return Segment::Data | (0 << Segment::PrivilegeLevelShift);
	default:
		return GetCodeSegmentFlags() | Segment::Data;
	}
}

void NEFormat::AddSegment(const Segment& segment)
{
	segments.push_back(segment);
	segment_index[std::dynamic_pointer_cast<Linker::Segment>(segments.back().image)] = segments.size() - 1;
}

uint16_t NEFormat::FetchModule(std::string name)
{
	auto it = module_reference_offsets.find(name);
	if(it == module_reference_offsets.end())
	{
		uint16_t module = FetchImportedName(name);
		module_references.push_back(ModuleReference(module, name));
		return module_reference_offsets[name] = module_references.size() - 1;
	}
	else
	{
		return it->second;
	}
}

uint16_t NEFormat::FetchImportedName(std::string name)
{
	auto it = imported_name_offsets.find(name);
	if(it == imported_name_offsets.end())
	{
		uint16_t offset = imported_names_length;
		Linker::Debug << "Debug: New imported name: " << name << " = " << offset << std::endl;
		imported_names.push_back(name);
		imported_name_offsets[name] = offset;
		imported_names_length += 1 + name.size();
		return offset;
	}
	else
	{
		return it->second;
	}
}

std::string NEFormat::MakeProcedureName(std::string name)
{
	if(option_capitalize_names)
	{
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
	}
	return name;
}

uint16_t NEFormat::MakeEntry(Linker::Position value)
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

uint16_t NEFormat::MakeEntry(uint16_t index, Linker::Position value)
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
	uint16_t segment = segment_index[value.segment];
	/* TODO: other flags? */
	if((segments[segment].flags & Segment::Movable))
	{
		entries[index] = Entry(Entry::Movable, segment + 1, Entry::Exported | Entry::SharedData, value.address);
	}
	else
	{
		entries[index] = Entry(Entry::Fixed, segment + 1, Entry::Exported, value.address);
	}
	return index;
}

uint8_t NEFormat::CountBundles(size_t entry_index) const
{
	size_t entry_count;
	entries[entry_index].same_bundle = false;
	for(entry_count = 1; entry_count < 0xFF && entry_index + entry_count < entries.size(); entry_count++)
	{
		if(entries[entry_index].type != entries[entry_index + entry_count].type)
		{
			break;
		}
		if(entries[entry_index].type == Entry::Fixed && entries[entry_index].segment != entries[entry_index + entry_count].segment)
		{
			break;
		}
		entries[entry_index + entry_count].same_bundle = true;
	}
	if(entry_index + entry_count < entries.size())
		entries[entry_index + entry_count].same_bundle = false;
	return entry_count;
}

std::shared_ptr<Linker::OptionCollector> NEFormat::GetOptions()
{
	return std::make_shared<NEOptionCollector>();
}

void NEFormat::SetOptions(std::map<std::string, std::string>& options)
{
	NEOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();

	if(collector.system())
	{
		system = collector.system();
	}

	if(collector.type())
	{
		output = collector.type();
	}

	SetTargetDefaults();

	if(collector.compat())
	{
		compatibility = collector.compat();
	}

	switch(system & ~PharLap)
	{
	case Windows:
		option_capitalize_names = true;
		break;
	case OS2:
	default: /* TODO: other systems? */
		option_capitalize_names = false;
		break;
	}
	/* TODO */
}

void NEFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	/* TODO: stack, heap */

	if(segment->name == ".heap")
	{
		//heap = segment;
	}
	else if(segment->name == ".stack")
	{
		stack = segment;
	}
	else
	{
Linker::Debug << "Debug: New segment " << segment->name << std::endl;
		AddSegment(Segment(segment, segment->sections[0]->IsExecutable() ? GetCodeSegmentFlags() : GetDataSegmentFlags()));

		if(segment->name == ".data")
			automatic_data = segments.size();
	}
}

std::unique_ptr<Script::List> NEFormat::GetScript(Linker::Module& module)
{
	static const char SmallScript[] = R"(
".code"
{
	all exec;
};

".data"
{
	at 0;
	all ".beg.data"; # for Win16, must come before everything
	all not zero;
	all not ".stack";
};

".stack"
{
	all;
};
)";

	static const char SmallScript_msdos4[] = R"(
".code"
{
	all exec;
};

".data"
{
	at 0;
	all not zero;
	all not ".stack";
	all;
};
)";

	/* TODO: stack, heap */
	static const char LargeScript[] = R"(
".code"
{
	all ".code" or ".text";
};

".data"
{
	at 0;
	all ".data" or ".rodata";
	all ".bss" or ".comm";
	all ".stack";
};

for not ".heap"
{
	at 0;
	all any and not zero;
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
		case MODEL_SMALL:
			if(system == MSDOS4)
			{
				return Script::parse_string(SmallScript_msdos4);
			}
			else
			{
				return Script::parse_string(SmallScript);
			}
		case MODEL_LARGE:
			/* TODO: this is MSDOS4 only */
			return Script::parse_string(LargeScript);
		default:
			Linker::FatalError("Internal error: invalid memory model");
		}
	}
}

void NEFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void NEFormat::ProcessModule(Linker::Module& module)
{
	sector_shift = 1; /* TODO: parametrize */
	for(auto& section : module.Sections())
	{
		section->RealignEnd(1 << sector_shift); /* TODO: this is probably what Watcom does */
	}

	Link(module);

	resident_names.push_back(Name{module_name, 0});
	FetchImportedName("");
	nonresident_names.push_back(Name{program_name, 0});

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string name;
		if(symbol.LoadName(name))
		{
			FetchImportedName(MakeProcedureName(name));
		}
	}

	for(const Linker::SymbolName& symbol : module.GetImportedSymbols())
	{
		std::string library;
		if(symbol.LoadLibraryName(library))
		{
			FetchModule(library);
		}
	}

	/* first make entries for those symbols exported by ordinals, or those that have hints */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(it.first.LoadOrdinalOrHint(ordinal))
		{
			// TODO: entry instances can now store the name and export state
			MakeEntry(ordinal - 1, it.second.GetPosition());
			if(it.first.IsExportedByOrdinal())
			{
				std::string internal_name = "";
				it.first.LoadName(internal_name);
				nonresident_names.push_back(Name{MakeProcedureName(internal_name), ordinal});
			}
		}
	}

	/* then make entries for those symbols exported by name */
	for(auto it : module.GetExportedSymbols())
	{
		uint16_t ordinal;
		if(!it.first.LoadOrdinalOrHint(ordinal))
		{
			// TODO: entry instances can now store the name and export state
			ordinal = MakeEntry(it.second.GetPosition()) + 1;
		}
		if(!it.first.IsExportedByOrdinal())
		{
			std::string exported_name = "";
			it.first.LoadName(exported_name);
			resident_names.push_back(Name{MakeProcedureName(exported_name), ordinal});
		}
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
					Linker::Error << rel << std::endl;
					continue;
				}
				Linker::Position position = rel.source.GetPosition();
				Segment& source_segment = segments[segment_index[position.segment]];
				uint8_t target_segment = segment_index[resolution.target];
				Segment& target_segment_object = segments[target_segment];
				target_segment += 1;
				int type = Segment::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				else if((target_segment_object.flags & Segment::Movable))
				{
					/* references to movable segments require a new entry point */
					assert(resolution.value == 0); /* TODO: target != 0 ? */
					if(target_segment_object.movable_entry_index == 0)
					{
						entries.push_back(Entry(Entry::Movable, target_segment, 0, 0));
						target_segment_object.movable_entry_index = entries.size();
					}
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::Internal,
							position.address, 0xFF, target_segment_object.movable_entry_index)
					);
					rel.WriteWord(0xFFFFFFFF);
				}
				else
				{
					//assert(resolution.value == 0); /* TODO: target != 0 ? */
					if(rel.kind == Linker::Relocation::SelectorIndex)
						resolution.value = (resolution.value >> 4) + rel.addend; // TODO: ugly hack in case the program is executed in real mode (should we change FormatIsProtectedMode?)
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::Internal,
							position.address, target_segment, resolution.value)
					);
					rel.WriteWord(0xFFFFFFFF);
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
				Segment& source_segment = segments[segment_index[position.segment]];

				std::string library, name;
				uint16_t ordinal;
				int type = Segment::Relocation::GetType(rel);
				if(type == -1)
				{
					Linker::Error << "Error: unknown relocation size, ignoring" << std::endl;
					continue;
				}
				if(symbol->GetImportedName(library, name))
				{
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::ImportName,
							position.address, FetchModule(library) + 1, FetchImportedName(MakeProcedureName(name)))
					);
					rel.WriteWord(0xFFFFFFFF); // gets truncated for shorter relocations
					/* TODO: if not additive? */
					continue;
				}
				else if(symbol->GetImportedOrdinal(library, ordinal))
				{
					source_segment.AddRelocation(
						Segment::Relocation(type, Segment::Relocation::ImportOrdinal,
							position.address, FetchModule(library) + 1, ordinal)
					);
					rel.WriteWord(0xFFFFFFFF); // gets truncated for shorter relocations
					/* TODO: if not additive? */
					continue;
				}
			}
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
	}

	for(auto& segment : segments)
	{
		segment.relocations.clear();
		for(auto& pair : segment.relocations_map)
		{
			segment.relocations.emplace_back(pair.second);
		}
	}

	/* TODO: allocate stack instead */
	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Position position = stack_top.GetPosition();
		sp = position.address;
		ss = segment_index[position.segment] + 1;
	}
	else if(system != MSDOS4)
	{
		/* top of automatic data segment */
		sp = 0;
		ss = automatic_data;
	}
	else
	{
		/* Multitasking MS-DOS 4 */
		if(automatic_data == 0)
		{
			sp = 0;
			ss = 0;
		}
		else
		{
			sp = std::dynamic_pointer_cast<Linker::Segment>(segments[automatic_data - 1].image)->TotalSize();
			ss = automatic_data;
//			Linker::Debug << "Debug: End of memory: " << sp << std::endl;
//			Linker::Debug << "Debug: Total size: " << image.TotalSize() << std::endl;
//			Linker::Debug << "Debug: Stack base: " << ss << std::endl;
//			if(!(stack->GetFlags() & Linker::Section::Stack))
//			{
//				Linker::Warning << "Warning: no stack top specified, using end of .bss segment" << std::endl;
//			}
		}
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		Linker::Position position = entry.GetPosition();
		ip = position.address;
		cs = segment_index[position.segment] + 1;
	}
	else
	{
		ip = 0;
		cs = 1;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void NEFormat::CalculateValues()
{
	offset_t current_offset;

	if((program_flags & (SINGLEDATA|MULTIPLEDATA)) == NODATA)
		automatic_data = 0;

	/* TODO: should be a parameter */
	switch(system)
	{
	case MSDOS4:
		stack_size = 0;
		heap_size = 0;
		windows_version.major = 0;
		windows_version.minor = 0;
		break;
	case OS2:
		stack_size = 0x1000;
		heap_size = 0;
		windows_version.major = 0;
		windows_version.minor = 0;
		break;
	case Windows:
		stack_size = 0x2000;
		heap_size = 0x400;
		windows_version.major = 3;
		windows_version.minor = 0;
		break;
	default:
		Linker::FatalError("Internal error: invalid target system");
	}

	if(ss != automatic_data)
		stack_size = 0; /* no initial stack possible */

	if(IsLibrary())
		stack_size = ss = sp = 0;

	// TODO: provide option to suppress stub generation
	file_offset = stub.GetStubImageSize();

	segment_table_offset = file_offset + 0x40;

	resource_table_offset = segment_table_offset + 8 * segments.size();

	resource_count = 0;
	for(auto& rtype : resource_types)
	{
		resource_count += rtype.resources.size();
	}

	resident_name_table_offset = resource_table_offset + 0; /* TODO: store resource table */
	current_offset = resident_name_table_offset + 1;
	for(Name& name : resident_names)
	{
		current_offset += 3 + name.name.size();
	}

	module_reference_table_offset = current_offset;

	current_offset = imported_names_table_offset = module_reference_table_offset + 2 * module_references.size();
	for(std::string& name : imported_names)
	{
		current_offset += 1 + name.size();
	}

	entry_table_offset = current_offset;
	entry_table_length = 2;
	movable_entry_count = 0;

	for(size_t entry_index = 0; entry_index < entries.size();)
	{
		size_t entry_count = CountBundles(entry_index);
		entry_table_length += 2 + entry_count * entries[entry_index].GetEntrySize();
		if(entries[entry_index].type == Entry::Movable)
			movable_entry_count += entry_count;
		entry_index += entry_count;
	}

	nonresident_name_table_offset = entry_table_offset + entry_table_length;
	nonresident_name_table_length = 1;
	for(Name& name : nonresident_names)
	{
		nonresident_name_table_length += 3 + name.name.size();
	}

	current_offset = nonresident_name_table_offset + nonresident_name_table_length;

	for(Segment& segment : segments)
	{
		segment.data_offset = ::AlignTo(current_offset, 1 << sector_shift);
		current_offset = segment.data_offset + segment.image->ImageSize();
		if(Linker::Segment * segmentp = dynamic_cast<Linker::Segment *>(segment.image.get()))
		{
			segment.total_size = segmentp->TotalSize();
		}
		else
		{
			segment.total_size = segment.image->ImageSize();
		}

		if(segment.relocations.size() != 0)
		{
			segment.flags = Segment::flag_type(segment.flags | Segment::Relocations);
			current_offset += 2 + 8 * segment.relocations.size();
		}
	}

	file_size = current_offset;

	/* TODO: these are not going to be implemented */
	fast_load_area_offset = 0 >> sector_shift;
	fast_load_area_length = 0 >> sector_shift;
}

void NEFormat::SetModel(std::string model)
{
	if(model == "" || model == "small")
	{
		memory_model = MODEL_SMALL;
	}
	else if(model == "large")
	{
		memory_model = MODEL_LARGE;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
		memory_model = MODEL_SMALL;
	}
}

void NEFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	program_name = filename;
	size_t ix = filename.rfind('.');
	if(ix != std::string::npos)
		module_name = filename.substr(0, ix);
	else
		module_name = filename;
	if(module.cpu != Linker::Module::I86)
	{
		Linker::FatalError("Fatal error: Format only supports Intel 8086 binaries");
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string NEFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	ResourceFile resfil;

	if(output == OUTPUT_DLL && ((system & ~0x7F) == OS2 || (system & ~0x7F) == Windows || (system & ~0x7F) == Windows386))
		return filename + ".dll";
	else
		return filename + ".exe";
}

void ResourceFile::ReadIdentifier(Linker::Reader& rd, Identifier& id)
{
	uint8_t first_byte;

	first_byte = rd.ReadUnsigned(1);
	if(first_byte == 0xFF)
	{
		id = uint16_t(rd.ReadUnsigned(2, ::LittleEndian));
	}
	else
	{
		rd.Skip(-1);
		id = rd.ReadASCIIZ();
	}
}

void ResourceFile::WriteIdentifier(Linker::Writer& wr, const Identifier& id)
{
	if(auto ordinal_p = std::get_if<uint16_t>(&id))
	{
		wr.WriteWord(1, 0xFF);
		wr.WriteWord(2, *ordinal_p, ::LittleEndian);
	}
	else if(auto string_p = std::get_if<std::string>(&id))
	{
		wr.WriteData(*string_p);
		wr.WriteWord(1, 0);
	}
	else
	{
		assert(false);
	}
}

offset_t ResourceFile::GetIdentifierSize(const Identifier& id)
{
	if(std::get_if<uint16_t>(&id))
	{
		return 3;
	}
	else if(auto string_p = std::get_if<std::string>(&id))
	{
		return 1 + string_p->size();
	}
	else
	{
		assert(false);
	}
}

void ResourceFile::ReadFile(Linker::Reader& rd)
{
	offset_t starting_offset = rd.Tell();
	rd.SeekEnd();
	offset_t ending_offset = rd.Tell();
	rd.Seek(starting_offset);

	while(rd.Tell() < ending_offset)
	{
		Resource resource;

		ReadIdentifier(rd, resource.type);
		ReadIdentifier(rd, resource.name);

		resource.flags = rd.ReadUnsigned(2, ::LittleEndian);

		uint32_t size = rd.ReadUnsigned(4, ::LittleEndian);
		resource.image = Linker::Buffer::ReadFromFile(rd, size);

		resources.emplace_back(resource);
	}
}

offset_t ResourceFile::WriteFile(Linker::Writer& wr) const
{
	for(auto& resource : resources)
	{
		WriteIdentifier(wr, resource.type);
		WriteIdentifier(wr, resource.name);

		wr.WriteWord(2, resource.flags, ::LittleEndian);
		wr.WriteWord(4, resource.image->ImageSize(), ::LittleEndian);
		resource.image->WriteFile(wr);
	}
	return offset_t(-1); // TODO
}

void ResourceFile::Dump(Dumper::Dumper& dump) const
{
	if(dump.encoding == nullptr)
		dump.SetEncoding(Dumper::Block::encoding_windows1252);

	static const std::map<offset_t, std::string> empty_map;
	const std::map<offset_t, std::string> * resource_type_id_descriptions;

	switch(system)
	{
	case System_Unspecified:
	default:
		resource_type_id_descriptions = &empty_map;
		break;
	case System_Windows:
	case System_Windows_1x:
	case System_Windows_3x:
		resource_type_id_descriptions = &Windows::resource_type_id_descriptions;
		break;
	case System_OS2:
		resource_type_id_descriptions = &OS2::resource_type_id_descriptions;
		break;
	}

	offset_t current_offset = 0;
	for(auto& resource : resources)
	{
		current_offset += GetIdentifierSize(resource.type) + GetIdentifierSize(resource.name) + 6;
		Dumper::Block resource_block("Resource", current_offset, resource.image->AsImage(), 0, 8);
		resource_block.AddField("Type", IdDisplay::Make(*resource_type_id_descriptions), resource.type);
		resource_block.AddField("Name", IdDisplay::Make(), resource.name);
		resource_block.AddField("Flags",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("movable", "fixed"), false)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("pure", "impure"), false)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("load on call", "preload"), false)
				->AddBitField(12, 1, Dumper::ChoiceDisplay::Make("discardable"), true),
			offset_t(resource.flags));
		resource_block.Display(dump);
		current_offset += resource.image->ImageSize();
	}
}

void ResourceFile::IdDisplay::DisplayValue(Dumper::Dumper& dump, std::tuple<Identifier> values)
{
	auto id = std::get<0>(values);
	if(auto ordinal_p = std::get_if<uint16_t>(&id))
	{
		dump.PrintHex(*ordinal_p, 4);
		auto it = names.find(*ordinal_p);
		if(it != names.end())
		{
			dump.out << " (" << it->second << ")";
		}
	}
	else if(auto string_p = std::get_if<std::string>(&id))
	{
		dump.out << '\'';
		Dumper::StringDisplay display(-1, "", "");
		dump.PutEncodedString(*string_p);
		dump.out << '\'';
	}
	else
	{
		assert(false);
	}
}

std::shared_ptr<ResourceFile::IdDisplay> ResourceFile::IdDisplay::Make()
{
	return std::make_shared<IdDisplay>();
}

std::shared_ptr<ResourceFile::IdDisplay> ResourceFile::IdDisplay::Make(const std::map<offset_t, std::string>& names)
{
	return std::make_shared<IdDisplay>(names);
}

