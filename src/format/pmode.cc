
#include <array>
#include "pmode.h"
#include "mzexe.h"
#include "../dumper/dumper.h"
#include "../linker/buffer.h"
#include "../linker/location.h"

/* TODO: unimplemented */

using namespace PMODE;

void PMW1Format::CompressedReader::Start(std::shared_ptr<Linker::Image> image)
{
	using_reader = false;
	source_image = image;
	image_offset = 0;
	uint8_t byte = ReadNextDataByte();
	AddBytes(1, &byte);
}

void PMW1Format::CompressedReader::Start(Linker::Reader * reader)
{
	using_reader = true;
	source_reader = reader;
	uint8_t byte = ReadNextDataByte();
	AddBytes(1, &byte);
}

bool PMW1Format::CompressedReader::IsDataBufferEmpty()
{
	return data_buffer_position == (data_buffer_offset + data_buffer_length) % DataBufferSize;
}

void PMW1Format::CompressedReader::ReadBytes(size_t offset, size_t count, uint8_t * data)
{
	//Linker::Debug << "Debug: Read 0x" << std::hex << count << " bytes from offset 0x" << std::hex << offset << " from buffer of size 0x" << std::hex << data_buffer_length << std::endl;
	if(data_buffer_length < offset)
	{
		Linker::Error << "Fatal error: Buffer underflow during PMW1 decompression" << std::endl;

		size_t skip = offset - data_buffer_length;
		if(skip < count)
		{
			memset(data, 0, skip);
			offset += skip;
			count -= skip;
			data += skip;
		}
		else
		{
			memset(data, 0, count);
			return;
		}
	}

	size_t start_offset = (data_buffer_offset + data_buffer_length - offset) % DataBufferSize;
	size_t end_offset = start_offset + count;
	if(end_offset <= DataBufferSize)
	{
		memcpy(data, data_buffer.data() + start_offset, count);
	}
	else
	{
		size_t actual_count = DataBufferSize - start_offset;
		memcpy(data,                data_buffer.data() + start_offset, actual_count);
		memcpy(data + actual_count, data_buffer.data(),                count - actual_count);
	}
}

void PMW1Format::CompressedReader::AddBytes(size_t count, uint8_t * data)
{
//	Linker::Debug << "Debug: offset=0x" << std::hex << data_buffer_offset << ", length=0x" << std::hex << data_buffer_length << std::endl;
//	Linker::Debug << "Debug: count=0x" << std::hex << count << std::endl;

	if(data_buffer_length + count > DataBufferSize)
	{
		data_buffer_offset = (data_buffer_offset + data_buffer_length + count) % DataBufferSize;
		data_buffer_length = DataBufferSize;
	}
	else
	{
		data_buffer_length += count;
	}
//	Linker::Debug << "Debug: offset=0x" << std::hex << data_buffer_offset << ", length=0x" << std::hex << data_buffer_length << std::endl;

	size_t start_offset = (data_buffer_offset + data_buffer_length - count) % DataBufferSize;
	size_t end_offset = start_offset + count;
	if(end_offset <= DataBufferSize)
	{
		memcpy(data_buffer.data() + start_offset, data, count);
	}
	else
	{
		size_t actual_count = DataBufferSize - start_offset;
//		Linker::Debug << "Debug: start_offset=0x" << std::hex << start_offset << ", end_offset=0x" << std::hex << end_offset << ", actual_count=0x" << std::hex << actual_count << std::endl;
		memcpy(data_buffer.data() + start_offset, data,                actual_count);
		memcpy(data_buffer.data(),                data + actual_count, count - actual_count);
	}

//	Linker::Debug << "Debug: offset=0x" << std::hex << data_buffer_offset << ", length=0x" << std::hex << data_buffer_length << std::endl;
}

uint32_t PMW1Format::CompressedReader::ReadNextUnsigned(size_t count)
{
	if(using_reader)
	{
		return source_reader->ReadUnsigned(count, ::LittleEndian);
	}
	else
	{
		uint32_t value = source_image->ReadUnsigned(count, image_offset, ::LittleEndian);
		image_offset += count;
		return value;
	}
}

