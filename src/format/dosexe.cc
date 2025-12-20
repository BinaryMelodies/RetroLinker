
#include "dosexe.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

void SeychellDOS32::AdamFormat::MakeApplication()
{
	memcpy(signature.data(), "Adam", 4);
}

void SeychellDOS32::AdamFormat::MakeLibrary()
{
	memcpy(signature.data(), "DLL ", 4);
}

uint32_t SeychellDOS32::AdamFormat::GetRelocationSize(uint32_t displacement, relocation_type type)
{
	uint32_t size = 1; // one byte is 0x00-0x7F with nibble order inverted
	uint8_t bytes = 1; // additional bytes is 0x81-0xFF with nibble order inverted, with an additional 0x80 appended each cycle
	displacement >>= 7;
	while(displacement > 0)
	{
		if((displacement & 0x7F) != 0)
		{
			size += bytes;
		}
		bytes ++;
		displacement >>= 7;
	}
	if(type == Selector16)
		size ++;
	return size;
}

void SeychellDOS32::AdamFormat::CalculateValues()
{
	if(stub.filename != "")
	{
		file_offset = stub.GetStubImageSize();
	}

	// v3.3: only selector relocations are supported, DX64: offset relocations are not counted here
	uint32_t relocations_size = 0;

	if(!IsV35())
	{
		selector_relocations.clear();
		for(auto offset_and_type : relocations_map)
		{
			if(offset_and_type.second == Selector16)
				selector_relocations.push_back(offset_and_type.first);
		}
		selector_relocation_count = selector_relocations.size();
		relocations_size = 4 * selector_relocation_count;

		offset_relocations.clear();
		for(auto offset_and_type : relocations_map)
		{
			if(offset_and_type.second == Offset32)
				offset_relocations.push_back(offset_and_type.first);
		}
		offset_relocations_size = 4 * offset_relocations.size();
	}
	else
	{
		uint32_t relocation_source_offset = 0;
		relocations_size = 0;
		for(auto rel : relocations_map)
		{
			relocations_size += GetRelocationSize(rel.first - relocation_source_offset, rel.second);
			relocation_source_offset = rel.first;
		}
	}

	if(!IsDLL())
	{
		MakeApplication();
	}
	else
	{
		MakeLibrary();
	}

	if(IsV35() || format == FORMAT_DX64)
	{
		header_size = std::max(header_size, uint32_t(0x2C));
	}
	else
	{
		header_size = std::max(header_size, uint32_t(0x28));
	}

	program_size = image->ImageSize();
	image_size = header_size + program_size + relocations_size;
	contents_size = image->ImageSize() + relocations_size;
}

void SeychellDOS32::AdamFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	rd.ReadData(signature);
	rd.ReadData(dlink_version);
	rd.ReadData(minimum_dos_version);

	if(!IsV35())
	{
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		program_size = rd.ReadUnsigned(4);
		memory_size = rd.ReadUnsigned(4);
		eip = rd.ReadUnsigned(4);
		esp = rd.ReadUnsigned(4);
		selector_relocation_count = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
		if(header_size >= 0x2C)
		{
			offset_relocations_size = rd.ReadUnsigned(4);
		}
		else
		{
			rd.Skip(header_size - 0x28);
		}
	}
	else
	{
		contents_size = rd.ReadUnsigned(4);
		image_size = rd.ReadUnsigned(4);
		header_size = rd.ReadUnsigned(4);
		eip = rd.ReadUnsigned(4);
		memory_size = rd.ReadUnsigned(4);
		esp = rd.ReadUnsigned(4);
		program_size = rd.ReadUnsigned(4);
		flags = rd.ReadUnsigned(4);
		rd.Skip(header_size - 0x28);
	}

	image = Linker::Buffer::ReadFromFile(rd, program_size);

	if(!IsV35())
	{
		for(size_t i = 0; i < selector_relocation_count; i++)
		{
			uint32_t offset = rd.ReadUnsigned(4);
			selector_relocations.push_back(offset);
			relocations_map[offset] = Selector16;
		}

		// DX64
		for(size_t i = 0; i < offset_relocations_size; i += 4)
		{
			uint32_t offset = rd.ReadUnsigned(4) - 4;
			offset_relocations.push_back(offset);
			relocations_map[offset] = Offset32;
		}
	}
	else
	{
		uint32_t current_offset = 0;

		uint32_t current_advancement = 0;
		bool needs_relocation = false;

		while(rd.Tell() < file_offset + header_size + contents_size)
		{
			uint8_t opcode = rd.ReadUnsigned(1);
			if(opcode == 0x00)
			{
				if(needs_relocation)
				{
					relocations_map[current_offset] = Selector16;
					needs_relocation = false;
				}
				else
				{
					current_offset += current_advancement;
					current_advancement = 0;
					needs_relocation = true;
				}
			}
			else if(opcode == 0x08)
			{
				current_advancement *= 0x80;
			}
			else if((opcode & 0x08) == 0)
			{
				uint8_t offset = (opcode << 4) | (opcode >> 4);

				if(needs_relocation)
				{
					relocations_map[current_offset] = Offset32;
					needs_relocation = false;
				}

				current_offset += current_advancement + offset;
				current_advancement = 0;
				needs_relocation = true;
			}
			else
			{
				uint8_t offset = ((opcode - 8) << 4) | (opcode >> 4);

				if(needs_relocation)
				{
					relocations_map[current_offset] = Offset32;
					needs_relocation = false;
				}

				current_offset += current_advancement;
				current_advancement = offset * 0x80;
			}
		}

		if(needs_relocation)
		{
			relocations_map[current_offset] = Offset32;
			needs_relocation = false;
		}
	}
}

