
#include "pefexe.h"
#include "../linker/location.h"

/* TODO: unimplemented */

using namespace Apple;

uint32_t PEFFormat::PatternInitialization::ReadValue(Linker::Reader& rd)
{
	uint32_t value = 0;
	uint8_t c;
	do
	{
		c = rd.ReadUnsigned(1);
		value = (value << 7) | (c & 0x7F);
	} while((c & 0x80) != 0);
	return value;
}

size_t PEFFormat::PatternInitialization::GetValueSize(uint32_t value, size_t size_hint)
{
	size_t ptr = 0;
	while(value != 0 || ptr < size_hint)
	{
		ptr++;
		value >>= 7;
	}
	return ptr;
}

void PEFFormat::PatternInitialization::WriteValue(Linker::Writer& wr, uint32_t value, size_t size_hint)
{
	uint8_t data[5];
	size_t ptr = 0;
	size_hint = GetValueSize(value, size_hint);
	while(ptr < size_hint)
	{
		data[ptr++] = value & 0x7F;
		value >>= 7;
	}
	while(ptr > 0)
	{
		if(--ptr != 0)
		{
			wr.WriteWord(1, 0x80 | data[ptr]);
		}
		else
		{
			wr.WriteWord(1, data[ptr]);
		}
	}
}

void PEFFormat::PatternInitialization::ReadFile(Linker::Reader& rd)
{
	file_offset = rd.Tell();
	uint8_t byte = rd.ReadUnsigned(1);
	opcode = opcode_type(byte >> 5);
	uint32_t param1 = opcode & 0x1F;
	if(param1 == 0)
	{
		param1 = ReadValue(rd);
	}
	uint32_t param2, param3;
	switch(opcode)
	{
	case Zero:
		count = param1;
		break;
	case BlockCopy:
		rd.ReadData(param1, common_data);
		break;
	case RepeatedBlock:
		count = ReadValue(rd) + 1;
		rd.ReadData(param1, common_data);
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		param2 = ReadValue(rd);
		param3 = ReadValue(rd);
		rd.ReadData(param1, common_data);
		while(param3-- > 0)
		{
			std::vector<uint8_t> data;
			rd.ReadData(param2, data);
			custom_data.push_back(data);
		}
		break;
	case InterleaveRepeatBlockWithZero:
		count = param1;
		param2 = ReadValue(rd);
		param3 = ReadValue(rd);
		while(param3-- > 0)
		{
			std::vector<uint8_t> data;
			rd.ReadData(param2, data);
			custom_data.push_back(data);
		}
		break;
	default:
		// TODO: undefined
		break;
	}
}

void PEFFormat::PatternInitialization::WriteFile(Linker::Writer& wr) const
{
	uint32_t param = 0;
	switch(opcode)
	{
	case Zero:
		param = count;
		break;
	case BlockCopy:
		param = common_data.size();
		break;
	case RepeatedBlock:
		param = count - 1;
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		param = common_data.size();
		break;
	case InterleaveRepeatBlockWithZero:
		param = count;
		break;
	default:
		// TODO: undefined
		break;
	}

	if(param <= 31)
	{
		wr.WriteWord(1, (opcode << 5) | param);
	}
	else
	{
		wr.WriteWord(1, opcode << 5);
		WriteValue(wr, param);
	}

	switch(opcode)
	{
	case Zero:
		break;
	case BlockCopy:
		wr.WriteData(common_data);
		break;
	case RepeatedBlock:
		WriteValue(wr, count - 1);
		wr.WriteData(common_data);
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		WriteValue(wr, custom_data[0].size()); // TODO: what if there are no custom data? what if they are not the same size?
		WriteValue(wr, custom_data.size());
		wr.WriteData(common_data);
		for(auto& data : custom_data)
		{
			wr.WriteData(data);
		}
		break;
	case InterleaveRepeatBlockWithZero:
		WriteValue(wr, custom_data[0].size()); // TODO: what if there are no custom data? what if they are not the same size?
		WriteValue(wr, custom_data.size());
		for(auto& data : custom_data)
		{
			wr.WriteData(data);
		}
		break;
	default:
		// TODO: undefined
		break;
	}
}