uint32_t PMW1Format::CompressedReader::ReadNextControlBits(size_t count)
{
	uint32_t result;

	if(control_word_size >= count)
	{
		result = control_word_buffer >> (control_word_size - count);
		control_word_size -= count;
	}
	else
	{
		result = control_word_buffer << (count - control_word_size);

		control_word_buffer = ReadNextUnsigned(4);
		//Linker::Debug << "Debug: Next control bits 0x" << std::hex << control_word_buffer << " (using " << std::dec << int(count - control_word_size) << " of this word)" << std::endl;

		result |= control_word_buffer >> (32 - count + control_word_size);
		control_word_size += 32 - count;
	}

	return result & ((1 << count) - 1);
}

uint8_t PMW1Format::CompressedReader::ReadNextDataByte()
{
	uint8_t byte = ReadNextUnsigned(1);
	//Linker::Debug << "Debug: Next byte 0x" << std::hex << int(byte) << std::endl;
	return byte;
}

void PMW1Format::CompressedReader::GenerateNextBytes()
{
	if(ReadNextControlBits(1) == 0)
	{
		//Linker::Debug << "Debug: Control bit 0" << std::endl;
		uint8_t byte = ReadNextDataByte();
		AddBytes(1, &byte);
	}
	else
	{
		uint8_t byte = ReadNextDataByte();
		size_t count = 0;
		size_t offset;
		switch(ReadNextControlBits(2))
		{
		default:
			assert(false);
		case 0b00:
			//Linker::Debug << "Debug: Control bit 100" << std::endl;
			if(byte == 0)
				return; // end of stream
			offset = byte;
			count = 2;
			break;
		case 0b01:
			offset = byte + (ReadNextControlBits(3) << 8) + 1;
			//Linker::Debug << "Debug: Control bit 101???" << std::endl;
			count = 2;
			break;
		case 0b10:
			//Linker::Debug << "Debug: Control bit 110" << std::endl;
			offset = byte + 1;
			break;
		case 0b11:
			if(ReadNextControlBits(1) == 0)
			{
				offset = byte + (ReadNextControlBits(2) << 8) + 1;
				//Linker::Debug << "Debug: Control bit 1110??" << std::endl;
			}
			else
			{
				offset = byte + (ReadNextControlBits(4) << 8) + 1;
				//Linker::Debug << "Debug: Control bit 1111????" << std::endl;
			}
			break;
		}

		if(count == 0)
		{
			if(ReadNextControlBits(1) == 1)
			{
				//Linker::Debug << "Debug: Control bit 1" << std::endl;
				count = 3;
			}
			else if(ReadNextControlBits(1) == 1)
			{
				//Linker::Debug << "Debug: Control bit 01" << std::endl;
				count = 4;
			}
			else if(ReadNextControlBits(1) == 1)
			{
				//Linker::Debug << "Debug: Control bit 001" << std::endl;
				count = 5;
			}
			else if(ReadNextControlBits(1) == 1)
			{
				//Linker::Debug << "Debug: Control bit 0001" << std::endl;
				count = 6;
			}
			else if(ReadNextControlBits(1) == 1)
			{
				//Linker::Debug << "Debug: Control bit 00001" << std::endl;
				count = 15 + ReadNextDataByte();
			}
			else
			{
				count = 7 + ReadNextControlBits(3);
				//Linker::Debug << "Debug: Control bit 00000???" << std::endl;
			}
		}

		static std::array<uint8_t, 255 + 15> tmp;

		while(count > 0)
		{
			size_t actual_count = std::min(count, offset);
			ReadBytes(offset, actual_count, tmp.data());
			AddBytes(actual_count, tmp.data());
			count -= actual_count;
		}
	}
}

bool PMW1Format::CompressedReader::GetNextByte(uint8_t& result)
{
	if(IsDataBufferEmpty())
	{
		GenerateNextBytes();
	}

	if(!IsDataBufferEmpty())
	{
		result = data_buffer[data_buffer_position];
		data_buffer_position = (data_buffer_position + 1) % DataBufferSize;
		return true;
	}
	else
	{
		return false;
	}
}

uint32_t PMW1Format::CompressedReader::GetNextUnsigned(size_t count)
{
	std::vector<uint8_t> bytes;
	for(size_t position = 0; position < count; position++)
	{
		uint8_t byte;
		if(!GetNextByte(byte))
		{
			// underflow
			return 0;
		}
		bytes.push_back(byte);
	}

	return ::ReadUnsigned(count, count, bytes.data(), ::LittleEndian);
}