offset_t SeychellDOS32::AdamFormat::WriteFile(Linker::Writer& wr) const
{
	if(stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}

	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	wr.WriteData(signature);
	wr.WriteData(dlink_version);
	wr.WriteData(minimum_dos_version);
	if(!IsV35())
	{
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, image->ImageSize());
		wr.WriteWord(4, memory_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, selector_relocation_count);
		wr.WriteWord(4, flags);
		if(offset_relocations_size != 0 && header_size >= 0x2C)
		{
			wr.WriteWord(4, offset_relocations_size);
			wr.Skip(header_size - 0x2C);
		}
		else
		{
			wr.Skip(header_size - 0x28);
		}
	}
	else
	{
		wr.WriteWord(4, contents_size);
		wr.WriteWord(4, image_size);
		wr.WriteWord(4, header_size);
		wr.WriteWord(4, eip);
		wr.WriteWord(4, memory_size);
		wr.WriteWord(4, esp);
		wr.WriteWord(4, program_size);
		wr.WriteWord(4, flags);
		wr.Skip(header_size - 0x28);
	}

	image->WriteFile(wr);

	if(!IsV35())
	{
		for(auto rel : selector_relocations)
		{
			wr.WriteWord(4, rel);
		}

		// DX64
		for(auto rel : offset_relocations)
		{
			wr.WriteWord(4, rel + 4);
		}
	}
	else
	{
		uint32_t relocation_source_offset = 0;
		for(auto rel : relocations_map)
		{
			uint32_t displacement = rel.first - relocation_source_offset;
			// encode the displacement in base 128
			std::array<uint8_t, 5> values;
			size_t value_count = 0;
			uint32_t tmp = displacement;
			do
			{
				assert(value_count < 5);
				values[value_count] = tmp & 0x7F;
				value_count ++;
				tmp >>= 7;
			} while(tmp != 0);
			for(int i = value_count - 1; i >= 0; i--)
			{
				uint8_t value = values[i];
				if(i > 0)
					value += 0x80;
				uint8_t opcode = (value << 4) | (value >> 4);
				wr.WriteWord(1, opcode);
				uint8_t scales = i > 1 ? i - 1 : 0;
				while(scales-- != 0)
				{
					wr.WriteWord(1, 0x08);
				}
			}
			if(rel.second == Selector16)
				wr.WriteWord(1, 0x00);
			relocation_source_offset = rel.first;
		}
	}

	return offset_t(-1);
}