offset_t PEFFormat::PatternInitialization::CodeSize() const
{
	offset_t size = 1;
	uint32_t param = 0;
	switch(opcode)
	{
	case Zero:
		param = count;
		break;
	case BlockCopy:
		param = common_data.size();
		break;
	case RepeatedBlock:
		param = count - 1;
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		param = common_data.size();
		break;
	case InterleaveRepeatBlockWithZero:
		param = count;
		break;
	default:
		// TODO: undefined
		break;
	}

	if(param > 31)
	{
		size += GetValueSize(param);
	}

	switch(opcode)
	{
	case Zero:
		break;
	case BlockCopy:
		size += common_data.size();
		break;
	case RepeatedBlock:
		size += GetValueSize(count - 1) + common_data.size();
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		size += GetValueSize(custom_data[0].size()); // TODO: what if there are no custom data? what if they are not the same size?
		size += GetValueSize(custom_data.size());
		size += custom_data.size();
		for(auto& data : custom_data)
		{
			size += data.size();
		}
		break;
	case InterleaveRepeatBlockWithZero:
		size += GetValueSize(custom_data[0].size()); // TODO: what if there are no custom data? what if they are not the same size?
		size += GetValueSize(custom_data.size());
		for(auto& data : custom_data)
		{
			size += data.size();
		}
		break;
	default:
		// TODO: undefined
		break;
	}

	return size;
}

offset_t PEFFormat::PatternInitialization::DataSize() const
{
	offset_t size = 0;

	switch(opcode)
	{
	case Zero:
		size += count;
		break;
	case BlockCopy:
		size += common_data.size();
		break;
	case RepeatedBlock:
		size += common_data.size() * count;
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		size += common_data.size() * (custom_data.size() + 1);
		for(auto& data : custom_data)
		{
			size += data.size();
		}
		break;
	case InterleaveRepeatBlockWithZero:
		size += count * (custom_data.size() + 1);
		for(auto& data : custom_data)
		{
			size += data.size();
		}
		break;
	default:
		// TODO: undefined
		break;
	}

	return size;
}

void PEFFormat::PatternInitialization::ExpandData(Linker::Buffer& buffer) const
{
	switch(opcode)
	{
	case Zero:
		buffer.Resize(buffer.ImageSize() + count);
		break;
	case BlockCopy:
		buffer.Append(const_cast<std::vector<uint8_t>&>(common_data));
		break;
	case RepeatedBlock:
		for(uint32_t index = 0; index < count; index ++)
		{
			buffer.Append(const_cast<std::vector<uint8_t>&>(common_data));
		}
		break;
	case InterleaveRepeatBlockWithBlockCopy:
		buffer.Append(const_cast<std::vector<uint8_t>&>(common_data));
		for(auto& data : custom_data)
		{
			buffer.Append(const_cast<std::vector<uint8_t>&>(data));
			buffer.Append(const_cast<std::vector<uint8_t>&>(common_data));
		}
		break;
	case InterleaveRepeatBlockWithZero:
		buffer.Resize(buffer.ImageSize() + count);
		for(auto& data : custom_data)
		{
			buffer.Append(const_cast<std::vector<uint8_t>&>(data));
			buffer.Resize(buffer.ImageSize() + count);
		}
		break;
	default:
		// TODO: undefined
		break;
	}
}

void PEFFormat::Section::ReadHeader(Linker::Reader& rd)
{
	name_offset = rd.ReadUnsigned(4);
	default_address = rd.ReadUnsigned(4);
	total_size = rd.ReadUnsigned(4);
	unpacked_size = rd.ReadUnsigned(4);
	packed_size = rd.ReadUnsigned(4);
	container_offset = rd.ReadUnsigned(4);
	section_kind = section_type(rd.ReadUnsigned(1));
	share_kind = share_type(rd.ReadUnsigned(1));
	alignment = rd.ReadUnsigned(1);
	reserved = rd.ReadUnsigned(1);
}

void PEFFormat::Section::ReadFile(PEFFormat& pef_format, Linker::Reader& rd)
{
	if(name_offset != no_name_offset)
	{
		rd.Seek(pef_format.GetSectionNameTableOffset());
		name = rd.ReadASCII('\0');
	}
	else
	{
		name = "";
	}

	switch(section_kind)
	{
	case Code:
	case UnpackedData:
	case Constant:
	case ExecutableData:
		rd.Seek(container_offset);
		image = Linker::Buffer::ReadFromFile(rd, packed_size);
		break;
	case PatternInitializedData:
		// TODO: untested
		rd.Seek(container_offset);
		patterns.clear();
		while(rd.Tell() < container_offset + packed_size)
		{
			patterns.push_back(PatternInitialization());
			patterns.back().ReadFile(rd);
		}

		{
			auto buffer = std::make_shared<Linker::Buffer>();
			image = buffer;
			for(auto& pattern : patterns)
			{
				pattern.ExpandData(*buffer);
			}
		}
		break;
	case Loader:
		// TODO
		break;
	default:
		break;
	}
}

