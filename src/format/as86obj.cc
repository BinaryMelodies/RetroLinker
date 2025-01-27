
#include "as86obj.h"
#include "../linker/buffer.h"
#include "../linker/location.h"
#include "../linker/module.h"
#include "../linker/section.h"
#include "../linker/target.h"

using namespace AS86Obj;

/* TODO: not fully implemented */

AS86ObjFormat::ByteCode::~ByteCode()
{
}

offset_t AS86ObjFormat::ByteCode::GetLength() const
{
	// This is a common enough behavior to place into the superclass
	return 1;
}

void AS86ObjFormat::ByteCode::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
}

offset_t AS86ObjFormat::RelocatorSize::GetMemorySize() const
{
	return 0;
}

void AS86ObjFormat::RelocatorSize::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Entry entry("Relocation size", index, file_offset, 8);
	entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(size));
	entry.Display(dump);
}

offset_t AS86ObjFormat::SkipBytes::GetMemorySize() const
{
	return count;
}

void AS86ObjFormat::SkipBytes::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Entry entry("Skip bytes", index, file_offset, 8);
	entry.AddField("Count", Dumper::HexDisplay::Make(), offset_t(count));
	entry.Display(dump);
}

void AS86ObjFormat::SkipBytes::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
	segments[current_segment]->Expand(segments[current_segment]->Size() + count);
}

offset_t AS86ObjFormat::ChangeSegment::GetMemorySize() const
{
	// This actually resets the offset pointer
	return 0;
}

void AS86ObjFormat::ChangeSegment::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Entry entry("Change segment", index, file_offset, 8);
	entry.AddField("Segment", Dumper::DecDisplay::Make(), offset_t(segment));
	entry.Display(dump);
}

void AS86ObjFormat::ChangeSegment::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
	current_segment = segment;
}

offset_t AS86ObjFormat::RawBytes::GetLength() const
{
	return 1 + buffer->ImageSize();
}

offset_t AS86ObjFormat::RawBytes::GetMemorySize() const
{
	return buffer->ImageSize();
}

void AS86ObjFormat::RawBytes::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Block block("Raw bytes",file_offset, buffer, memory_offset, 8);
	block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(index));
	block.Display(dump);
}

void AS86ObjFormat::RawBytes::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
	segments[current_segment]->SetZeroFilled(false);
	segments[current_segment]->Append(*buffer);
}

offset_t AS86ObjFormat::SimpleRelocator::GetLength() const
{
	return 1 + relocation_size;
}

offset_t AS86ObjFormat::SimpleRelocator::GetMemorySize() const
{
	return relocation_size;
}

void AS86ObjFormat::SimpleRelocator::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Entry entry("Relocation", index, file_offset, 8);
	entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation_size));
	entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(offset));
	entry.AddField("Segment", Dumper::DecDisplay::Make(), offset_t(segment));
	entry.AddOptionalField("IP relative", Dumper::ChoiceDisplay::Make("true"), offset_t(ip_relative));
	entry.Display(dump);
}

void AS86ObjFormat::SimpleRelocator::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
	Linker::Location rel_source = Linker::Location(segments[current_segment], segments[current_segment]->Size());
	segments[current_segment]->Expand(segments[current_segment]->Size() + relocation_size);
	if(segments[segment] == nullptr)
	{
		segments[segment] = GetDefaultSection(segment);
	}
	Linker::Target rel_target = Linker::Target(Linker::Location(segments[segment], offset));
	Linker::Relocation rel = Linker::Relocation::Empty();
	if(ip_relative)
		rel = Linker::Relocation::Relative(relocation_size, rel_source, rel_target, -relocation_size, ::LittleEndian);
	else
		rel = Linker::Relocation::Offset(relocation_size, rel_source, rel_target, 0, ::LittleEndian);
	module.AddRelocation(rel);
}

offset_t AS86ObjFormat::SymbolRelocator::GetLength() const
{
	return 1 + index_size + offset_size;
}

offset_t AS86ObjFormat::SymbolRelocator::GetMemorySize() const
{
	return relocation_size;
}

void AS86ObjFormat::SymbolRelocator::Dump(Dumper::Dumper& dump, unsigned index, offset_t& file_offset, offset_t& memory_offset) const
{
	Dumper::Entry entry("Relocation", index, file_offset, 8);
	entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(relocation_size));
	entry.AddField("Offset", Dumper::HexDisplay::Make(8), offset_t(offset));
	entry.AddField("Offset size", Dumper::DecDisplay::Make(), offset_t(offset_size));
	entry.AddField("Symbol index", Dumper::HexDisplay::Make(4), offset_t(symbol_index));
	entry.AddField("Symbol name", Dumper::StringDisplay::Make(), symbol_name);
	entry.AddField("Symbol index size", Dumper::DecDisplay::Make(), offset_t(index_size));
	entry.AddOptionalField("IP relative", Dumper::ChoiceDisplay::Make("true"), offset_t(ip_relative));
	entry.Display(dump);
}