void PMW1Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	std::array<char, 4> signature;
	file_offset = Microsoft::FindActualSignature(rd, signature, "PMW1");
	version.major = rd.ReadUnsigned(1);
	version.minor = rd.ReadUnsigned(1);
	flags = rd.ReadUnsigned(2);
	eip_object = rd.ReadUnsigned(4);
	eip = rd.ReadUnsigned(4);
	esp_object = rd.ReadUnsigned(4);
	esp = rd.ReadUnsigned(4);
	object_table_offset = rd.ReadUnsigned(4);
	uint32_t object_count = rd.ReadUnsigned(4);
	relocation_table_offset = rd.ReadUnsigned(4);
	data_offset = rd.ReadUnsigned(4);

	rd.Seek(file_offset + object_table_offset);
	unsigned i;
	for(i = 0; i < object_count; i++)
	{
		Object object;
		object.memory_size = rd.ReadUnsigned(4);
		object.file_size = rd.ReadUnsigned(4);
		object.flags = rd.ReadUnsigned(4);
		object.relocation_offset = rd.ReadUnsigned(4);
		object.relocation_block_count = rd.ReadUnsigned(4);
		object.image_size = rd.ReadUnsigned(4);
		objects.push_back(object);
	}

	for(auto& object : objects)
	{
		rd.Seek(file_offset + relocation_table_offset + object.relocation_offset);
		for(i = 0; i < object.relocation_block_count; i++)
		{
			uint16_t stored_block_size = rd.ReadUnsigned(2);
			uint16_t uncompressed_block_size = rd.ReadUnsigned(2);
			if((flags & 1) == 0 /*|| true*/)
			{
				for(uint16_t j = 0; j < stored_block_size; j += 10)
				{
					Object::Relocation rel;
					rel.type = rd.ReadUnsigned(1);
					rel.source = rd.ReadUnsigned(4);
					rel.target_object = rd.ReadUnsigned(1);
					rel.target_offset = rd.ReadUnsigned(4);
					object.relocations.push_back(rel);
				}
			}
			else
			{
				offset_t block_end = rd.Tell() + stored_block_size;
				CompressedReader crd;
				Linker::Debug << "Debug: Starting PMW1 decompression" << std::endl;
				crd.Start(&rd);
				for(uint16_t j = 0; j < uncompressed_block_size; j += 10)
				{
					Object::Relocation rel;
					rel.type = crd.GetNextUnsigned(1);
					rel.source = crd.GetNextUnsigned(4);
					rel.target_object = crd.GetNextUnsigned(1);
					rel.target_offset = crd.GetNextUnsigned(4);
					object.relocations.push_back(rel);
				}
				rd.Seek(block_end);
			}
		}
	}

	rd.Seek(file_offset + data_offset);
	for(auto& object : objects)
	{
		object.image = Linker::Buffer::ReadFromFile(rd, object.file_size);
		if((flags & 1) != 0 /*&& false*/)
		{
			CompressedReader crd;
			Linker::Debug << "Debug: Starting PMW1 decompression" << std::endl;
			crd.Start(object.image->AsImage());
			auto decompressed_image = std::make_shared<Linker::Buffer>();
			while(decompressed_image->ImageSize() < object.image_size)
			{
				std::vector<uint8_t> byte_buffer(1);
				if(!crd.GetNextByte(byte_buffer[0]))
				{
					Linker::Error << "Error: Ran out of compressed buffer" << std::endl;
					break;
				}
				decompressed_image->Append(byte_buffer);
			}
			object.decompressed_image = decompressed_image;
		}
	}
}

