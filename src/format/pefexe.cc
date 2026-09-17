
#include "pefexe.h"
#include "../linker/location.h"
#include "../linker/module.h"
#include "../linker/position.h"
#include "../linker/resolution.h"
#include "../linker/section.h"
#include "../linker/segment.h"

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
	uint32_t param1 = byte & 0x1F;
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

void PEFFormat::RelocationProcessor::Initialize()
{
	//reloc_instr_ptr = 0;
	reloc_address = 0;
	import_index = 0;
	section_c = pef_format.sections[0]->IsInstantiated() ? 0 : PEFFormat::NoSection;
	section_d = pef_format.sections[1]->IsInstantiated() ? 1 : PEFFormat::NoSection;
	//current_repeat_count = 0;
}

void PEFFormat::RelocationProcessor::Regress(uint32_t block_count)
{
	while(block_count > 0)
	{
		reloc_instr_ptr --;
		block_count -= reloc_opcodes[reloc_instr_ptr].CodeSize() >> 1;
	}
}

void PEFFormat::RelocationProcessor::GenerateRelocations()
{
	current_repeat_count = 0;
	for(reloc_instr_ptr = 0; reloc_instr_ptr < reloc_opcodes.size(); )
	{
		auto& opcode = reloc_opcodes[reloc_instr_ptr++];
		opcode.GenerateRelocations(*this);
	}
}

void PEFFormat::RelocOpcode::ReadFile(Linker::Reader& rd)
{
	offset = rd.Tell();
	uint16_t word = rd.ReadUnsigned(2);
	if((word & 0xC000) == 0x0000)
	{
		opcode = BySectDWithSkip;
		value = ((word >> 6) & 0xFF) * 4;
		repeat = word & 0x3F;
	}
	else if((word & 0xE000) == 0x4000)
	{
		switch((word >> 9) & 0xF)
		{
		case 0:
			opcode = BySectC;
			break;
		case 1:
			opcode = BySectD;
			break;
		case 2:
			opcode = TVector12;
			break;
		case 3:
			opcode = TVector8;
			break;
		case 4:
			opcode = VTable8;
			break;
		case 5:
			opcode = ImportRun;
			break;
		default:
			opcode = SmInvalid;
			value = word;
			return;
		}
		repeat = (word & 0x1FF) + 1;
	}
	else if((word & 0xE000) == 0x6000)
	{
		switch((word >> 9) & 0xF)
		{
		case 0:
			opcode = SmByImport;
			break;
		case 1:
			opcode = SmSetSectC;
			break;
		case 2:
			opcode = SmSetSectD;
			break;
		case 3:
			opcode = SmBySection;
			break;
		default:
			opcode = SmInvalid;
			value = word;
			return;
		}
		value = word & 0x1FF;
	}
	else if((word & 0xF000) == 0x8000)
	{
		opcode = IncrPosition;
		value = (word & 0x0FFF) + 1;
	}
	else if((word & 0xF000) == 0x9000)
	{
		opcode = SmRepeat;
		value = ((word >> 8) & 0xF) + 1;
		repeat = (word & 0xFF) + 1;
	}
	else if((word & 0xFC00) == 0xA000)
	{
		opcode = SetPosition;
		value = uint32_t(word & 0x03FF) << 16;
		value |= rd.ReadUnsigned(2);
	}
	else if((word & 0xFC00) == 0xA400)
	{
		opcode = LgByImport;
		value = uint32_t(word & 0x03FF) << 16;
		value |= rd.ReadUnsigned(2);
	}
	else if((word & 0xFC00) == 0xB000)
	{
		opcode = LgRepeat;
		value = ((word >> 6) & 0xF) + 1;
		repeat = uint32_t(word & 0x003F) << 16;
		repeat |= rd.ReadUnsigned(2);
	}
	else
	{
		switch(word & 0xFFC0)
		{
		case LgBySection:
		case LgSetSectC:
		case LgSetSectD:
			opcode = opcode_type(word & 0xFFC0);
			value = uint32_t(word & 0x003F) << 16;
			value |= rd.ReadUnsigned(2);
			break;
		default:
			opcode = SmInvalid; // note: this could be LgInvalid and another word could be read
			value = word;
			return;
		}
	}
}

uint32_t PEFFormat::RelocOpcode::GetWord() const
{
	switch(opcode)
	{
	case BySectDWithSkip:
		return opcode | (((value / 4) & 0xFF) << 6) | (repeat & 0x3F);
	case BySectC:
	case BySectD:
	case TVector12:
	case TVector8:
	case VTable8:
	case ImportRun:
		return opcode | ((repeat - 1) & 0x1FF);
	case SmByImport:
	case SmSetSectC:
	case SmSetSectD:
	case SmBySection:
		return opcode | (value & 0x1FF);
	case IncrPosition:
		return opcode | ((value - 1) & 0x0FFF);
	case SmRepeat:
		return opcode | (((value - 1) & 0xF) << 8) | ((repeat - 1) & 0xFF);
	case SetPosition:
	case LgByImport:
		return (uint32_t(opcode) << 16) | (value & 0x03FFFFFF);
	case LgRepeat:
		return (uint32_t(opcode) << 16) | (uint32_t((value - 1) & 0xF) << 22) | (repeat & 0x003FFFFF);
	case LgBySection:
	case LgSetSectC:
	case LgSetSectD:
		return (uint32_t(opcode) << 16) | (value & 0x003FFFFF);
	case SmInvalid:
	//case LgInvalid:
		return value;
	default:
		// TODO: error
		return 0;
	}
}

offset_t PEFFormat::RelocOpcode::CodeSize() const
{
	switch(opcode)
	{
	case BySectDWithSkip:
	case BySectC:
	case BySectD:
	case TVector12:
	case TVector8:
	case VTable8:
	case ImportRun:
	case SmByImport:
	case SmSetSectC:
	case SmSetSectD:
	case SmBySection:
	case IncrPosition:
	case SmRepeat:
	case SmInvalid:
		return 2;
	case SetPosition:
	case LgByImport:
	case LgRepeat:
	case LgBySection:
	case LgSetSectC:
	case LgSetSectD:
	//case LgInvalid:
		return 4;
	default:
		// TODO: error
		return 0;
	}
}

void PEFFormat::RelocOpcode::GenerateRelocations(RelocationProcessor& processor) const
{
	switch(opcode)
	{
	case BySectDWithSkip:
		processor.Advance(value);
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionD();
		}
		break;
	case BySectC:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionC();
		}
		break;
	case BySectD:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionD();
		}
		break;
	case TVector12:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionC();
			processor.AddSectionD();
			processor.Advance(4);
		}
		break;
	case TVector8:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionC();
			processor.AddSectionD();
		}
		break;
	case VTable8:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSectionD();
			processor.Advance(4);
		}
		break;
	case ImportRun:
		for(uint32_t index = 0; index < repeat; index++)
		{
			processor.AddSymbol();
		}
		break;
	case SmByImport:
	case LgByImport:
		processor.import_index = value;
		processor.AddSymbol();
		break;
	case SmSetSectC:
	case LgSetSectC:
		processor.section_c = value;
		break;
	case SmSetSectD:
	case LgSetSectD:
		processor.section_d = value;
		break;
	case SmBySection:
	case LgBySection:
		processor.AddSection(value);
		break;
	case IncrPosition:
		processor.reloc_address += value;
		break;
	case SmRepeat:
	case LgRepeat:
		processor.Repeat(value, repeat);
		break;
	case SetPosition:
		processor.reloc_address = value;
		break;
	case SmInvalid:
	//case LgInvalid:
		// TODO: error
		break;
	default:
		// TODO: error
		break;
	}
}