void PEFFormat::Section::CalculateValues(PEFFormat& pef_format)
{
	switch(section_kind)
	{
	case Code:
	case Constant:
		total_size = unpacked_size = packed_size = image->ImageSize();
		break;
	case UnpackedData:
	case ExecutableData:
		unpacked_size = packed_size = image->ImageSize();
		if(total_size < unpacked_size)
		{
			total_size = unpacked_size;
		}
		break;
	case PatternInitializedData:
		// TODO: untested
		packed_size = 0;
		unpacked_size = 0;
		for(auto& pattern : patterns)
		{
			packed_size += pattern.CodeSize();
			unpacked_size += pattern.DataSize();
		}
		if(total_size < unpacked_size)
		{
			total_size = unpacked_size;
		}
		break;
	case Loader:
		total_size = unpacked_size = packed_size = 0;
		break;
	default:
		break;
	}

	if(!name.empty())
	{
		name_offset = pef_format.section_name_table_end;
		pef_format.section_name_table.push_back(name);
		pef_format.section_name_table_end += name.size() + 1;
	}

	// TODO: container_offset
}

void PEFFormat::Section::WriteHeader(Linker::Writer& wr) const
{
	wr.WriteWord(4, name_offset);
	wr.WriteWord(4, default_address);
	wr.WriteWord(4, total_size);
	wr.WriteWord(4, unpacked_size);
	wr.WriteWord(4, packed_size);
	wr.WriteWord(4, container_offset);
	wr.WriteWord(1, section_kind);
	wr.WriteWord(1, share_kind);
	wr.WriteWord(1, alignment);
	wr.WriteWord(1, reserved);
}

void PEFFormat::Section::WriteFile(const PEFFormat& pef_format, Linker::Writer& wr) const
{
	switch(section_kind)
	{
	case Code:
	case UnpackedData:
	case Constant:
	case ExecutableData:
		wr.Seek(container_offset);
		image->WriteFile(wr);
		break;
	case PatternInitializedData:
		// TODO: untested
		wr.Seek(container_offset);
		for(auto& pattern : patterns)
		{
			pattern.WriteFile(wr);
		}
		break;
	case Loader:
		// TODO
		break;
	default:
		break;
	}

	// TODO: write names
}

void PEFFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::BigEndian;
	rd.Seek(8);
	architecture = cpu_type(rd.ReadUnsigned(4));
	format_version = rd.ReadUnsigned(4);
	date_time_stamp = rd.ReadUnsigned(4);
	old_def_version = rd.ReadUnsigned(4);
	old_imp_version = rd.ReadUnsigned(4);
	current_version = rd.ReadUnsigned(4);
	uint16_t section_count = rd.ReadUnsigned(2);
	inst_section_count = rd.ReadUnsigned(2);
	reserved = rd.ReadUnsigned(4);

	for(uint16_t section_index = 0; section_index < section_count; section_index++)
	{
		auto section = std::make_shared<Section>();
		section->ReadHeader(rd);
		sections.push_back(section);
	}

	section_name_table_end = GetSectionNameTableOffset();
	for(auto section : sections)
	{
		section->ReadFile(*this, rd);
	}
}

offset_t PEFFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::BigEndian;
	wr.WriteData("Joy!peff");
	wr.WriteWord(4, architecture);
	wr.WriteWord(4, format_version);
	wr.WriteWord(4, date_time_stamp);
	wr.WriteWord(4, old_def_version);
	wr.WriteWord(4, old_imp_version);
	wr.WriteWord(4, current_version);
	wr.WriteWord(2, sections.size());
	wr.WriteWord(2, inst_section_count);
	wr.WriteWord(4, reserved);

	for(auto section : sections)
	{
		section->WriteHeader(wr);
	}

	// TODO: this only works if CalculateValues was executed, and it will not reproduce the table perfectly
	for(auto name : section_name_table)
	{
		wr.WriteData(name);
		wr.WriteWord(1, 0);
	}

	for(auto section : sections)
	{
		section->WriteFile(*this, wr);
	}

	return offset_t(-1);
}