offset_t PMW1Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	wr.WriteData("PMW1");
	wr.WriteWord(1, version.major);
	wr.WriteWord(1, version.minor);
	wr.WriteWord(2, flags);
	wr.WriteWord(4, eip_object);
	wr.WriteWord(4, eip);
	wr.WriteWord(4, esp_object);
	wr.WriteWord(4, esp);
	wr.WriteWord(4, object_table_offset);
	wr.WriteWord(4, objects.size());
	wr.WriteWord(4, relocation_table_offset);
	wr.WriteWord(4, data_offset);

	wr.Seek(file_offset + object_table_offset);
	for(auto& object : objects)
	{
		wr.WriteWord(4, object.memory_size);
		wr.WriteWord(4, object.file_size);
		wr.WriteWord(4, object.flags);
		wr.WriteWord(4, object.relocation_offset);
		wr.WriteWord(4, object.relocation_block_count);
		wr.WriteWord(4, object.image_size);
	}

	for(auto& object : objects)
	{
		wr.Seek(file_offset + relocation_table_offset + object.relocation_offset);
		for(auto& rel : object.relocations)
		{
			wr.WriteWord(1, rel.type);
			wr.WriteWord(4, rel.source);
			wr.WriteWord(1, rel.target_object);
			wr.WriteWord(4, rel.target_offset);
		}
	}

	wr.Seek(file_offset + data_offset);
	for(auto& object : objects)
	{
		object.image->WriteFile(wr);
	}

	return offset_t(-1);
}

void PMW1Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("PMW1 format");

	Dumper::Region file_region("File", file_offset, 0 /* TODO */, 8);
	file_region.AddField("Version", Dumper::VersionDisplay::Make(), offset_t(version.major), offset_t(version.minor));
	file_region.AddField("Flags",
		Dumper::BitFieldDisplay::Make(4)
			->AddBitField(0, 1, Dumper::ChoiceDisplay::Make("compressed"), true),
		offset_t(flags));
	file_region.AddField("EIP", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(eip_object), offset_t(eip));
	file_region.AddField("ESP", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(esp_object), offset_t(esp));
	file_region.AddField("Relocations offset", Dumper::HexDisplay::Make(8), offset_t(relocation_table_offset));
	file_region.Display(dump, Dumper::Header);

	Dumper::Region object_table_region("Object table", file_offset + object_table_offset, 24 * objects.size(), 8);
	object_table_region.Display(dump, Dumper::Header);

	unsigned i = 0;
	offset_t object_offset = file_offset + data_offset;
	for(auto& object : objects)
	{
		std::shared_ptr<Dumper::Region> object_region;
		std::shared_ptr<Dumper::Block> object_block;
		if((flags & 1) != 0 /*&& false*/)
		{
			// compressed
			object_region = std::make_shared<Dumper::Region>("Object", object_offset, object.image_size, 8);
			object_block = nullptr;
		}
		else
		{
			object_region = object_block = std::make_shared<Dumper::Block>("Object", object_offset, object.image->AsImage(), 0, 8);
		}

		object_region->InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		object_region->AddField("Size in memory", Dumper::HexDisplay::Make(8), offset_t(object.memory_size));
		object_region->AddField("Size in image", Dumper::HexDisplay::Make(8), offset_t(object.image_size));
		object_region->AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(object.flags));
		object_region->AddOptionalField("Relocation block count", Dumper::DecDisplay::Make(), offset_t(object.relocation_block_count));
		object_region->AddOptionalField("Relocation offset", Dumper::HexDisplay::Make(8), offset_t(object.relocation_offset));

		if((flags & 1) != 0 /*&& false*/)
		{
			// compressed
			object_region->Display(dump, Dumper::Header | Dumper::Image | Dumper::Relocation);
			object_block = std::make_shared<Dumper::Block>("Object", 0, object.decompressed_image->AsImage(), 0, 8);
		}

#if 0
		for(auto& rel : object.relocations)
		{
			object_block->AddSignal(rel.source, 1); // TODO: size
		}
#endif

		object_block->Display(dump, Dumper::Header | Dumper::Image | Dumper::Relocation);

		unsigned j = 0;
		for(auto& rel : object.relocations)
		{
			Dumper::Entry relocation_entry("Relocation", j + 1, file_offset + relocation_table_offset + object.relocation_offset + j * 10, 8);
			relocation_entry.AddField("Type", Dumper::HexDisplay::Make(2), offset_t(rel.type));
			relocation_entry.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(rel.source));
			relocation_entry.AddField("Target", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(rel.target_object), offset_t(rel.target_offset));
			// TODO: addend
			relocation_entry.Display(dump, Dumper::Relocation);
			j++;
		}

		object_offset += object.file_size;
		i++;
	}
}

void PMW1Format::CalculateValues()
{
	// TODO
}

std::string PMW1Format::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".exe";
}