void PEFFormat::RelocOpcode::Dump(Dumper::Dumper& dump, const PEFFormat& pef_format, uint32_t opcode_index, int display_options) const
{
	static const std::map<offset_t, std::string> opcode_type =
	{
		{ RelocOpcode::SmInvalid,       "Invalid" },
		//{ RelocOpcode::LgInvalid,       "Invalid" },
		{ RelocOpcode::BySectDWithSkip, "BySectDWithSkip" },
		{ RelocOpcode::BySectC,         "BySectC" },
		{ RelocOpcode::BySectD,         "BySectD" },
		{ RelocOpcode::TVector12,       "TVector12" },
		{ RelocOpcode::TVector8,        "TVector8" },
		{ RelocOpcode::VTable8,         "VTable8" },
		{ RelocOpcode::ImportRun,       "ImportRun" },
		{ RelocOpcode::SmByImport,      "SmByImport" },
		{ RelocOpcode::SmSetSectC,      "SmSetSectC" },
		{ RelocOpcode::SmSetSectD,      "SmSetSectD" },
		{ RelocOpcode::SmBySection,     "SmBySection" },
		{ RelocOpcode::IncrPosition,    "IncrPosition" },
		{ RelocOpcode::SmRepeat,        "SmRepeat" },
		{ RelocOpcode::SetPosition,     "SetPosition" },
		{ RelocOpcode::LgByImport,      "LgByImport" },
		{ RelocOpcode::LgRepeat,        "LgRepeat" },
		{ RelocOpcode::LgBySection,     "LgBySection" },
		{ RelocOpcode::LgSetSectC,      "LgSetSectC" },
		{ RelocOpcode::LgSetSectD,      "LgSetSectD" },
	};
	Dumper::Entry reloc_entry("Relocation opcode", opcode_index + 1);
	reloc_entry.AddField("File offset", Dumper::HexDisplay::Make(8), offset_t(offset));
	reloc_entry.AddField("Opcode", Dumper::ChoiceDisplay::Make(opcode_type), offset_t(opcode));
	reloc_entry.AddField("Width", Dumper::DecDisplay::Make(), offset_t(CodeSize()));
	reloc_entry.AddField("Record", Dumper::HexDisplay::Make(2 * CodeSize()), offset_t(GetWord()));
	switch(opcode)
	{
	case RelocOpcode::BySectDWithSkip:
		reloc_entry.AddField("Skip", Dumper::HexDisplay::Make(8), offset_t(value));
		reloc_entry.AddField("Count", Dumper::DecDisplay::Make(), offset_t(repeat));
		break;
	case RelocOpcode::BySectC:
	case RelocOpcode::BySectD:
	case RelocOpcode::TVector12:
	case RelocOpcode::TVector8:
	case RelocOpcode::VTable8:
	case RelocOpcode::ImportRun:
		reloc_entry.AddField("Count", Dumper::DecDisplay::Make(), offset_t(repeat));
		break;
	case RelocOpcode::SmByImport:
	case RelocOpcode::SmSetSectC:
	case RelocOpcode::SmSetSectD:
	case RelocOpcode::SmBySection:
		reloc_entry.AddField("Index", Dumper::HexDisplay::Make(4), offset_t(value));
		break;
	case RelocOpcode::IncrPosition:
		reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(4), offset_t(value));
		break;
	case RelocOpcode::SmRepeat:
	case RelocOpcode::LgRepeat:
		reloc_entry.AddField("Block count", Dumper::DecDisplay::Make(), offset_t(value));
		reloc_entry.AddField("Repeat count", Dumper::DecDisplay::Make(), offset_t(repeat));
		break;
	case RelocOpcode::SetPosition:
		reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(value));
		break;
	case RelocOpcode::LgByImport:
	case RelocOpcode::LgBySection:
	case RelocOpcode::LgSetSectC:
	case RelocOpcode::LgSetSectD:
		reloc_entry.AddField("Index", Dumper::HexDisplay::Make(8), offset_t(value));
		break;
	default:
		break;
	}
	reloc_entry.Display(dump, Dumper::Relocation | Dumper::Control | display_options);
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
	if(name_offset != NoNameOffset)
	{
		rd.Seek(pef_format.GetSectionNameTableOffset());
		name = rd.ReadASCIIZ();
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
		rd.Seek(container_offset);
		{
			Linker::Reader section_reader = rd.CreateWindow(container_offset, packed_size);
			section_reader.on_overflow = Linker::Reader::ReportOnOverflow;
			patterns.clear();
			try
			{
				while(section_reader.Tell() < packed_size)
				{
					patterns.push_back(PatternInitialization());
					patterns.back().ReadFile(section_reader);
				}
			}
			catch(Linker::ReadOverflow)
			{
				Linker::Error << "Error: pattern initialized data exceeded section limit" << std::endl;
			}
		}

		{
			auto buffer = std::make_shared<Linker::Buffer>();
			image = buffer;
			for(auto& pattern : patterns)
			{
				pattern.ExpandData(*buffer);
			}
			buffer->Resize(unpacked_size);
		}
		break;
	case Loader:
		rd.Seek(container_offset);
		pef_format.loader_section_offset = container_offset;
		{
			Linker::Reader section_reader = rd.CreateWindow(container_offset, unpacked_size);
			pef_format.ReadLoaderSection(section_reader);
		}
		break;
	default:
		break;
	}
}

size_t PEFFormat::Section::GetImageSize(PEFFormat& pef_format)
{
	if(section_kind == Loader)
	{
		return pef_format.GetLoaderSectionSize();
	}
	else
	{
		return packed_size;
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
		// fill in later
		total_size = unpacked_size = packed_size = 0;
		break;
	default:
		break;
	}

	if(auto section = std::dynamic_pointer_cast<Linker::Segment>(image))
	{
		total_size = section->TotalSize();
	}

	if(!name.empty())
	{
		name_offset = pef_format.section_name_table_end;
		pef_format.section_name_table.push_back(name);
		pef_format.section_name_table_end += name.size() + 1;
	}
	else
	{
		name_offset = NoNameOffset;
	}
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
		wr.Seek(container_offset);
		pef_format.WriteLoaderSection(wr);
		break;
	default:
		break;
	}
}

void PEFFormat::Reference::SetPosition(PEFFormat& pef_format, const Linker::Position& position)
{
	offset = position.address;
	if(position.segment != nullptr)
	{
		section_pointer = pef_format.segment_to_section_map[position.segment];
		offset -= position.segment->base_address;
	}
	else
	{
		section_pointer.reset();
	}
}

