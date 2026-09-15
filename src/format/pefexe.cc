
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

void PEFFormat::RelocationProcessor::Initialize()
{
	//reloc_instr_ptr = 0;
	reloc_address = 0;
	import_index = 0;
	section_c = pef_format.sections[0]->IsInstantiated() ? 0 : SymbolReference::NoSection;
	section_d = pef_format.sections[1]->IsInstantiated() ? 1 : SymbolReference::NoSection;
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
		rd.Seek(container_offset);
		pef_format.loader_section_offset = container_offset;
		pef_format.ReadLoaderSection(rd);
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
	else
	{
		name_offset = NoNameOffset;
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
		wr.Seek(container_offset);
		pef_format.WriteLoaderSection(wr);
		break;
	default:
		break;
	}
}

std::string PEFFormat::Name::LoadNameString(const PEFFormat& pef_format, Linker::Reader& rd)
{
	rd.Seek(pef_format.loader_section_offset + pef_format.loader_strings_offset + name_offset);
	return name = rd.ReadASCII('\0');
}

void PEFFormat::ReadLoaderSection(Linker::Reader& rd)
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
	export_hash_table_power = rd.ReadUnsigned(4);
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
		symbol->symbol_class = ImportedSymbol::class_type((value >> 24) & 0x0F);
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
			rd.Seek(loader_section_offset + reloc_instr_offset + section->first_reloc_offset);
			while(rd.Tell() < loader_section_offset + reloc_instr_offset + section->first_reloc_offset + section->reloc_instr_size)
			{
				RelocOpcode opcode;
				opcode.ReadFile(rd);
				section->reloc_opcodes.push_back(opcode);
			}

			RelocationProcessor processor(*this, section->reloc_opcodes, section->relocations);
			processor.GenerateRelocations();

			for(auto& relocation : section->relocations)
			{
				if(relocation.type == Relocation::Symbol)
				{
					relocation.symbol = imported_symbols[relocation.number];
				}
			}
		}
	}

	// TODO: read all relocation instructions

	//// loader string table

	for(auto library : imported_libraries)
	{
		library->LoadNameString(*this, rd);
	}

	for(auto symbol : imported_symbols)
	{
		symbol->LoadNameString(*this, rd);
	}

	// TODO: read full table

	//// export hash table

	// TODO

	//// export key table

	// TODO

	//// exported symbol table

	// TODO
}

