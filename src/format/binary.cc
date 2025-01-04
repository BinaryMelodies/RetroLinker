
#include "binary.h"

using namespace Binary;

// GenericBinaryFormat

/* * * General members * * */

void GenericBinaryFormat::Clear()
{
	/* format fields */
	image = nullptr;
}

void GenericBinaryFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = (::EndianType)0; /* does not matter */

	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Buffer>();
	buffer->ReadFile(rd);
	image = buffer;
}

void GenericBinaryFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = (::EndianType)0; /* does not matter */
	image->WriteFile(wr);
}

void GenericBinaryFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Binary format");

	Dumper::Block image_block("Image", file_offset, image, 0, 4);

	image_block.Display(dump);
}

/* * * Writer members * * */
void GenericBinaryFormat::SetOptions(std::map<std::string, std::string>& options)
{
	SetLinkerParameter(options, "base_address");
	SetLinkerParameter(options, "segment_bias");
}

void GenericBinaryFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		image = segment;
		base_address = segment->base_address;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, expected `.code`, ignoring" << std::endl;
	}
}

void GenericBinaryFormat::CreateDefaultSegments()
{
	if(image == nullptr)
	{
		image = std::make_shared<Linker::Segment>(".code");
	}
}

std::unique_ptr<Script::List> GenericBinaryFormat::GetScript(Linker::Module& module)
{
	/* most targets */
	static const char * GenericScript = R"(
".code"
{
	at ?base_address?;
	all not zero align ?section_align?;
	all not ".stack" align ?section_align?;
	all align ?section_align?;
};
)";

	return Script::parse_string(GenericScript);
}

bool GenericBinaryFormat::ProcessRelocation(Linker::Module& module, Linker::Relocation& rel, Linker::Resolution resolution)
{
	if(resolution.target != nullptr && resolution.reference == nullptr)
	{
		if(position_independent)
		{
			/* if script file is provided, this might not be an actual relocatable file format, ignore error */
			if(module.cpu == Linker::Module::M68K)
			{
				Linker::Error << "Error: Position independent format (such as Human68k .r) does not support absolute addresses, assuming 0" << std::endl;
			}
			else
			{
				Linker::Error << "Error: Position independent format does not support absolute addresses, assuming 0" << std::endl;
			}
		}
		else if(module.cpu == Linker::Module::I86)
		{
			Linker::Error << "Error: MS-DOS .com does not support segment relocations, assuming 0" << std::endl;
		}
	}
	rel.WriteWord(resolution.value);
	return true;
}

void GenericBinaryFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);
	ProcessScript(script, module);
	CreateDefaultSegments();
}

void GenericBinaryFormat::ProcessModule(Linker::Module& module)
{
	Link(module);

	for(Linker::Relocation& rel : module.relocations)
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
			continue;
		}
		else
		{
			ProcessRelocation(module, rel, resolution);
		}
	}
}

void GenericBinaryFormat::CalculateValues()
{
}

void GenericBinaryFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(linker_parameters.find("base_address") == linker_parameters.end())
	{
		if(module.cpu == Linker::Module::I86)
		{
			linker_parameters["base_address"] = Linker::Location();
		}
		else
		{
			Linker::Debug << "Debug: Using default base address of " << base_address << std::endl;
			linker_parameters["base_address"] = Linker::Location(base_address);
		}
	}

	if(module.cpu == Linker::Module::I86 && linker_parameters.find("segment_bias") == linker_parameters.end())
	{
		linker_parameters["segment_bias"] = Linker::Location(0x100);
	}

	if(linker_parameters.find("section_align") == linker_parameters.end())
	{
		switch(module.cpu)
		{
		case Linker::Module::I86:
			linker_parameters["section_align"] = Linker::Location(16);
			break;
		case Linker::Module::M68K:
		case Linker::Module::ARM:
			linker_parameters["section_align"] = Linker::Location(4);
			break;
		default:
			linker_parameters["section_align"] = Linker::Location(1);
			break;
		}
	}

	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string GenericBinaryFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	/* Note:
	 * - CP/M-80, MS-DOS, DOS/65 etc. use .com
	 * - Human68k uses .r
	 * - RISC OS uses ,ff8 which is not an extension but an attribute suffix
	*/
	return filename + extension;
}

// BinaryFormat

/* * * General members * * */

void BinaryFormat::Clear()
{
	GenericBinaryFormat::Clear();
	/* format fields */
	pif = nullptr;
}