std::string PEFFormat::Name::LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd)
{
	rd.Seek(pef_format.loader_strings_offset + name_offset);
	return name = rd.ReadASCIIZ();
}

std::string PEFFormat::Name::LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd, uint16_t length)
{
	rd.Seek(pef_format.loader_strings_offset + name_offset);
	return name = rd.ReadData(length);
}

void PEFFormat::Name::StoreNameString(PEFFormat& pef_format)
{
	name_offset = pef_format.loader_string_table_size;
	pef_format.loader_string_table.push_back(name + std::string("\0", 1));
	pef_format.loader_string_table_size += name.size() + 1;
}

void PEFFormat::Name::StoreNameStringNoNull(PEFFormat& pef_format)
{
	name_offset = pef_format.loader_string_table_size;
	pef_format.loader_string_table.push_back(name);
	pef_format.loader_string_table_size += name.size();
}

std::string PEFFormat::ExportedSymbol::LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd)
{
	return LoadNameString(pef_format, rd, symbol_length);
}

uint32_t PEFFormat::ComputeHashWord(std::string name)
{
	// based on the documentation in Mac OS Runtime Architectures
	int32_t hash_value = 0;
	for(auto c : name)
	{
		hash_value = (hash_value << 1) - (hash_value >> 16);
		hash_value ^= c & 0xFF;
	}
	hash_value ^= hash_value >> 16;
	return (name.size() << 16) | (hash_value & 0xFFFF);
}

uint32_t PEFFormat::HashTableIndex(uint32_t hash_word, uint32_t export_hash_table_power)
{
	// based on the documentation in Mac OS Runtime Architectures
	hash_word ^= hash_word >> export_hash_table_power;
	return hash_word & ((1 << export_hash_table_power) - 1);
}

uint8_t PEFFormat::ComputeHashTableExponent(uint32_t hash_table_size)
{
	// based on the recommendation in Mac OS Runtime Architectures
	static constexpr int32_t ExponentLimit = 16;
	static constexpr int32_t AverageChainLimit = 10;

	uint8_t export_hash_table_power;
	for(export_hash_table_power = 0; export_hash_table_power < ExponentLimit; export_hash_table_power ++)
	{
		if((hash_table_size >> export_hash_table_power) < AverageChainLimit)
			break;
	}
	return export_hash_table_power;
}

bool PEFFormat::FormatSupportsLibraries() const
{
	return true;
}

bool PEFFormat::FormatSupportsResources() const
{
	return true;
}

void PEFFormat::ReadLoaderSection(Linker::Reader& rd)
{
	rd.on_overflow = Linker::Reader::ReportOnOverflow;

	try
	{
		//// header

		main_symbol.section = rd.ReadUnsigned(4);
		main_symbol.offset = rd.ReadUnsigned(4);

		init_symbol.section = rd.ReadUnsigned(4);
		init_symbol.offset = rd.ReadUnsigned(4);

		term_symbol.section = rd.ReadUnsigned(4);
		term_symbol.offset = rd.ReadUnsigned(4);

		uint32_t imported_library_count = rd.ReadUnsigned(4);
		uint32_t total_imported_symbol_count = rd.ReadUnsigned(4);

		uint32_t reloc_section_count = rd.ReadUnsigned(4);
		reloc_instr_offset = rd.ReadUnsigned(4);

		loader_strings_offset = rd.ReadUnsigned(4);

		export_hash_offset = rd.ReadUnsigned(4);
		uint32_t export_hash_table_power = rd.ReadUnsigned(4);
		uint32_t exported_symbol_count = rd.ReadUnsigned(4);

		//// imported library descriptions

		for(uint32_t imported_library_index = 0; imported_library_index < imported_library_count; imported_library_index++)
		{
			auto library = std::make_shared<ImportedLibrary>();
			imported_libraries.push_back(library);
			library->name_offset = rd.ReadUnsigned(4);
			library->old_imp_version = rd.ReadUnsigned(4);
			library->current_version = rd.ReadUnsigned(4);
			library->imported_symbol_count = rd.ReadUnsigned(4);
			library->first_imported_symbol = rd.ReadUnsigned(4);
			library->options = rd.ReadUnsigned(1);
			library->reserved_a = rd.ReadUnsigned(1);
			library->reserved_b = rd.ReadUnsigned(2);
		}

		//// imported symbol tables

		for(uint32_t imported_symbol_index = 0; imported_symbol_index < total_imported_symbol_count; imported_symbol_index++)
		{
			auto symbol = std::make_shared<ImportedSymbol>();
			imported_symbols.push_back(symbol);
			uint32_t value = rd.ReadUnsigned(4);
			symbol->symbol_class = symbol_class_type((value >> 24) & 0x0F);
			symbol->flags = (value >> 24) & 0xF0;
			symbol->name_offset = value & 0x00FFFFFF;
		}

		// now the library specific symbols can be loaded
		for(auto library : imported_libraries)
		{
			library->imported_symbols.insert(
				library->imported_symbols.begin(),
				imported_symbols.begin() + library->first_imported_symbol,
				imported_symbols.begin() + library->first_imported_symbol + library->imported_symbol_count);

			for(auto symbol : library->imported_symbols)
			{
				symbol->library = library;
			}
		}

		//// relocation headers

		for(uint32_t reloc_section_index = 0; reloc_section_index < reloc_section_count; reloc_section_index++)
		{
			uint16_t section_index = rd.ReadUnsigned(2);
			reloc_section_indexes.push_back(section_index);
			auto section = sections[section_index];
			section->contains_relocations = true;
			section->reserved_a = rd.ReadUnsigned(2);
			section->reloc_instr_size = rd.ReadUnsigned(4) * 2;
			section->first_reloc_offset = rd.ReadUnsigned(4) * 2;
		}

		//// relocation area

		// this is where consecutive reading stops

		for(auto section : sections)
		{
			if(section->contains_relocations)
			{
				rd.Seek(reloc_instr_offset + section->first_reloc_offset);
				while(rd.Tell() < reloc_instr_offset + section->first_reloc_offset + section->reloc_instr_size)
				{
					RelocOpcode opcode;
					opcode.ReadFile(rd);
					section->reloc_opcodes.push_back(opcode);
				}

				RelocationProcessor processor(*this, section->reloc_opcodes, section->relocations);
				processor.GenerateRelocations();

				for(auto& relocation : section->relocations)
				{
					if(relocation.second.type == Relocation::Symbol)
					{
						relocation.second.symbol = imported_symbols[relocation.second.number];
					}
				}
			}
		}

		try
		{
			// read all relocations

			rd.Seek(reloc_instr_offset);
			relocs_area.clear();
			while(rd.Tell() < loader_strings_offset)
			{
				RelocOpcode opcode;
				opcode.ReadFile(rd);
				relocs_area.push_back(opcode);
			}
		}
		catch(Linker::ReadOverflow)
		{
			Linker::Error << "Error: relocation data exceeded loader section limit" << std::endl;
		}

		//// loader string table

		for(auto library : imported_libraries)
		{
			library->LoadNameString(*this, rd);
		}

		for(auto symbol : imported_symbols)
		{
			symbol->LoadNameString(*this, rd);
		}

		// read full table after reading the exported symbol table (since exported symbols are not necessarily zero terminated)

		//// export hash table

		rd.Seek(export_hash_offset);
		hash_table.resize(1 << export_hash_table_power);
		for(auto& hash_table_entry : hash_table)
		{
			uint32_t value = rd.ReadUnsigned(4);
			hash_table_entry.chain_count = value >> 18;
			hash_table_entry.first_index = value & 0x0003FFFF;
		}

		//// export key table

		for(uint32_t export_index = 0; export_index < exported_symbol_count; export_index ++)
		{
			auto symbol = std::make_shared<ExportedSymbol>();
			exported_symbols.push_back(symbol);
			symbol->symbol_length = rd.ReadUnsigned(2);
			symbol->hash_value = rd.ReadUnsigned(2);
		}

		//// exported symbol table

		std::set<uint32_t> string_terminations;

		for(auto& symbol : exported_symbols)
		{
			symbol->name_offset = rd.ReadUnsigned(4);
			symbol->symbol_class = symbol_class_type(symbol->name_offset >> 24);
			symbol->name_offset &= 0x00FFFFFF;
			symbol->offset = rd.ReadUnsigned(4);
			symbol->section = rd.ReadSigned(2); // sign extend to 32-bit

			// since exported strings are not (necessarily) null terminated, we need to record where terminations occur
			// in order to be able to parse the full string table
			string_terminations.insert(symbol->name_offset + symbol->symbol_length);
		}

		for(auto& symbol : exported_symbols)
		{
			symbol->LoadNameString(*this, rd);
		}

		try
		{
			// read full string table
			rd.Seek(loader_strings_offset);
			loader_string_table.clear();
			loader_string_table_size = 0;
			while(rd.Tell() < export_hash_offset)
			{
				// check where the next exported symbol termination occurs
				auto termination = std::upper_bound(string_terminations.begin(), string_terminations.end(), rd.Tell() - loader_strings_offset);
				offset_t maximum;
				if(termination == string_terminations.end())
				{
					maximum = export_hash_offset - rd.Tell();
				}
				else
				{
					maximum = *termination - (rd.Tell() - loader_strings_offset);
				}
				std::string name = rd.ReadASCIIZ(maximum);
				if(name.size() < maximum)
				{
					// zero terminated
					name += std::string("\0", 1);
				}
				loader_string_table.push_back(name);
				loader_string_table_size += maximum;
			}
		}
		catch(Linker::ReadOverflow)
		{
			Linker::Error << "Error: loader string table exceeded loader section limit" << std::endl;
		}
	}
	catch(Linker::ReadOverflow)
	{
		Linker::Error << "Error: loader data exceeded section limit" << std::endl;
		return;
	}
}