void SeychellDOS32::AdamFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Adam format");
	Dumper::Region file_region("File", file_offset, image_size + (!IsV35() && offset_relocations.size() != 0 ? 4 * offset_relocations.size() : 0), 8);
	file_region.AddField("Signature", Dumper::StringDisplay::Make(4, "'"), std::string(signature.data(), 4));
	file_region.AddField("DLINK version", Dumper::VersionDisplay::Make(), offset_t(dlink_version[1]), offset_t(dlink_version[0])); // TODO: print as BCD/hexadecimal
	file_region.AddField("DOS version", Dumper::VersionDisplay::Make(), offset_t(minimum_dos_version[1]), offset_t(minimum_dos_version[0])); // TODO: print as BCD/hexadecimal
	if(IsV35())
		file_region.AddField("Image size (after header)", Dumper::HexDisplay::Make(8), offset_t(contents_size));
	if(!IsV35() && offset_relocations.size())
		file_region.AddField("Image size (without offset relocations)", Dumper::HexDisplay::Make(8), offset_t(image_size));
	file_region.AddField("Header size", Dumper::HexDisplay::Make(8), offset_t(header_size));
	file_region.AddField("Entry point (EIP)", Dumper::HexDisplay::Make(8), offset_t(eip)); // TODO: RIP
	file_region.AddField("Starting stack (ESP)", Dumper::HexDisplay::Make(8), offset_t(esp)); // TODO: RSP
	file_region.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(flags)); // TODO: print bits
	file_region.AddOptionalField("Offset relocation size", Dumper::HexDisplay::Make(8), offset_t(offset_relocations_size)); // DX64 only
	file_region.Display(dump);

	Dumper::Block image_block("Image", file_offset + header_size, image->AsImage(), 0 /* TODO */, 8);
	image_block.AddField("Memory size", Dumper::HexDisplay::Make(8), offset_t(memory_size));
	for(auto rel : relocations_map)
	{
		image_block.AddSignal(rel.first, rel.second == Offset32 ? 4 : 2);
	}
	image_block.Display(dump);

	static const std::map<offset_t, std::string> relocation_type_description =
	{
		{ Selector16, "16-bit selector" },
		{ Offset32, "32-bit base offset" },
	};

	if(!IsV35())
	{
		uint32_t relocation_index = 0;
		for(auto rel : selector_relocations)
		{
			Dumper::Entry rel_entry("Relocation", relocation_index + 1, file_offset + header_size + program_size + 4 * relocation_index, 8);
			rel_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(rel));
			rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_description), offset_t(Selector16));
			rel_entry.Display(dump);
			relocation_index ++;
		}

		for(auto rel : offset_relocations)
		{
			Dumper::Entry rel_entry("Relocation", relocation_index + 1, file_offset + header_size + program_size + 4 * relocation_index, 8);
			rel_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(rel));
			rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_description), offset_t(Offset32));
			rel_entry.Display(dump);
			relocation_index ++;
		}
	}
	else
	{
		uint32_t relocation_offset = file_offset + header_size + program_size; // TODO:
		uint32_t relocation_source_offset = 0;
		uint32_t relocation_index = 0;
		for(auto rel : relocations_map)
		{
			uint32_t relocation_size = GetRelocationSize(rel.first - relocation_source_offset, rel.second);
			relocation_source_offset = rel.first;

			Dumper::Entry rel_entry("Relocation", relocation_index + 1, relocation_offset, 8);
			rel_entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(rel.first));
			rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make(relocation_type_description), offset_t(rel.second));
			rel_entry.AddField("Entry size", Dumper::HexDisplay::Make(8), offset_t(relocation_size));
			rel_entry.Display(dump);
			relocation_index ++;
			relocation_offset += relocation_size;
		}
	}
}

/* * * Writer members * * */

bool SeychellDOS32::AdamFormat::FormatSupportsSegmentation() const
{
	return true;
}

unsigned SeychellDOS32::AdamFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	unsigned flags;
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		flags = Linker::Section::Stack;
	}
	else
	{
		flags = 0;
	}
	return flags;
}

std::shared_ptr<Linker::OptionCollector> SeychellDOS32::AdamFormat::GetOptions()
{
	return std::make_shared<AdamOptionCollector>();
}

void SeychellDOS32::AdamFormat::SetOptions(std::map<std::string, std::string>& options)
{
	AdamOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();
	stack_size = collector.stack();
	output = collector.output();
}

void SeychellDOS32::AdamFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		if(image != nullptr)
		{
			Linker::Error << "Error: duplicate `.code` segment, ignoring" << std::endl;
			return;
		}
		image = segment;
		memory_size = segment->TotalSize();
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, ignoring" << std::endl;
	}
}

void SeychellDOS32::AdamFormat::CreateDefaultSegments()
{
	if(image == nullptr)
	{
		image = std::make_shared<Linker::Segment>(".code");
	}
}