void AS86ObjFormat::SymbolRelocator::Generate(Linker::Module& module, int& current_segment, std::array<std::shared_ptr<Linker::Section>, 16>& segments) const
{
	Linker::Location rel_source = Linker::Location(segments[current_segment], segments[current_segment]->Size());
	segments[current_segment]->Expand(segments[current_segment]->Size() + relocation_size);
	Linker::Target rel_target = Linker::Target(Linker::SymbolName(symbol_name));
//Linker::Debug << "Debug: symbol " << symbol_name << std::endl;
	Linker::Relocation rel = Linker::Relocation::Empty();
	if(ip_relative)
		rel = Linker::Relocation::Relative(relocation_size, rel_source, rel_target, offset - relocation_size, ::LittleEndian);
	else
		rel = Linker::Relocation::Offset(relocation_size, rel_source, rel_target, offset, ::LittleEndian);
	module.AddRelocation(rel);
}

std::unique_ptr<AS86ObjFormat::ByteCode> AS86ObjFormat::ByteCode::ReadFile(Linker::Reader& rd, int& relocation_size)
{
	int c = rd.ReadUnsigned(1);
	switch(c >> 4)
	{
	case 0x0:
		{
			if(c == 0)
				return nullptr;
			std::unique_ptr<RelocatorSize> bytecode = std::make_unique<RelocatorSize>(c & 0xF);
			relocation_size = bytecode->size;
			return bytecode;
		}
	case 0x1:
		return std::make_unique<SkipBytes>(rd.ReadUnsigned(GetSize(c & 3)));
	case 0x2:
		return std::make_unique<ChangeSegment>(c & 0xF);
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		{
			uint8_t count = c & 0x3F;
			if(count == 0)
			{
				count = 64;
			}
			std::unique_ptr<RawBytes> bytecode = std::make_unique<RawBytes>();
			bytecode->buffer = std::make_unique<Linker::Buffer>();
			bytecode->buffer->ReadFile(rd, count);
			return bytecode;
		}
	case 0x8:
	case 0x9:
	case 0xA:
	case 0xB:
		{
			uint32_t offset = rd.ReadUnsigned(GetSize(relocation_size));
			return std::make_unique<SimpleRelocator>(c, offset, GetSize(relocation_size));
		}
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		{
			int index_size = (c & 4) != 0 ? 2 : 1;
			uint16_t symbol_index = rd.ReadUnsigned(index_size);
//Linker::Debug << "Debug: symbol index " << symbol_index << std::endl;
			uint32_t offset = rd.ReadUnsigned(GetSize(c & 3));
			return std::make_unique<SymbolRelocator>(c, offset, symbol_index, GetSize(relocation_size), GetSize(c & 3), index_size);
		}
	default:
		Linker::FatalError("Fatal error: invalid bytecode in module data");
	}
}

AS86ObjFormat::segment_size_list::offset::operator int() const
{
	return (word >> ((15 - (index ^ 3)) << 1)) & 3;
}

AS86ObjFormat::segment_size_list::offset& AS86ObjFormat::segment_size_list::offset::operator =(int value)
{
	int shift = (15 - (index ^ 3)) << 1;
	value &= 3;
	word = (word & ~(uint32_t(3) << shift)) | (value << shift);
	return *this;
}

int AS86ObjFormat::segment_size_list::operator[](int index) const
{
	return (word >> ((15 - (index ^ 3)) << 1)) & 3;
}

AS86ObjFormat::segment_size_list::offset AS86ObjFormat::segment_size_list::operator[](int index)
{
	return offset { word, index };
}

AS86ObjFormat::segment_size_list& AS86ObjFormat::segment_size_list::operator =(uint32_t value)
{
	word = value;
	return *this;
}