void PEFFormat::WriteLoaderSection(Linker::Writer& wr) const
{
	//// header

	// TODO

	//// imported library descriptions

	// TODO

	//// imported symbol tables

	// TODO

	//// relocation headers

	// TODO

	//// relocation area

	// TODO

	//// loader string table

	// TODO

	//// export hash table

	// TODO

	//// export key table

	// TODO

	//// exported symbol table

	// TODO
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
		section_name_table.push_back(rd.ReadASCII('\0'));
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

	// TODO: initialize imported symbol table

	if(reloc_instr_offset < GetMinimumRelocInstrOffset())
	{
		reloc_instr_offset = GetMinimumRelocInstrOffset();
	}

	if(loader_strings_offset < reloc_instr_offset + GetRelocationAreaSize())
	{
		loader_strings_offset = reloc_instr_offset + GetRelocationAreaSize();
	}

	if(export_hash_offset < loader_strings_offset + GetLoaderStringAreaSize())
	{
		export_hash_offset = loader_strings_offset + GetLoaderStringAreaSize();
	}

	uint32_t section_offset = section_name_table_end;
	for(auto section : sections)
	{
		section->container_offset = ::AlignTo(section_offset, section->ExpectedAlignment());
		if(section->section_kind == Section::Loader)
		{
			loader_section_offset = section->container_offset;
		}
		section_offset = section->container_offset + section->GetImageSize(*this);
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

	Dumper::Region section_headers_region("Section headers", file_offset + ContainerHeaderSize, SectionHeaderSize * sections.size(), 8);
	section_headers_region.Display(dump);

	for(uint16_t section_number = 0; section_number < sections.size(); section_number++)
	{
		auto section = sections[section_number];
		Dumper::Block section_block("Section",
			section->section_kind != Section::PatternInitializedData ? section->container_offset : 0,
				// for pattern initialized data, image represents the unpacked data, so the file offset makes no sense
			section->image ? section->image->AsImage() : nullptr,
				// no image for loader section
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
		if(section->contains_relocations)
		{
			section_block.AddField("Relocation record size", Dumper::HexDisplay::Make(8), offset_t(section->reloc_instr_size));
			section_block.AddField("Relocation record offset", Dumper::HexDisplay::Make(8), offset_t(section->first_reloc_offset));

			for(auto& relocation : section->relocations)
			{
				section_block.AddSignal(relocation.offset, 4);
			}
		}

		if(section->section_kind == Section::Loader)
		{
			if(main_symbol.IsPresent())
			{
				section_block.AddField("Main symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(main_symbol.section), offset_t(main_symbol.offset));
			}
			else
			{
				section_block.AddField("Main symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			if(init_symbol.IsPresent())
			{
				section_block.AddField("Initialization function symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(init_symbol.section), offset_t(init_symbol.offset));
			}
			else
			{
				section_block.AddField("Initialization function symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			if(init_symbol.IsPresent())
			{
				section_block.AddField("Termination function symbol", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(init_symbol.section), offset_t(init_symbol.offset));
			}
			else
			{
				section_block.AddField("Termination function symbol", Dumper::StringDisplay::Make(), std::string("-"));
			}

			section_block.AddField("Imported library count", Dumper::DecDisplay::Make(), offset_t(imported_libraries.size()));
			section_block.AddField("Total imported symbol count", Dumper::DecDisplay::Make(), offset_t(imported_symbols.size()));

			section_block.AddField("Sections with relocations", Dumper::DecDisplay::Make(), offset_t(reloc_section_indexes.size()));
			section_block.AddOptionalField("Offset to relocations", Dumper::HexDisplay::Make(8), offset_t(reloc_instr_offset));

			section_block.AddField("Loader string offset", Dumper::HexDisplay::Make(8), offset_t(loader_strings_offset));

#if 0
	export_hash_offset = rd.ReadUnsigned(4);
	export_hash_table_power = rd.ReadUnsigned(4);
	uint32_t exported_symbol_count = rd.ReadUnsigned(4);
#endif
		}
		// TODO: print records for PatternInitializedData
		section_block.Display(dump);

		if(section->contains_relocations)
		{
			uint32_t opcode_index = 0;
			for(auto& opcode : section->reloc_opcodes)
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
				reloc_entry.AddField("File offset", Dumper::HexDisplay::Make(8), offset_t(opcode.offset));
				reloc_entry.AddField("Opcode", Dumper::ChoiceDisplay::Make(opcode_type), offset_t(opcode.opcode));
				reloc_entry.AddField("Width", Dumper::DecDisplay::Make(), offset_t(opcode.CodeSize()));
				reloc_entry.AddField("Record", Dumper::HexDisplay::Make(2 * opcode.CodeSize()), offset_t(opcode.GetWord()));
				switch(opcode.opcode)
				{
				case RelocOpcode::BySectDWithSkip:
					reloc_entry.AddField("Skip", Dumper::HexDisplay::Make(8), offset_t(opcode.value));
					reloc_entry.AddField("Count", Dumper::DecDisplay::Make(), offset_t(opcode.repeat));
					break;
				case RelocOpcode::BySectC:
				case RelocOpcode::BySectD:
				case RelocOpcode::TVector12:
				case RelocOpcode::TVector8:
				case RelocOpcode::VTable8:
				case RelocOpcode::ImportRun:
					reloc_entry.AddField("Count", Dumper::DecDisplay::Make(), offset_t(opcode.repeat));
					break;
				case RelocOpcode::SmByImport:
				case RelocOpcode::SmSetSectC:
				case RelocOpcode::SmSetSectD:
				case RelocOpcode::SmBySection:
					reloc_entry.AddField("Index", Dumper::HexDisplay::Make(4), offset_t(opcode.value));
					break;
				case RelocOpcode::IncrPosition:
					reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(4), offset_t(opcode.value));
					break;
				case RelocOpcode::SmRepeat:
				case RelocOpcode::LgRepeat:
					reloc_entry.AddField("Block count", Dumper::DecDisplay::Make(), offset_t(opcode.value));
					reloc_entry.AddField("Repeat count", Dumper::DecDisplay::Make(), offset_t(opcode.repeat));
					break;
				case RelocOpcode::SetPosition:
					reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(opcode.value));
					break;
				case RelocOpcode::LgByImport:
				case RelocOpcode::LgBySection:
				case RelocOpcode::LgSetSectC:
				case RelocOpcode::LgSetSectD:
					reloc_entry.AddField("Index", Dumper::HexDisplay::Make(8), offset_t(opcode.value));
					break;
				default:
					break;
				}
				reloc_entry.Display(dump);

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
				reloc_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(relocation.offset));
				reloc_entry.AddField("Target", Dumper::ChoiceDisplay::Make(relocation_type), offset_t(relocation.type));
				reloc_entry.AddField(
					relocation.type == Relocation::Symbol ? "Symbol index" : "Section index", Dumper::DecDisplay::Make(), offset_t(relocation.number));

				if(relocation.type == Relocation::Symbol)
				{
					// for symbols, display library and symbol name
					if(auto symbol = relocation.symbol.lock())
					{
						if(auto library = symbol->library.lock())
						{
							reloc_entry.AddField("Library", Dumper::StringDisplay::Make("'"), library->name);
						}
						reloc_entry.AddField("Symbol", Dumper::StringDisplay::Make("'"), symbol->name);
					}
				}
				reloc_entry.Display(dump);

				reloc_index++;
			}
		}

		if(section->section_kind == Section::Loader)
		{
			static const std::map<offset_t, std::string> symbol_type =
			{
				{ ImportedSymbol::Code,  "Code" },
				{ ImportedSymbol::Data,  "Data" },
				{ ImportedSymbol::TVect, "Transition Vector" },
				{ ImportedSymbol::TOC,   "Table of Contents" },
				{ ImportedSymbol::Glue,  "Linker inserted glue symbol" },
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
				library_region.Display(dump);

				offset_t symbol_index = 0;
				for(auto symbol : imported_symbols)
				{
					Dumper::Entry symbol_entry("Symbol", symbol_index + 1);
					symbol_entry.AddField("Name offset", Dumper::HexDisplay::Make(8), offset_t(symbol->name_offset));
					symbol_entry.AddField("Name", Dumper::StringDisplay::Make("'"), symbol->name);
					symbol_entry.AddField("Class", Dumper::ChoiceDisplay::Make(symbol_type), offset_t(symbol->symbol_class));
					symbol_entry.AddField("Flags",
						Dumper::BitFieldDisplay::Make(2)
							->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("weak"), true),
						offset_t(symbol->flags | symbol->symbol_class));
					symbol_entry.Display(dump);

					symbol_index ++;
				}

				library_index ++;
			}
			// TODO: print all symbols?
			// TODO: print all relocation opcodes?
		}

		// TODO
	}

	Dumper::Region section_names_region("Section name table", GetSectionNameTableOffset(), section_name_table_end - GetSectionNameTableOffset(), 8);
	// TODO: print names
	section_names_region.Display(dump);

	// TODO
}