std::unique_ptr<Script::List> SeychellDOS32::AdamFormat::GetScript(Linker::Module& module)
{
	static const char * DefaultScript = R"(
".code"
{
	all not zero;
	all not ".stack";
	all;
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

void SeychellDOS32::AdamFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

void SeychellDOS32::AdamFormat::ProcessModule(Linker::Module& module)
{
	module.AllocateStack(stack_size);

	Link(module);

	std::set<uint32_t> relocation_offsets;
	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}

		if(rel.kind == Linker::Relocation::Direct)
		{
			if(resolution.target != nullptr && (format == FORMAT_35 || format == FORMAT_DX64))
			{
				if(rel.size < 4)
				{
					Linker::Warning << "Warning: " << std::dec << (rel.size * 8) << "-bit relocations not supported, suppressing" << std::endl;
				}
				relocations_map[rel.source.GetPosition().address] = Offset32;
			}
			rel.WriteWord(resolution.value);
		}
		else if(rel.kind == Linker::Relocation::SelectorIndex && resolution.target != nullptr)
		{
			if(format != FORMAT_DX64)
			{
				relocations_map[rel.source.GetPosition().address] = Selector16;
			}
			else
			{
				Linker::Error << "Error: unsupported selector relocation, ignoring" << std::endl;
			}
			rel.WriteWord(resolution.value);
		}
		else
		{
			Linker::Error << "Error: unsupported reference type, ignoring" << std::endl;
			continue;
		}
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Debug << "Debug: " << stack_top << std::endl;
		esp = stack_top.GetPosition().address;
	}
	else
	{
		// use end of image as stack top
		esp = memory_size;
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		eip = entry.GetPosition().address;
	}
	else
	{
		eip = 0;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void SeychellDOS32::AdamFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(module.cpu == Linker::Module::X86_64)
	{
		if(format == FORMAT_UNSPECIFIED)
		{
			format = FORMAT_DX64;
			Linker::Debug << "Debug: 64-bit module, generating DX64 output" << std::endl;
		}
		else if(format != FORMAT_DX64)
		{
			Linker::FatalError("Fatal error: Format only supports 32-bit x86 binaries");
		}
	}
	else if(module.cpu == Linker::Module::I386)
	{
		if(format == FORMAT_UNSPECIFIED)
		{
			format = FORMAT_33;
			Linker::Debug << "Debug: 32-bit module, generating DOS32 3.3 output" << std::endl;
		}
		else if(format == FORMAT_DX64)
		{
			Linker::FatalError("Fatal error: Format only supports 64-bit x86 binaries");
		}
	}
	else
	{
		switch(format)
		{
		default:
			Linker::FatalError("Fatal error: Format only supports 32-bit x86 binaries");
			break;
		case FORMAT_UNSPECIFIED:
			Linker::FatalError("Fatal error: Format only supports 32/64-bit x86 binaries");
			break;
		case FORMAT_DX64:
			Linker::FatalError("Fatal error: Format only supports 64-bit x86 binaries");
			break;
		}
	}

	switch(format)
	{
	case FORMAT_UNSPECIFIED: // should not happen
	case FORMAT_33:
	case FORMAT_DX64:
		// 3.0
		dlink_version = { 0x00, 0x03 };
		// 1.20
		minimum_dos_version = { 0x20, 0x01 };
		break;
	case FORMAT_35:
		// 3.50
		dlink_version = { 0x50, 0x03 };
		// 2.0
		minimum_dos_version = { 0x00, 0x02 };
		flags = 0x00040000; // 4MB heap limit for unregistered version
		break;
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string SeychellDOS32::AdamFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe"; // TODO: .dll extension?
}

// D3X

/* untested */

void BorcaD3X::D3X1Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Skip(4); /* signature */
	header_size = rd.ReadUnsigned(4);
	binary_size = rd.ReadUnsigned(4);
	extra_size = rd.ReadUnsigned(4);
	entry = rd.ReadUnsigned(4);
	stack_top = rd.ReadUnsigned(4);
	/* TODO */
}

offset_t BorcaD3X::D3X1Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(4, "D3X1");
	wr.WriteWord(4, header_size);
	wr.WriteWord(4, binary_size);
	wr.WriteWord(4, extra_size);
	wr.WriteWord(4, entry);
	wr.WriteWord(4, stack_top);
	/* TODO */
	return offset_t(-1);
}

void BorcaD3X::D3X1Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("D3X1 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

/* untested */

void DX64::LVFormat::SetSignature(format_type type)
{
	switch(type)
	{
	case FORMAT_FLAT:
		memcpy(signature, "Flat", 4);
		break;
	case FORMAT_LV:
		memcpy(signature, "LV\0\0", 4);
		break;
	default:
		Linker::FatalError("Internal error: invalid format type");
	}
}

void DX64::LVFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.ReadData(4, signature);
	if(memcmp(signature, "Flat", 4) != 0 && memcmp(signature, "LV\0\0", 4) != 0)
	{
		Linker::Error << "Error: invalid signature" << std::endl;
	}
	uint32_t program_size = rd.ReadUnsigned(4);
	eip = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	extra_memory_size = rd.ReadUnsigned(4);

	image = Linker::Buffer::ReadFromFile(rd, program_size);
}

offset_t DX64::LVFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.WriteData(4, signature);
	wr.WriteWord(4, image->ImageSize());
	wr.WriteWord(4, eip);
	wr.WriteWord(4, esp);
	wr.WriteWord(4, extra_memory_size);
	image->WriteFile(wr);
	return offset_t(-1);
}

void DX64::LVFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("LV/Flat format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