void AS86ObjFormat::Module::Dump(Dumper::Dumper& dump, unsigned index)
{
	Dumper::Region module_region("Module", file_offset, module_size, 8);
	module_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(index + 1));
	module_region.AddField("Module name", Dumper::StringDisplay::Make(), module_name);
	module_region.AddField("Code offset", Dumper::HexDisplay::Make(8), offset_t(code_offset));
	module_region.AddField("Image size", Dumper::HexDisplay::Make(8), offset_t(image_size));
	module_region.AddField("Module version", Dumper::VersionDisplay::Make(), offset_t(module_version.major), offset_t(module_version.minor));
	module_region.AddField("Maximum segment size", Dumper::HexDisplay::Make(8), offset_t(maximum_segment_size));
	for(int i = 0; i < 16; i++)
	{
		int size_len = GetSize(segment_sizes_word[i]);
		if(size_len != 0 || segment_sizes[i] != 0)
		{
			{
				std::ostringstream oss;
				oss << "Segment #" << i << " size length";
				module_region.AddField(oss.str(), Dumper::DecDisplay::Make(), offset_t(size_len));
			}

			{
				std::ostringstream oss;
				oss << "Segment #" << i << " size";
				module_region.AddField(oss.str(), Dumper::HexDisplay::Make(4), offset_t(segment_sizes[i]));
			}
		}
	}

	module_region.Display(dump);

	Dumper::Region string_table_region("String table region", string_table_offset, string_table_size, 8);
	string_table_region.Display(dump);

	unsigned i = 0;
	for(auto& symbol : symbols)
	{
		Dumper::Entry symbol_entry("Symbol", i, symbol.symbol_definition_offset, 8);
		symbol_entry.AddField("Name", Dumper::StringDisplay::Make(), symbol.name);
		symbol_entry.AddField("Offset to name", Dumper::HexDisplay::Make(4), offset_t(symbol.name_offset));
		symbol_entry.AddField("Symbol type",
			Dumper::BitFieldDisplay::Make(4)
				->AddBitField(4, 1, Dumper::ChoiceDisplay::Make("Absolute"), true)
				->AddBitField(5, 1, Dumper::ChoiceDisplay::Make("Relative"), true)
				->AddBitField(6, 1, Dumper::ChoiceDisplay::Make("Imported"), true)
				->AddBitField(7, 1, Dumper::ChoiceDisplay::Make("Exported"), true)
				->AddBitField(8, 1, Dumper::ChoiceDisplay::Make("Entry point"), true)
				->AddBitField(13, 1, Dumper::ChoiceDisplay::Make("Common symbol"), true),
			offset_t(symbol.symbol_type));
		if((symbol.symbol_type & Symbol::Imported) != 0 && symbol.segment == 0xF && symbol.offset_size == 0)
		{
			symbol_entry.AddField("Offset", Dumper::ChoiceDisplay::Make("Undefined"), offset_t(true));
		}
		else if((symbol.symbol_type & Symbol::Common) != 0)
		{
			symbol_entry.AddField("Segment", Dumper::DecDisplay::Make(), offset_t(symbol.segment));
			symbol_entry.AddField("Size", Dumper::DecDisplay::Make(), offset_t(symbol.offset));
			symbol_entry.AddField("Size size", Dumper::DecDisplay::Make(), offset_t(symbol.offset_size));
		}
		else
		{
			symbol_entry.AddField("Offset", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(symbol.segment), offset_t(symbol.offset));
			symbol_entry.AddField("Offset size", Dumper::DecDisplay::Make(), offset_t(symbol.offset_size));
		}
		symbol_entry.Display(dump);
		i ++;
	}

	i = 0;
	offset_t file_offset = string_table_offset + string_table_size;
	int current_segment = 0;
	offset_t memory_offsets[16] = { };
	for(auto& bytecode : data)
	{
		bytecode->Dump(dump, i, file_offset, memory_offsets[current_segment]);
		file_offset += bytecode->GetLength();
		memory_offsets[current_segment] += bytecode->GetMemorySize();
		if(ChangeSegment * change = dynamic_cast<ChangeSegment *>(bytecode.get()))
		{
			current_segment = change->segment;
		}
		i ++;
	}
}

