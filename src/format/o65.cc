
#include "o65.h"

/* TODO: unimplemented */

using namespace O65;

void O65Format::Module::Initialize()
{
	mode_word = 0;

	code_base = 0;
	code_image = nullptr;

	data_base = 0;
	data_image = nullptr;

	bss_base = 0;
	bss_size = 0;

	zero_base = 0;
	zero_size = 0;

	stack_size = 0;
}

void O65Format::Module::Clear()
{
	mode_word = 0;

	code_base = 0;
	if(code_image)
		delete code_image;
	code_image = nullptr;

	data_base = 0;
	if(data_image)
		delete data_image;
	data_image = nullptr;

	bss_base = 0;
	bss_size = 0;

	zero_base = 0;
	zero_size = 0;

	stack_size = 0;

	header_options.clear();
	undefined_references.clear();
	code_relocations.clear();
	data_relocations.clear();
	exported_globals.clear();
}

bool O65Format::Module::IsPageRelocatable() const
{
	return mode_word & MODE_PAGE_RELOC;
}

bool O65Format::Module::IsChained() const
{
	return mode_word & MODE_CHAIN;
}

void O65Format::Module::SetChained()
{
	mode_word |= MODE_CHAIN;
}

int O65Format::Module::GetWordSize() const
{
	return mode_word & MODE_SIZE ? 4 : 2;
}

offset_t O65Format::Module::ReadUnsigned(Linker::Reader& rd) const
{
	return rd.ReadUnsigned(GetWordSize());
}

void O65Format::Module::WriteWord(Linker::Writer& wr, offset_t value) const
{
	wr.WriteWord(GetWordSize(), value);
}

void O65Format::Module::ReadFile(Linker::Reader& rd)
{
	char signature[5];
	Clear();

	rd.endiantype = ::LittleEndian;
	rd.ReadData(5, signature);
	if(memcmp(signature, "\1\0o65", 5) != 0)
		throw "Invalid magic number";
	if(rd.ReadUnsigned(1) != 0)
		throw "Invalid format version";

	mode_word = rd.ReadUnsigned(2);
	code_base = ReadUnsigned(rd);
	offset_t code_size = ReadUnsigned(rd);
	data_base = ReadUnsigned(rd);
	offset_t data_size = ReadUnsigned(rd);
	bss_base = ReadUnsigned(rd);
	bss_size = ReadUnsigned(rd);
	zero_base = ReadUnsigned(rd);
	zero_size = ReadUnsigned(rd);
	stack_size = ReadUnsigned(rd);

//	Linker::Debug << "At offset " << rd.Tell() << std::endl;

	int length;
	while((length = rd.ReadUnsigned(1)) != 0)
	{
//		Linker::Debug << "Reading optional header entry of length " << length << std::endl;
		header_options.push_back(header_option(rd.ReadUnsigned(1)));
		header_options.back().data.resize(length);
		rd.ReadData(length, reinterpret_cast<char *>(header_options.back().data.data()));
	}

	Linker::Section * code_section = new Linker::Section(".text");
	code_image = code_section;
	code_section->ReadFile(rd, code_size);
	code_section->SetReadable(true);
	code_section->SetExecable(true);

	Linker::Section * data_section = new Linker::Section(".data");
	data_image = data_section;
	data_section->ReadFile(rd, data_size);
	data_section->SetReadable(true);
	data_section->SetWritable(true);

	offset_t undefined_count = ReadUnsigned(rd);
//	Linker::Debug << "Reading " << undefined_count << " undefined names" << std::endl;
	for(offset_t i = 0; i < undefined_count; i++)
	{
		undefined_references.push_back(rd.ReadASCIIZ());
	}

	std::map<offset_t, relocation> * relocation_parts[2] = { &code_relocations, &data_relocations };
	for(int i = 0; i < 2; i++)
	{
		offset_t offset = -1;
		int delta;
		while((delta = rd.ReadUnsigned(1)) != 0)
		{
			if(delta == 0xFF)
			{
				offset += 0xFE;
				continue;
			}

			offset += delta;
			uint8_t type_segment = rd.ReadUnsigned(1);
			offset_t symbol_index = (type_segment & relocation::RELOC_SEGMENT_MASK) == relocation::RELOC_SEGMENT_UNDEFINED ? ReadUnsigned(rd) : 0;
			offset_t value =
				((type_segment & relocation::RELOC_TYPE_MASK) == relocation::RELOC_TYPE_HIGH) && !IsPageRelocatable()
					? rd.ReadUnsigned(1)
						: (type_segment & relocation::RELOC_TYPE_MASK) == relocation::RELOC_TYPE_SEG
						? rd.ReadUnsigned(2)
							: 0;

			(*relocation_parts[i])[offset] = relocation(type_segment, value, symbol_index);
		}
	}

	offset_t exported_count = ReadUnsigned(rd);
	for(offset_t i = 0; i < exported_count; i++)
	{
		std::string name = rd.ReadASCIIZ();
		uint8_t segment_id = rd.ReadUnsigned(1);
		offset_t value = ReadUnsigned(rd);
		exported_globals.push_back(exported_global(name, segment_id, value));
	}
}