void PEFFormat::CalculateValues()
{
	format_version = 1;
	reserved = 0;

	inst_section_count = 0;
	// move all instantiated sections before all uninstantiated sections
	for(uint16_t section_index = 0; section_index < sections.size(); section_index++)
	{
		if(sections[section_index]->IsInstantiated())
		{
			if(inst_section_count + 1 < section_index)
			{
				auto section = sections[section_index];
				sections.erase(sections.begin() + section_index);
				sections.insert(sections.begin() + inst_section_count, section);
			}

			inst_section_count ++;
		}
	}

	for(auto section : sections)
	{
		section->CalculateValues(*this);
	}
}

void PEFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PEF format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	Dumper::Region header_region("Container header", file_offset, ContainerHeaderSize, 8);
	// convert architecture word back to ASCII string
	union
	{
		uint32_t value;
		char string[4];
	} u;
	u.value = FromBigEndian32(architecture); // TODO: should be ToBigEndian32
	header_region.AddField("Architecture", Dumper::StringDisplay::Make("'"), std::string(u.string, 4));
	header_region.AddField("Format version", Dumper::DecDisplay::Make(), offset_t(format_version));
	header_region.AddField("Date time stamp", Dumper::DecDisplay::Make(), offset_t(date_time_stamp)); // TODO: format
	header_region.AddField("Old definition version", Dumper::DecDisplay::Make(), offset_t(old_def_version));
	header_region.AddField("Old implementation version", Dumper::DecDisplay::Make(), offset_t(old_imp_version));
	header_region.AddField("Current version", Dumper::DecDisplay::Make(), offset_t(current_version));
	header_region.AddField("Section count", Dumper::DecDisplay::Make(), offset_t(sections.size()));
	header_region.AddField("Instantiated section count", Dumper::DecDisplay::Make(), offset_t(inst_section_count));
	header_region.AddOptionalField("Reserved field", Dumper::HexDisplay::Make(8), offset_t(reserved));
	header_region.Display(dump);

	for(uint16_t section_number = 0; section_number < sections.size(); section_number++)
	{
		auto section = sections[section_number];
		Dumper::Block section_block("Section",
			section->section_kind != Section::PatternInitializedData ? section->container_offset : 0,
				// for pattern initialized data, image represents the unpacked data, so the file offset makes no sense
			section->image ? section->image->AsImage() : nullptr,
			section->default_address,
			8);
		section_block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(section_number + 1));
		section_block.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(section->name_offset));
		section_block.AddOptionalField("Name", Dumper::StringDisplay::Make("\""), section->name);
		section_block.AddOptionalField("Total size", Dumper::HexDisplay::Make(8), offset_t(section->total_size));
		section_block.AddOptionalField("Unpacked size", Dumper::HexDisplay::Make(8), offset_t(section->unpacked_size));
		section_block.AddOptionalField("Packed size", Dumper::HexDisplay::Make(8), offset_t(section->packed_size));
		static const std::map<offset_t, std::string> section_type =
		{
			{ Section::Code,                   "Code" },
			{ Section::UnpackedData,           "UnpackedData" },
			{ Section::PatternInitializedData, "PatternInitializedData" },
			{ Section::Constant,               "Constant" },
			{ Section::Loader,                 "Loader" },
			{ Section::Debug,                  "Debug" },
			{ Section::ExecutableData,         "ExecutableData" },
			{ Section::Exception,              "Exception" },
			{ Section::Traceback,              "Traceback" },
		};
		section_block.AddField("Section kind", Dumper::ChoiceDisplay::Make(section_type), offset_t(section->section_kind));
		static const std::map<offset_t, std::string> share_type =
		{
			{ Section::ProcessShare,   "ProcessShare" },
			{ Section::GlobalShare,    "GlobalShare" },
			{ Section::ProtectedShare, "ProtectedShare" },
		};
		section_block.AddField("Share kind", Dumper::ChoiceDisplay::Make(share_type), offset_t(section->share_kind));
		section_block.AddField("Alignment", Dumper::DecDisplay::Make(), offset_t(1 << section->alignment));
		section_block.AddOptionalField("Reserved", Dumper::HexDisplay::Make(2), offset_t(section->reserved));
		// TODO: print records for PatternInitializedData
		section_block.Display(dump);
		// TODO
	}

	// TODO
}

