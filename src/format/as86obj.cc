
#include "as86obj.h"
#include "../linker/buffer.h"

using namespace AS86Obj;

/* TODO: unimplemented */

static uint8_t as86_sizes[] = { 0, 1, 2, 4 };

AS86ObjFormat::ByteCode::~ByteCode()
{
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
		return std::make_unique<SkipBytes>(c & 0xF);
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
			uint32_t offset = rd.ReadUnsigned(as86_sizes[relocation_size]);
			return std::make_unique<SimpleRelocator>(offset, c);
		}
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		{
			uint16_t symbol_index = rd.ReadUnsigned((c & 4) != 0 ? 2 : 1);
			uint32_t offset = rd.ReadUnsigned(as86_sizes[c & 3]);
			return std::make_unique<SymbolRelocator>(offset, symbol_index, c);
		}
	default:
		Linker::FatalError("Fatal error: invalid bytecode in module data");
	}
}

AS86ObjFormat::Module::segment_size_list::offset::operator int() const
{
	return (word >> ((15 - (index ^ 3)) << 1)) & 3;
}

AS86ObjFormat::Module::segment_size_list::offset& AS86ObjFormat::Module::segment_size_list::offset::operator =(int value)
{
	int shift = (15 - (index ^ 3)) << 1;
	value &= 3;
	word = (word & ~(uint32_t(3) << shift)) | (value << shift);
	return *this;
}

int AS86ObjFormat::Module::segment_size_list::operator[](int index) const
{
	return (word >> ((15 - (index ^ 3)) << 1)) & 3;
}

AS86ObjFormat::Module::segment_size_list::offset AS86ObjFormat::Module::segment_size_list::operator[](int index)
{
	return offset { word, index };
}

AS86ObjFormat::Module::segment_size_list& AS86ObjFormat::Module::segment_size_list::operator =(uint32_t value)
{
	word = value;
	return *this;
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
			module.segment_sizes[j] = rd.ReadUnsigned(as86_sizes[module.segment_sizes_word[j]]);
		}
		uint16_t symbol_count = rd.ReadUnsigned(2);
		for(uint16_t j = 0; j < symbol_count; j++)
		{
			module.symbols.push_back(Symbol());
			Symbol& symbol = module.symbols.back();
			symbol.name_offset = rd.ReadUnsigned(2);
			symbol.symbol_type = rd.ReadUnsigned(2);
			symbol.offset_size = symbol.symbol_type >> 14;
			symbol.symbol_type &= 0x3FFF;
			symbol.offset = rd.ReadUnsigned(symbol.offset_size);
			module.symbols.push_back(symbol);
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
			module.data.push_back(std::move(bytecode));
		}
	}
}

void AS86ObjFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

void AS86ObjFormat::ProduceModule(Linker::Module& module, Linker::Reader& rd)
{
	/* TODO */
}