void O65Format::Module::WriteFile(Linker::Writer& wr)
{
	if(data_base == code_base + (code_image ? code_image->ActualDataSize() : 0)
	&& bss_base  == data_base + (data_image ? data_image->ActualDataSize() : 0))
	{
		mode_word |= MODE_SIMPLE;
	}
	else
	{
		mode_word &= ~MODE_SIMPLE;
	}

//Linker::Debug << wr.Tell() << std::endl;

	wr.endiantype = ::LittleEndian;
	wr.WriteData(6, "\1\0o65\0");
	wr.WriteWord(2, mode_word);
	WriteWord(wr, code_base);
	WriteWord(wr, code_image ? code_image->ActualDataSize() : 0);
	WriteWord(wr, data_base);
	WriteWord(wr, data_image ? data_image->ActualDataSize() : 0);
	WriteWord(wr, bss_base);
	WriteWord(wr, bss_size);
	WriteWord(wr, zero_base);
	WriteWord(wr, zero_size);
	WriteWord(wr, stack_size);

	for(auto& header_option : header_options)
	{
		wr.WriteWord(1, header_option.data.size());
		wr.WriteWord(1, header_option.type);
		wr.WriteData(header_option.data.size(), reinterpret_cast<char *>(header_option.data.data()));
	}
	wr.WriteWord(1, 0);

	WriteWord(wr, undefined_references.size());
	for(auto& undefined_reference : undefined_references)
	{
		wr.WriteData(undefined_reference);
		wr.WriteWord(1, 0);
	}

	if(code_image)
		code_image->WriteFile(wr);

	if(data_image)
		data_image->WriteFile(wr);

	std::map<offset_t, relocation> * relocation_parts[2] = { &code_relocations, &data_relocations };
	for(int i = 0; i < 2; i++)
	{
		offset_t offset = -1;
		for(auto pair : *relocation_parts[i])
		{
			int delta = pair.first - offset;
			while(delta > 0xFE)
			{
				wr.WriteWord(1, 0xFF);
				delta -= 0xFE;
			}
			wr.WriteWord(1, delta);
			wr.WriteWord(1, pair.second.type_segment);
			if((pair.second.type_segment & relocation::RELOC_SEGMENT_MASK) == relocation::RELOC_SEGMENT_UNDEFINED)
				WriteWord(wr, pair.second.symbol_index);
			if(((pair.second.type_segment & relocation::RELOC_TYPE_MASK) == relocation::RELOC_TYPE_HIGH) && !IsPageRelocatable())
				wr.WriteWord(1, pair.second.value & 0xFF);
			else if((pair.second.type_segment & relocation::RELOC_TYPE_MASK) == relocation::RELOC_TYPE_SEG)
				wr.WriteWord(2, pair.second.value & 0xFFFF);
		}
		wr.WriteWord(1, 0);
	}

	WriteWord(wr, undefined_references.size());
	for(auto& global : exported_globals)
	{
		wr.WriteData(global.name);
		wr.WriteWord(1, 0);
		wr.WriteWord(1, global.segment_id);
		WriteWord(wr, global.value);
	}
}