void PEFFormat::WriteLoaderSection(Linker::Writer& wr) const
{
	//// header

	wr.WriteWord(4, main_symbol.section);
	wr.WriteWord(4, main_symbol.offset);

	wr.WriteWord(4, init_symbol.section);
	wr.WriteWord(4, init_symbol.offset);

	wr.WriteWord(4, term_symbol.section);
	wr.WriteWord(4, term_symbol.offset);

	wr.WriteWord(4, imported_libraries.size());
	wr.WriteWord(4, imported_symbols.size());

	wr.WriteWord(4, reloc_section_indexes.size());
	wr.WriteWord(4, reloc_instr_offset);

	wr.WriteWord(4, loader_strings_offset);

	wr.WriteWord(4, export_hash_offset);

	uint32_t export_hash_table_size = hash_table.size();
	uint32_t export_hash_table_power = 0;
	while(export_hash_table_size > 1)
	{
		export_hash_table_size >>= 1;
		export_hash_table_power ++;
	}
	wr.WriteWord(4, export_hash_table_power);
	wr.WriteWord(4, exported_symbols.size());

	//// imported library descriptions

	for(auto library : imported_libraries)
	{
		wr.WriteWord(4, library->name_offset);
		wr.WriteWord(4, library->old_imp_version);
		wr.WriteWord(4, library->current_version);
		wr.WriteWord(4, library->imported_symbol_count);
		wr.WriteWord(4, library->first_imported_symbol);
		wr.WriteWord(1, library->options);
		wr.WriteWord(1, library->reserved_a);
		wr.WriteWord(2, library->reserved_b);
	}

	//// imported symbol tables

	for(auto symbol : imported_symbols)
	{
		wr.WriteWord(4, (symbol->name_offset & 0x00FFFFFF) | ((symbol->symbol_class & 0x0F) << 24) | ((symbol->flags & 0xF0) << 24));
	}

	//// relocation headers

	for(auto section_index : reloc_section_indexes)
	{
		auto section = sections[section_index];
		wr.WriteWord(2, section_index);
		wr.WriteWord(2, section->reserved_a);
		wr.WriteWord(4, section->reloc_instr_size / 2);
		wr.WriteWord(4, section->first_reloc_offset / 2);
	}

	//// relocation area

	wr.Seek(loader_section_offset + reloc_instr_offset);
	for(auto opcode : relocs_area)
	{
		opcode.WriteFile(wr);
	}

	//// loader string table

	wr.Seek(loader_section_offset + loader_strings_offset);
	for(auto string : loader_string_table)
	{
		wr.WriteData(string);
	}

	//// export hash table

	wr.Seek(loader_section_offset + export_hash_offset);
	for(auto& hash_table_entry : hash_table)
	{
		wr.WriteWord(4, (uint32_t(hash_table_entry.chain_count) << 18) | (hash_table_entry.first_index & 0x0003FFFF));
	}

	//// export key table

	for(auto& symbol : exported_symbols)
	{
		wr.WriteWord(2, symbol->symbol_length);
		wr.WriteWord(2, symbol->hash_value);
	}

	//// exported symbol table

	for(auto& symbol : exported_symbols)
	{
		wr.WriteWord(4, (uint32_t(symbol->symbol_class) << 24) | (symbol->name_offset & 0x00FFFFFF));
		wr.WriteWord(4, symbol->offset);
		wr.WriteWord(2, symbol->section & 0xFFFF);
	}
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

	section_name_table_end = GetSectionNameTableOffset();
	for(uint16_t section_index = 0; section_index < section_count; section_index++)
	{
		auto section = std::make_shared<Section>();
		section->ReadHeader(rd);
		sections.push_back(section);
		if(section_index == 0 || section_name_table_end > section->container_offset)
		{
			section_name_table_end = section->container_offset;
		}
	}

	// TODO: untested
	while(rd.Tell() < section_name_table_end)
	{
		section_name_table.push_back(rd.ReadASCIIZ());
	}

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

	section_name_table_end = GetSectionNameTableOffset();
	section_name_table.clear();
	for(auto section : sections)
	{
		section->CalculateValues(*this);
	}
	for(uint32_t section_index = 0; section_index < sections.size(); section_index ++)
	{
		sections[section_index]->section_number = section_index;
	}

	// rebuild symbol table using the symbols stored in the internal library structures
	imported_symbols.clear();
	for(auto library : imported_libraries)
	{
		library->first_imported_symbol = imported_symbols.size();
		library->imported_symbol_count = library->imported_symbols.size();
		imported_symbols.insert(
			imported_symbols.end(),
			library->imported_symbols.begin(),
			library->imported_symbols.end());
	}
	for(uint32_t symbol_index = 0; symbol_index < imported_symbols.size(); symbol_index ++)
	{
		imported_symbols[symbol_index]->symbol_number = symbol_index;
	}

	// collect relocation containing section indexes
	reloc_section_indexes.clear();
	for(uint32_t section_index = 0; section_index < sections.size(); section_index ++)
	{
		auto section = sections[section_index];
		if(!section->relocations.empty())
		{
			section->contains_relocations = true;
		}
		if(section->contains_relocations)
		{
			reloc_section_indexes.push_back(section_index);
		}
	}

	if(reloc_instr_offset < GetMinimumRelocInstrOffset())
	{
		reloc_instr_offset = GetMinimumRelocInstrOffset();
	}
	relocs_area.clear();
	relocs_area_size = 0;

	for(uint32_t section_index = 0; section_index < sections.size(); section_index ++)
	{
		auto section = sections[section_index];
		if(section->contains_relocations)
		{
			if(section->reloc_opcodes.empty())
			{
				// fetch index for each relocation
				for(auto& relocation : section->relocations)
				{
					if(!relocation.second.compiled)
					{
						switch(relocation.second.type)
						{
						case Relocation::Section:
							relocation.second.number = relocation.second.section.lock()->section_number;
							break;
						case Relocation::Symbol:
							relocation.second.number = relocation.second.symbol.lock()->symbol_number;
							break;
						}
						relocation.second.compiled = true;
					}
				}

				uint32_t csegment = 0;
				uint32_t dsegment = 1;

				// compile into relocations
				for(auto relocation : section->relocations)
				{
					if(relocation.second.type == Relocation::Section)
					{
						section->reloc_opcodes.push_back(RelocOpcode(RelocOpcode::SetPosition, relocation.first, 0));
						if(relocation.second.number == csegment)
						{
							section->reloc_opcodes.push_back(RelocOpcode(RelocOpcode::BySectC, 0, 1));
						}
						else if(relocation.second.number == dsegment)
						{
							section->reloc_opcodes.push_back(RelocOpcode(RelocOpcode::BySectD, 0, 1));
						}
						else
						{
							// TODO: generalize for more sections (not urgent)
							Linker::Error << "Error: unexpected target section to relocation, ignoring" << std::endl;
						}
					}
				}

				for(auto relocation : section->relocations)
				{
					if(relocation.second.type == Relocation::Symbol)
					{
						section->reloc_opcodes.push_back(RelocOpcode(RelocOpcode::SetPosition, relocation.first, 0));
						section->reloc_opcodes.push_back(RelocOpcode(RelocOpcode::LgByImport, relocation.second.number, 0));
					}
				}
			}
			section->first_reloc_offset = relocs_area_size;
			for(auto opcode : section->reloc_opcodes)
			{
				relocs_area.push_back(opcode);
				relocs_area_size += opcode.CodeSize();
			}
			section->reloc_instr_size = relocs_area_size - section->first_reloc_offset;
		}
	}

	// collect imported and exported symbol names
	if(loader_strings_offset < reloc_instr_offset + GetRelocationAreaSize())
	{
		loader_strings_offset = reloc_instr_offset + GetRelocationAreaSize();
	}
	for(auto library : imported_libraries)
	{
		library->StoreNameString(*this);
	}
	for(auto symbol : imported_symbols)
	{
		symbol->StoreNameString(*this);
	}
	for(auto symbol : exported_symbols)
	{
		symbol->StoreNameStringNoNull(*this);

		// attach a terminating null string so that the symbol name is null terminated anyway
		// we could have done this by symbol->StoreNameString that automatically attaches the null character
		// however this makes the internal state identical to that on reading, which would split the terminating null off
		Name null;
		null.StoreNameString(*this);

		// get the numeric value of the section
		symbol->StoreSectionIndex();
		symbol->symbol_length = symbol->name.size();
		// note: ProcessModule already calculates it, but CalculateValues is intended to
		// create a consistent state, so this call will be duplicated
		symbol->hash_value = ComputeHashWord(symbol->name) & 0xFFFF;
	}

	if(export_hash_offset < loader_strings_offset + GetLoaderStringAreaSize())
	{
		export_hash_offset = loader_strings_offset + GetLoaderStringAreaSize();
	}

	uint32_t export_hash_table_size = hash_table.size();
	uint32_t export_hash_table_power = 0;
	while(export_hash_table_size > 1)
	{
		export_hash_table_size >>= 1;
		export_hash_table_power ++;
	}
	hash_table.resize(1 << export_hash_table_power);

	uint32_t section_offset = section_name_table_end;
	for(auto section : sections)
	{
		section->container_offset = ::AlignTo(section_offset, section->alignment);
		if(section->section_kind == Section::Loader)
		{
			section->total_size = section->unpacked_size = section->packed_size = GetLoaderSectionSize();
			loader_section_offset = section->container_offset;
		}
		section_offset = section->container_offset + section->GetImageSize(*this);
	}

	main_symbol.StoreSectionIndex();
	init_symbol.StoreSectionIndex();
	term_symbol.StoreSectionIndex();
}

void PEFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PEF format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump, Dumper::Header);

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
	header_region.Display(dump, Dumper::Header);

	Dumper::Region section_headers_region("Section headers", file_offset + ContainerHeaderSize, SectionHeaderSize * sections.size(), 8);
	section_headers_region.Display(dump, Dumper::Header);

	for(uint16_t section_number = 0; section_number < sections.size(); section_number++)
	{
		auto section = sections[section_number];
		std::unique_ptr<Dumper::Region> section_region;
		if(section->IsInstantiated() && section->section_kind != Section::PatternInitializedData)
		{
			section_region = std::make_unique<Dumper::Block>("Section",
				section->container_offset,
				section->image->AsImage(),
				section->default_address,
				8);
		}
		else
		{
			section_region = std::make_unique<Dumper::Region>("Section",
				section->container_offset,
				section->packed_size,
				8);
		}
		section_region->InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(section_number + 1));
		section_region->AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(section->name_offset));
		section_region->AddOptionalField("Name", Dumper::StringDisplay::Make("\""), section->name);
		section_region->AddField("Total size", Dumper::HexDisplay::Make(8), offset_t(section->total_size));
		section_region->AddField("Unpacked size", Dumper::HexDisplay::Make(8), offset_t(section->unpacked_size));
		section_region->AddField("Packed size", Dumper::HexDisplay::Make(8), offset_t(section->packed_size));
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
		section_region->AddField("Section kind", Dumper::ChoiceDisplay::Make(section_type), offset_t(section->section_kind));
		static const std::map<offset_t, std::string> share_type =
		{
			{ Section::ProcessShare,   "ProcessShare" },
			{ Section::GlobalShare,    "GlobalShare" },
			{ Section::ProtectedShare, "ProtectedShare" },
		};
		section_region->AddField("Share kind", Dumper::ChoiceDisplay::Make(share_type), offset_t(section->share_kind));
		section_region->AddField("Alignment", Dumper::DecDisplay::Make(), offset_t(1 << section->alignment));
		section_region->AddOptionalField("Reserved", Dumper::HexDisplay::Make(2), offset_t(section->reserved));

		if(section->section_kind == Section::PatternInitializedData)
		{
			section_region->Display(dump, Dumper::Header | Dumper::Image | Dumper::Generated);

			// print records for PatternInitializedData
			uint32_t pattern_index = 0;
			for(auto pattern : section->patterns)
			{
				Dumper::Entry pattern_entry("Pattern", pattern_index + 1);
				pattern_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(pattern.file_offset));

				static const std::map<offset_t, std::string> opcode_type =
				{
					{ PatternInitialization::Zero,                               "Zero" },
					{ PatternInitialization::BlockCopy,                          "blockCopy" },
					{ PatternInitialization::RepeatedBlock,                      "repeatedBlock" },
					{ PatternInitialization::InterleaveRepeatBlockWithBlockCopy, "interleaveRepeatBlockWithBlockCopy" },
					{ PatternInitialization::InterleaveRepeatBlockWithZero,      "interleaveRepeatBlockWithZero" },
				};

				pattern_entry.AddField("Opcode", Dumper::ChoiceDisplay::Make(opcode_type), offset_t(pattern.opcode));

				switch(pattern.opcode)
				{
				case PatternInitialization::Zero:
					pattern_entry.AddField("Count", Dumper::HexDisplay::Make(8), offset_t(pattern.count));
					break;
				case PatternInitialization::BlockCopy:
					pattern_entry.AddField("Block size", Dumper::HexDisplay::Make(8), offset_t(pattern.common_data.size()));
					pattern_entry.AddField("Raw data", Dumper::StringDisplay::Make("\""), std::string(reinterpret_cast<char *>(pattern.common_data.data()), pattern.common_data.size()));
					break;
				case PatternInitialization::RepeatedBlock:
					pattern_entry.AddField("Block size", Dumper::HexDisplay::Make(8), offset_t(pattern.common_data.size()));
					pattern_entry.AddField("Repeat count", Dumper::HexDisplay::Make(8), offset_t(pattern.count));
					pattern_entry.AddField("Raw data", Dumper::StringDisplay::Make("\""), std::string(reinterpret_cast<char *>(pattern.common_data.data()), pattern.common_data.size()));
					break;
				case PatternInitialization::InterleaveRepeatBlockWithBlockCopy:
					pattern_entry.AddField("Common size", Dumper::HexDisplay::Make(8), offset_t(pattern.common_data.size()));
					pattern_entry.AddField("Custom size", Dumper::HexDisplay::Make(8), offset_t(pattern.custom_data[0].size()));
					pattern_entry.AddField("Repeat count", Dumper::HexDisplay::Make(8), offset_t(pattern.custom_data.size()));
					pattern_entry.AddField("Common data", Dumper::StringDisplay::Make("\""), std::string(reinterpret_cast<char *>(pattern.common_data.data()), pattern.common_data.size()));
					{
						uint32_t data_index = 0;
						for(auto& custom_data : pattern.custom_data)
						{
							std::ostringstream oss;
							oss << "Custom data " << data_index + 1;
							pattern_entry.AddField(oss.str(), Dumper::StringDisplay::Make("\""), std::string(reinterpret_cast<char *>(custom_data.data()), custom_data.size()));
							data_index ++;
						}
					}
					break;
				case PatternInitialization::InterleaveRepeatBlockWithZero:
					pattern_entry.AddField("Common size", Dumper::HexDisplay::Make(8), offset_t(pattern.count));
					pattern_entry.AddField("Custom size", Dumper::HexDisplay::Make(8), offset_t(pattern.custom_data[0].size()));
					pattern_entry.AddField("Repeat count", Dumper::HexDisplay::Make(8), offset_t(pattern.custom_data.size()));
					{
						uint32_t data_index = 0;
						for(auto& custom_data : pattern.custom_data)
						{
							std::ostringstream oss;
							oss << "Custom data " << data_index + 1;
							pattern_entry.AddField(oss.str(), Dumper::StringDisplay::Make("\""), std::string(reinterpret_cast<char *>(custom_data.data()), custom_data.size()));
							data_index ++;
						}
					}
					break;
				}
				pattern_entry.Display(dump, Dumper::Control);

				pattern_index ++;
			}

			section_region = std::make_unique<Dumper::Block>("Unpacked section data",
				0, // for pattern initialized data, image represents the unpacked data, so the file offset makes no sense
				section->image->AsImage(),
				section->default_address,
				8);
		}

		if(section->contains_relocations)
		{
			section_region->AddField("Relocation record size", Dumper::HexDisplay::Make(8), offset_t(section->reloc_instr_size));
			section_region->AddField("Relocation record offset", Dumper::HexDisplay::Make(8), offset_t(section->first_reloc_offset));

			for(auto& relocation : section->relocations)
			{
				auto section_block = dynamic_cast<Dumper::Block *>(section_region.get());
				if(section_block)
				{
					section_block->AddSignal(relocation.first, 4);
				}
			}
		}

		if(section->section_kind == Section::Loader)
		{
			if(main_symbol.IsPresent())
			{
				section_region->AddField("Main symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(main_symbol.section), offset_t(main_symbol.offset));
			}
			else
			{
				section_region->AddField("Main symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			if(init_symbol.IsPresent())
			{
				section_region->AddField("Initialization function symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(init_symbol.section), offset_t(init_symbol.offset));
			}
			else
			{
				section_region->AddField("Initialization function symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			if(init_symbol.IsPresent())
			{
				section_region->AddField("Termination function symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(init_symbol.section), offset_t(init_symbol.offset));
			}
			else
			{
				section_region->AddField("Termination function symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			section_region->AddField("Imported library count", Dumper::DecDisplay::Make(), offset_t(imported_libraries.size()));
			section_region->AddField("Total imported symbol count", Dumper::DecDisplay::Make(), offset_t(imported_symbols.size()));

			section_region->AddField("Sections with relocations", Dumper::DecDisplay::Make(), offset_t(reloc_section_indexes.size()));
			section_region->AddOptionalField("Offset to relocations", Dumper::HexDisplay::Make(8), offset_t(reloc_instr_offset));

			section_region->AddField("Loader string offset", Dumper::HexDisplay::Make(8), offset_t(loader_strings_offset));
		}
		section_region->Display(dump, Dumper::Header | Dumper::Image);

		if(section->contains_relocations)
		{
			uint32_t opcode_index = 0;
			for(auto& opcode : section->reloc_opcodes)
			{
				opcode.Dump(dump, *this, opcode_index);
				opcode_index++;
			}

			uint32_t reloc_index = 0;
			for(auto& relocation : section->relocations)
			{
				static const std::map<offset_t, std::string> relocation_type =
				{
					{ Relocation::Section, "Section" },
					{ Relocation::Symbol,  "Imported symbol" },
				};
				Dumper::Entry reloc_entry("Relocation", reloc_index + 1);
				reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(relocation.first));
				reloc_entry.AddField("Target", Dumper::ChoiceDisplay::Make(relocation_type), offset_t(relocation.second.type));
				reloc_entry.AddField(
					relocation.second.type == Relocation::Symbol ? "Symbol index" : "Section index", Dumper::DecDisplay::Make(), offset_t(relocation.second.number));

				if(relocation.second.type == Relocation::Symbol)
				{
					// for symbols, display library and symbol name
					if(auto symbol = relocation.second.symbol.lock())
					{
						if(auto library = symbol->library.lock())
						{
							reloc_entry.AddField("Library", Dumper::StringDisplay::Make("'"), library->name);
						}
						reloc_entry.AddField("Symbol", Dumper::StringDisplay::Make("'"), symbol->name);
					}
				}
				reloc_entry.Display(dump, Dumper::Relocation | Dumper::Generated);

				reloc_index++;
			}
		}

		if(section->section_kind == Section::Loader)
		{
			static const std::map<offset_t, std::string> symbol_type =
			{
				{ PEFFormat::Code,  "Code" },
				{ PEFFormat::Data,  "Data" },
				{ PEFFormat::TVect, "Transition Vector" },
				{ PEFFormat::TOC,   "Table of Contents" },
				{ PEFFormat::Glue,  "Linker inserted glue symbol" },
			};

			offset_t library_index = 0;
			for(auto library : imported_libraries)
			{
				Dumper::Region library_region("Imported library", loader_section_offset + LoaderHeaderSize + LibraryDescriptionSize * library_index, LibraryDescriptionSize, 8);
				library_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(library_index + 1));
				library_region.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(library->name_offset));
				library_region.AddField("Name", Dumper::StringDisplay::Make("'"), library->name);
				library_region.AddField("Old implementation version", Dumper::DecDisplay::Make(), offset_t(library->old_imp_version));
				library_region.AddField("Current version", Dumper::DecDisplay::Make(), offset_t(library->current_version));
				library_region.AddField("First symbol", Dumper::DecDisplay::Make(), offset_t(library->first_imported_symbol + 1));
				library_region.AddField("Symbol count", Dumper::DecDisplay::Make(), offset_t(library->imported_symbol_count));
				library_region.AddField("Options",
					Dumper::BitFieldDisplay::Make(2)
						->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("weak import"), true)
						->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("must initialize before client"), true),
					offset_t(library->options));
				library_region.AddOptionalField("Reserved A", Dumper::HexDisplay::Make(2), offset_t(library->reserved_a));
				library_region.AddOptionalField("Reserved B", Dumper::HexDisplay::Make(4), offset_t(library->reserved_b));
				library_region.Display(dump, Dumper::Import);

				offset_t symbol_index = 0;
				for(auto symbol : library->imported_symbols)
				{
					Dumper::Entry symbol_entry("Library symbol", symbol_index + 1);
					symbol_entry.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(symbol->name_offset));
					symbol_entry.AddField("Name", Dumper::StringDisplay::Make("'"), symbol->name);
					symbol_entry.AddField("Class", Dumper::ChoiceDisplay::Make(symbol_type), offset_t(symbol->symbol_class));
					symbol_entry.AddField("Flags",
						Dumper::BitFieldDisplay::Make(2)
							->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("weak"), true),
						offset_t(symbol->flags | symbol->symbol_class));
					symbol_entry.Display(dump, Dumper::Import | Dumper::Symbol | Dumper::Redundant);

					symbol_index ++;
				}

				library_index ++;
			}

			// symbols
			offset_t symbol_index = 0;
			for(auto symbol : imported_symbols)
			{
				Dumper::Entry symbol_entry("Symbol", symbol_index);
				if(auto library = symbol->library.lock())
				{
					symbol_entry.AddField("Library", Dumper::StringDisplay::Make("'"), library->name);
				}
				symbol_entry.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(symbol->name_offset));
				symbol_entry.AddField("Name", Dumper::StringDisplay::Make("'"), symbol->name);
				symbol_entry.AddField("Class", Dumper::ChoiceDisplay::Make(symbol_type), offset_t(symbol->symbol_class));
				symbol_entry.AddField("Flags",
					Dumper::BitFieldDisplay::Make(2)
						->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("weak"), true),
					offset_t(symbol->flags | symbol->symbol_class));
				symbol_entry.Display(dump, Dumper::Import | Dumper::Symbol);

				symbol_index ++;
			}

			Dumper::Region relocations_region("Relocations area", loader_section_offset + reloc_instr_offset, loader_strings_offset - reloc_instr_offset, 8);
			relocations_region.Display(dump, Dumper::Header | Dumper::Relocation);
			uint32_t opcode_index = 0;
			for(auto& opcode : relocs_area)
			{
				opcode.Dump(dump, *this, opcode_index, Dumper::Redundant);
				opcode_index ++;
			}

			Dumper::Region hash_table_region("Export hash table", loader_section_offset + export_hash_offset, GetExportHashTableSize(), 8);
			hash_table_region.Display(dump, Dumper::Header | Dumper::Export);
			offset_t hash_index = 0;
			for(auto& hash_table_entry : hash_table)
			{
				Dumper::Entry dump_entry("Table entry", hash_index);
				dump_entry.AddField("Chain count", Dumper::DecDisplay::Make(), offset_t(hash_table_entry.chain_count));
				dump_entry.AddField("First index", Dumper::DecDisplay::Make(), offset_t(hash_table_entry.first_index));
				dump_entry.Display(dump, Dumper::Export);
				hash_index ++;
			}

			symbol_index = 0;
			for(auto& symbol : exported_symbols)
			{
				Dumper::Entry symbol_entry("Exported symbol", symbol_index);
				symbol_entry.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(symbol->name_offset));
				symbol_entry.AddField("Name", Dumper::StringDisplay::Make("'"), symbol->name);
				symbol_entry.AddField("Hash value", Dumper::HexDisplay::Make(4), offset_t(symbol->hash_value));
				symbol_entry.AddField("Class", Dumper::ChoiceDisplay::Make(symbol_type), offset_t(symbol->symbol_class));
				switch(symbol->section)
				{
				case Absolute:
					symbol_entry.AddField("Value", Dumper::HexDisplay::Make(8), offset_t(symbol->offset));
					break;
				case Reexported:
					symbol_entry.AddField("Imported symbol", Dumper::DecDisplay::Make(8), offset_t(symbol->offset));
					{
						auto imported_symbol = imported_symbols[symbol->offset];
						if(auto library = imported_symbol->library.lock())
						{
							symbol_entry.AddField("Library", Dumper::StringDisplay::Make("'"), library->name);
						}
						symbol_entry.AddField("Name", Dumper::StringDisplay::Make("'"), imported_symbol->name);
					}
					break;
				default:
					symbol_entry.AddField("Value", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(uint16_t(symbol->section)), offset_t(symbol->offset));
					break;
				}
				symbol_entry.Display(dump, Dumper::Export);
				symbol_index ++;
			}

			Dumper::Region string_table_region("Loader string table", loader_section_offset + loader_strings_offset, GetExportHashTableSize(), 8);
			string_table_region.Display(dump, Dumper::Header | Dumper::String);

			offset_t string_offset = 0;
			offset_t string_index = 0;
			for(auto& string : loader_string_table)
			{
				Dumper::Entry string_entry("Loader string", string_index + 1);
				string_entry.AddField("Offset", Dumper::HexDisplay::Make(8), string_offset);
				std::string name = string;
				bool null_terminated = false;
				if(name.back() == '\0')
				{
					name = name.substr(0, name.size() - 1);
					null_terminated = true;
				}
				string_entry.AddField("Name", Dumper::StringDisplay::Make("'"), name);
				string_entry.AddField("Null terminated", Dumper::ChoiceDisplay::Make("true", "false"), offset_t(null_terminated));
				string_entry.Display(dump, Dumper::String);
				string_offset += string.size();
				string_index ++;
			}
		}
	}

	Dumper::Region section_names_region("Section name table", GetSectionNameTableOffset(), section_name_table_end - GetSectionNameTableOffset(), 8);
	// TODO: print names
	section_names_region.Display(dump, Dumper::String);

	// TODO
}

void PEFFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->sections.size() == 0)
		return;

	auto first_section = segment->sections[0];
	if(first_section->GetFlags() & Linker::Section::Resource)
	{
		// TODO
	}
	else if(first_section->IsExecutable())
	{
		sections.push_back(std::make_shared<Section>(Section::Code, Section::GlobalShare, segment));
	}
	else
	{
		sections.push_back(std::make_shared<Section>(Section::UnpackedData, Section::ProcessShare, segment));
	}
}

std::unique_ptr<Script::List> PEFFormat::GetScript(Linker::Module& module)
{
	static const char * DefaultScript = R"(
".code"
{
	all execute;
	at align(here, ?code_section_align?);
};

".data"
{
	at 0;
	all not zero and not resource;
	all zero and not resource;
	at align(here, ?data_section_align?);
};

".rsrc"
{
	all resource; // TODO
}
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

void PEFFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);
}

void PEFFormat::SortImports()
{
	// for now, nothing needs to be done
}

void PEFFormat::ProcessRelocations(Linker::Module& module)
{
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;

		if(rel.kind != Linker::Relocation::Direct)
		{
			Linker::Error << "Error: invalid external relocation, ignoring" << std::endl;
			continue;
		}
		if(rel.size != 4)
		{
			Linker::Error << "Error: only 4-byte external relocation allowed, ignoring" << std::endl;
			continue;
		}
		if((rel.mask & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			Linker::Error << "Error: relocation mask not supported, ignoring" << std::endl;
			Linker::Error << rel.mask << std::endl;
			continue;
		}
		if(rel.shift != 0)
		{
			Linker::Error << "Error: relocation shift not supported, ignoring" << std::endl;
			continue;
		}

		auto source_position = rel.source.GetPosition();
		auto source_section = segment_to_section_map[source_position.segment];
		switch(source_section->section_kind)
		{
		case Section::Code:
		case Section::Constant:
			Linker::Error << "Error: relocation in read-only segment, proceeding" << std::endl;
			break;
		default:
			break;
		}

		auto source_offset = source_position.address - source_position.segment->base_address;

		if(rel.Resolve(module, resolution))
		{
			rel.WriteWord(resolution.value);

			auto target_section = segment_to_section_map[resolution.target];
			source_section->relocations[source_offset] = Relocation::ToSection(target_section);
		}
		else if(Linker::SymbolName * symbol = std::get_if<Linker::SymbolName>(&rel.target.target))
		{
			std::string library, name;
			std::shared_ptr<ImportedSymbol> imported_symbol;
			uint32_t relative = 0;

			if(symbol->GetImportedName(library, name))
			{
				imported_symbol = FetchImportLibrary(library)->GetImportByName(name);
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
				continue;
			}
			else
			{
				relative = -reference.address;
			}

			rel.WriteWord(relative + rel.addend);

			source_section->relocations[source_offset] = Relocation::ToSymbol(imported_symbol);
		}
		else
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
	}
}

void PEFFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	sections.push_back(std::make_shared<Section>(Section::Loader, Section::GlobalShare, nullptr));

	for(auto section : sections)
	{
		if(auto segment = std::dynamic_pointer_cast<Linker::Segment>(section->image))
		{
			segment_to_section_map[segment] = section;
		}
	}

	ProcessRelocations(module);
	SortImports();

	// collect exported symbols

	for(auto symbol : module.GetExportedSymbols())
	{
		std::string name;
		uint16_t hint; // we reinterpret hint to be the symbol type: 0 for code, 1 for data, 2 for transition vector, 3 for TOC symbol, 4 for glue symbol
		if(symbol.first.GetExportedByName(name, hint))
		{
			// check if there was a hint provided
			if(hint == 0 && !symbol.first.LoadOrdinalOrHint(hint))
			{
				hint = 1; // data will be the default
			}
			else if(!IsValidSymbolClass(hint))
			{
				Linker::Error << "Error: invalid symbol class " << hint << std::endl;
				hint = 1; // revert to data
			}
			auto exported_symbol = std::make_shared<ExportedSymbol>(name, symbol_class_type(hint));
			auto position = symbol.second.GetPosition();
			exported_symbol->SetPosition(*this, position);
			exported_symbols.push_back(exported_symbol);
		}
		else
		{
			Linker::Error << "Error: PEF does not support exports by ordinal" << std::endl;
		}
	}

	uint8_t export_hash_table_power = ComputeHashTableExponent(exported_symbols.size());

	// collect exported symbols into chains
	hash_table.clear();
	hash_table.resize(1 << export_hash_table_power);
	for(auto symbol : exported_symbols)
	{
		uint32_t hash_value = ComputeHashWord(symbol->name);
		uint32_t hash_index = HashTableIndex(hash_value, export_hash_table_power);
		hash_table[hash_index].chain.push_back(symbol);
	}

	// reorder the exported symbols so symbols belonging to the same chain appear contiguously
	exported_symbols.clear();
	for(auto& hash_table_entry : hash_table)
	{
		hash_table_entry.chain_count = hash_table_entry.chain.size();
		hash_table_entry.first_index = exported_symbols.size();
		exported_symbols.insert(
			exported_symbols.end(),
			hash_table_entry.chain.begin(),
			hash_table_entry.chain.end());
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		main_symbol.SetPosition(*this, entry.GetPosition());
	}
	else
	{
		if(exported_symbols.size() == 0)
		{
			Linker::Warning << "Warning: no entry point or exported symbols specified specified" << std::endl;
		}
	}

	// TODO: set init_symbol, term_symbol?
}

void PEFFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	switch(module.cpu)
	{
	case Linker::Module::M68K:
		architecture = M68K;
		break;
	case Linker::Module::PPC:
		architecture = PPC;
		break;
	default:
		Linker::Error << "Error: Unsupported CPU type" << std::endl;
	}

	linker_parameters["code_section_align"] = 0x00001000;
	linker_parameters["data_section_align"] = 0x00000010;

	Linker::OutputFormat::GenerateFile(filename, module);
}