void AS86ObjFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	if(rd.ReadUnsigned(2) != 0x86A3)
	{
		Linker::FatalError("Fatal error: invalid file signature");
	}
	uint16_t module_count = rd.ReadUnsigned(2);
	rd.Skip(1); // checksum

	for(uint16_t i = 0; i < module_count; i++)
	{
		modules.push_back(Module());
		Module& module = modules.back();
		module.file_offset = rd.Tell();
		module.code_offset = rd.ReadUnsigned(4);
		module.image_size = rd.ReadUnsigned(4);
		module.string_table_size = rd.ReadUnsigned(2);
		module.module_version.major = rd.ReadUnsigned(1);
		module.module_version.minor = rd.ReadUnsigned(1);
		module.maximum_segment_size = rd.ReadUnsigned(4);
		module.segment_sizes_word = rd.ReadUnsigned(4);
		for(int j = 0; j < 16; j++)
		{
			module.segment_sizes[j] = rd.ReadUnsigned(GetSize(module.segment_sizes_word[j]));
		}
		uint16_t symbol_count = rd.ReadUnsigned(2);
		for(uint16_t j = 0; j < symbol_count; j++)
		{
			module.symbols.push_back(Symbol());
			Symbol& symbol = module.symbols.back();
			symbol.symbol_definition_offset = rd.Tell();
			symbol.name_offset = rd.ReadUnsigned(2);
			symbol.symbol_type = rd.ReadUnsigned(2);
			symbol.offset_size = GetSize(symbol.symbol_type >> 14);
			symbol.segment = symbol.symbol_type & 0xF;
			symbol.symbol_type &= 0x3FF0;
			symbol.offset = rd.ReadUnsigned(symbol.offset_size);
		}
		module.string_table_offset = rd.Tell();
		module.module_name = rd.ReadASCIIZ();
		for(auto& symbol : module.symbols)
		{
			rd.Seek(module.string_table_offset + symbol.name_offset);
			symbol.name = rd.ReadASCIIZ();
		}
		// TODO: read entire symbol table separately?
		rd.Seek(module.string_table_offset + module.string_table_size);
		int relocation_size = 0;
		while(true)
		{
			std::unique_ptr<ByteCode> bytecode = ByteCode::ReadFile(rd, relocation_size);
			if(bytecode == nullptr)
			{
				break;
			}
			if(SymbolRelocator * relocation = dynamic_cast<SymbolRelocator *>(bytecode.get()))
			{
//Linker::Debug << "Debug: relocator " << relocation->symbol_index << std::endl;
				relocation->symbol_name = module.symbols[relocation->symbol_index].name;
			}
			module.data.push_back(std::move(bytecode));
		}
		module.module_size = rd.Tell() - module.file_offset;
	}
}

offset_t AS86ObjFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
	return offset_t(-1);
}

void AS86ObjFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("as86 format");

	Dumper::Region file_region("File", 0, modules.back().file_offset + modules.back().module_size, 8);
	file_region.Display(dump);

	unsigned i = 0;
	for(auto& module : modules)
	{
		module.Dump(dump, i);
		i++;
	}
}

std::shared_ptr<Linker::Section> AS86ObjFormat::GetDefaultSection(unsigned index, std::string name)
{
	if(name == "")
	{
		std::ostringstream oss;
		oss << "." << index;
		name = oss.str();
	}
	int flags;
	if(index == 0 || index > 3)
	{
		flags = Linker::Section::Readable | Linker::Section::Execable;
	}
	else
	{
		flags = Linker::Section::Readable | Linker::Section::Writable;
	}
	std::shared_ptr<Linker::Section> section = std::make_shared<Linker::Section>(name, flags);
	section->SetAlign(4);
	return section;
}

void AS86ObjFormat::GenerateModule(Linker::Module& module) const
{
	module.cpu = Linker::Module::I86; // TODO: I386?
	std::array<std::shared_ptr<Linker::Section>, 16> segments;
	segments[0] = GetDefaultSection(0, ".text");
	segments[3] = GetDefaultSection(3, ".data");
	for(auto& objmod : modules)
	{
		for(auto& symbol : objmod.symbols)
		{
			if((symbol.symbol_type & Symbol::Imported) != 0)
			{
				module.AddUndefinedSymbol(symbol.name);
			}
			else if((symbol.symbol_type & Symbol::Common) != 0)
			{
				module.AddCommonSymbol(symbol.name, Linker::CommonSymbol(symbol.name, symbol.offset, 1)); // TODO: third field?
			}
			else
			{
				std::shared_ptr<Linker::Section> section;
				if((symbol.symbol_type & Symbol::Absolute) != 0)
				{
					section = nullptr;
				}
				else
				{
					if(segments[symbol.segment] == nullptr)
					{
						segments[symbol.segment] = GetDefaultSection(symbol.segment);
					}
					section = segments[symbol.segment];
				}
				Linker::Location location = Linker::Location(section, symbol.offset);
				if((symbol.symbol_type & Symbol::Exported) != 0)
					module.AddGlobalSymbol(symbol.name, location);
				else
					module.AddLocalSymbol(symbol.name, location);
			}
		}

		int current_segment = 0;
		for(auto& bytecode : objmod.data)
		{
			bytecode->Generate(module, current_segment, segments);
			if(segments[current_segment] == nullptr)
			{
				segments[current_segment] = GetDefaultSection(current_segment);
			}
		}
	}
	for(unsigned i = 0; i < 16; i++)
	{
		if(segments[i] != nullptr)
		{
			module.AddSection(segments[i]);
		}
	}
}