void O65Format::Module::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	if((mode_word & MODE_65816))
		module.cpu = Linker::Module::W65K;
	else
		module.cpu = Linker::Module::MOS6502;

	Linker::Section * code_section = code_image != nullptr ? dynamic_cast<Linker::Section *>(code_image) : nullptr;
	if(code_section != nullptr)
	{
		module.AddSection(code_section);
	}

	Linker::Section * data_section = data_image != nullptr ? dynamic_cast<Linker::Section *>(data_image) : nullptr;
	if(data_section != nullptr)
	{
		module.AddSection(data_section);
	}

	Linker::Section * bss_section = bss_size != 0 ? new Linker::Section(".bss") : nullptr;
	if(bss_section != nullptr)
	{
		bss_section->SetZeroFilled(true);
		bss_section->Expand(bss_size);
		module.AddSection(bss_section);
	}

	Linker::Section * zero_section = zero_size != 0 ? new Linker::Section(".zero") : nullptr;
	if(zero_section != nullptr)
	{
		zero_section->SetZeroFilled(true);
		zero_section->Expand(zero_size);
		module.AddSection(zero_section);
	}

	Linker::Section * stack_section = zero_size != 0 ? new Linker::Section(".stack") : nullptr;
	if(stack_section != nullptr)
	{
		stack_section->SetZeroFilled(true);
		stack_section->Expand(zero_size);
		module.AddSection(stack_section);
	}

	Linker::Section * sections[] =
	{
		nullptr,
		nullptr,
		code_section,
		data_section,
		bss_section,
		zero_section,
	};

	for(auto& global : exported_globals)
	{
		unsigned segment_number = global.segment_id & relocation::RELOC_SEGMENT_MASK;
		if(segment_number == 0 || segment_number >= sizeof sections / sizeof sections[0])
		{
			Linker::Error << "Error: Invalid segment number " << segment_number << ", dropping segment" << std::endl;
			continue;
		}
		module.AddGlobalSymbol(
			global.name,
			Linker::Location(sections[segment_number], global.value)
		);
	}

#if 0
	std::vector<Linker::SymbolName> external_symbols;
	for(auto& undefined_reference : undefined_references)
	{
		external_symbols.push_back(Linker::SymbolName
		wr.WriteData(undefined_reference);
		wr.WriteWord(1, 0);
	}
	/* TODO */
#endif

	std::map<offset_t, relocation> * relocation_parts[2] = { &code_relocations, &data_relocations };
	for(int i = 0; i < 2; i++)
	{
		for(auto pair : *relocation_parts[i])
		{
			auto offset = pair.first;
			auto rel = pair.second;
			Linker::Relocation obj_rel = Linker::Relocation::Empty();

			Linker::Location rel_source = Linker::Location(
				i == 0 ? code_section : data_section,
				offset
			);
			Linker::Target rel_target =
				(rel.type_segment & relocation::RELOC_SEGMENT_MASK) == relocation::RELOC_SEGMENT_UNDEFINED
					? Linker::SymbolName(undefined_references[rel.symbol_index])
					: Linker::Target(Linker::Location(sections[rel.type_segment & relocation::RELOC_SEGMENT_MASK], 0));

			switch(rel.type_segment & relocation::RELOC_TYPE_MASK)
			{
			case relocation::RELOC_TYPE_LOW:
				obj_rel = Linker::Relocation::Absolute(1, rel_source, rel_target, rel.value, ::LittleEndian);
				break;
			case relocation::RELOC_TYPE_HIGH:
				obj_rel = Linker::Relocation::Absolute(1, rel_source, rel_target, rel.value, ::LittleEndian);
				obj_rel.SetShift(8);
				break;
			case relocation::RELOC_TYPE_WORD:
				obj_rel = Linker::Relocation::Absolute(2, rel_source, rel_target, rel.value, ::LittleEndian);
				break;
			case relocation::RELOC_TYPE_SEG:
				obj_rel = Linker::Relocation::Absolute(1, rel_source, rel_target, rel.value, ::LittleEndian);
				obj_rel.SetShift(16);
				break;
			case relocation::RELOC_TYPE_SEGADDR:
				obj_rel = Linker::Relocation::Absolute(3, rel_source, rel_target, rel.value, ::LittleEndian);
				break;
			}
			module.relocations.push_back(obj_rel);
		}
	}
}

offset_t O65Format::GetModuleCount()
{
	return modules.size();
}

O65Format::Module& O65Format::GetModule(offset_t index)
{
	return *modules[index];
}

O65Format::Module& O65Format::AddModule()
{
	Module * new_module = new Module;
	assert(modules.size() > 0);
	modules.back()->SetChained();
	modules.push_back(new_module);
	return *new_module;
}

void O65Format::Initialize()
{
}

void O65Format::Clear()
{
	// TODO: switch over from manual memory management to more robust methods
//	for(auto module : modules)
//	{
//		module->Clear();
//		delete module;
//	}
	modules.clear();
}

void O65Format::ReadFile(Linker::Reader& rd)
{
	do
	{
		modules.push_back(new Module);
		modules.back()->ReadFile(rd);
	} while(modules.back()->IsChained());
}

void O65Format::WriteFile(Linker::Writer& wr)
{
	for(auto module : modules)
	{
		module->WriteFile(wr);
	}
}

void O65Format::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	ReadFile(rd);
	modules[0]->ProduceModule(module, rd);
}