void BinaryFormat::ReadFile(Linker::Reader& rd)
{
	Clear();

	rd.endiantype = (::EndianType)0; /* does not matter */

	/* check for PIF structure at end */
	/* this is only meaningful for MS-DOS x86 binaries, but the DR PIFED signature is expected to be sufficiently distinguished */
	offset_t position = rd.Tell();
	rd.SeekEnd(-(relative_offset_t)Microsoft::MZFormat::PIF::SIZE);
	offset_t count = rd.Tell() - position;
	if(rd.ReadUnsigned(4, LittleEndian) == Microsoft::MZFormat::PIF::MAGIC_BEGIN)
	{
		pif = std::make_unique<Microsoft::MZFormat::PIF>();
		pif->ReadFile(rd);
		if(rd.ReadUnsigned(4, LittleEndian) != Microsoft::MZFormat::PIF::MAGIC_END)
		{
			/* failed */
			pif = nullptr;
		}
	}
	rd.Seek(position);

	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Buffer>();
	if(pif == nullptr)
		buffer->ReadFile(rd);
	else
		buffer->ReadFile(rd, count); /* skip PIF structure */
	image = buffer;
}

void BinaryFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = (::EndianType)0; /* does not matter */
	image->WriteFile(wr);
	if(pif)
		pif->WriteFile(wr);
}

void BinaryFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Binary format");

	Dumper::Block image_block("Image", file_offset, image, 0, 4);

	if(pif)
	{
		pif->Dump(dump, image->ActualDataSize());
	}

	image_block.Display(dump);
}

/* * * Writer members * * */
bool BinaryFormat::FormatSupportsSegmentation() const
{
	/* only x86 */
	return true;
}

bool BinaryFormat::FormatIs16bit() const
{
	/* only x86 */
	return true;
}

unsigned BinaryFormat::FormatAdditionalSectionFlags(std::string section_name) const
{
	if(section_name == ".stack" || section_name.rfind(".stack.", 0) == 0)
	{
		return Linker::Section::Stack;
	}
	else
	{
		return 0;
	}
}

void BinaryFormat::SetModel(std::string model)
{
	Linker::Debug << "Debug: setting memory model to `" << model << "'" << std::endl;
	if(model == "" || model == "default")
	{
		memory_model = MODEL_DEFAULT;
	}
	else if(model == "tiny")
	{
		memory_model = MODEL_TINY;
	}
	else if(model == "small")
	{
		memory_model = MODEL_SMALL;
	}
	else if(model == "compact")
	{
		memory_model = MODEL_COMPACT;
	}
	else if(model == "large")
	{
		memory_model = MODEL_LARGE;
	}
	else
	{
		Linker::Error << "Error: unsupported memory model" << std::endl;
	}
}

std::unique_ptr<Script::List> BinaryFormat::GetScript(Linker::Module& module)
{
	/* most targets */
	static const char * GenericScript = R"(
".code"
{
	at ?base_address?;
	all not zero align ?section_align?;
	all not ".stack" align ?section_align?;
	all align ?section_align?;
};
)";

	/* DOS memory models */
	static const char * TinyScript = R"(
".code"
{
	base here - ?segment_bias?;
	all not zero;
	all not ".stack";
	align 16;
	all;
	align 16;
};
)";

	static const char * SmallScript = R"(
".code"
{
	base here - ?segment_bias?;
	all exec;
	align 16;
	base here;
	all not zero;
	all not ".stack";
	align 16;
	all;
	align 16;
};
)";

	static const char * CompactScript = R"(
".code"
{
	base here - ?segment_bias?;
	all exec;
	all not zero and not ".data" and not ".rodata"
		align 16 base here;
	align 16;
	base here;
	all ".data" or ".rodata";
	align 16;
	all ".bss" or ".comm";
	all not ".stack" align 16 base here;
	all base here;
};
)";

	if(linker_script != "")
	{
		if(memory_model != MODEL_DEFAULT)
		{
			Linker::Warning << "Warning: linker script provided, overriding memory model" << std::endl;
		}
		return LinkerManager::GetScript(module);
	}
	else
	{
		switch(memory_model)
		{
		case MODEL_DEFAULT:
			switch(module.cpu)
			{
//					case Linker::Module::M68K:
//						return Script::parse_string(SimpleScript);
			case Linker::Module::I86:
				return Script::parse_string(TinyScript);
			default:
				return Script::parse_string(GenericScript);
			}
		case MODEL_TINY:
			return Script::parse_string(TinyScript);
		case MODEL_SMALL:
			return Script::parse_string(SmallScript);
		case MODEL_COMPACT:
			return Script::parse_string(CompactScript);
		case MODEL_LARGE:
			/* TODO */
			//return Script::parse_string(LargeScript);
			;
		}
	}
	assert(false);
}

void BinaryFormat::ProcessModule(Linker::Module& module)
{
	GenericBinaryFormat::ProcessModule(module);

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		if(stack_top.GetPosition().address != 0)
		{
			Linker::Warning << "Warning: stack top can't be assigned, ignoring" << std::endl;
		}
	}

	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		if(entry.GetPosition().address != base_address)
		{
			Linker::Error << "Error: entry point must be at beginning of image, ignoring" << std::endl; // TODO: not for subclasses
		}
	}
}

