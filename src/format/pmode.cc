
#include "pmode.h"
#include "../dumper/dumper.h"

/* TODO: unimplemented */

using namespace PMODE;

void PMW1Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	rd.Skip(4); // "PMW1"
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
		object.relocation_count = rd.ReadUnsigned(4);
		object.image_size = rd.ReadUnsigned(4);
		objects.push_back(object);
	}

	for(auto& object : objects)
	{
		rd.Seek(file_offset + relocation_table_offset + object.relocation_offset);
		for(i = 0; i < object.relocation_count; i++)
		{
			Object::Relocation rel;
			rel.type = rd.ReadUnsigned(1);
			rel.source = rd.ReadUnsigned(4);
			rel.target_object = rd.ReadUnsigned(1);
			rel.target_offset = rd.ReadUnsigned(4);
			object.relocations.push_back(rel);
		}
	}

	rd.Seek(file_offset + data_offset);
	for(auto& object : objects)
	{
		object.image = Linker::Buffer::ReadFromFile(rd, object.file_size);
	}
}

offset_t PMW1Format::WriteFile(Linker::Writer& wr)
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
		wr.WriteWord(4, object.relocation_count);
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

void PMW1Format::Dump(Dumper::Dumper& dump)
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
	file_region.Display(dump);

	Dumper::Region object_table_region("Object table", file_offset + object_table_offset, 24 * objects.size(), 8);
	object_table_region.Display(dump);

	unsigned i = 0;
	offset_t object_offset = file_offset + data_offset;
	for(auto& object : objects)
	{
		// TODO: decompress
		Dumper::Block object_block("Object", object_offset, object.image->AsImage(), 0, 8);
		object_block.InsertField(0, "Number", Dumper::DecDisplay::Make(), offset_t(i + 1));
		object_block.AddField("Size in memory", Dumper::HexDisplay::Make(8), offset_t(object.memory_size));
		object_block.AddField("Size in image", Dumper::HexDisplay::Make(8), offset_t(object.image_size));
		object_block.AddField("Flags", Dumper::HexDisplay::Make(8), offset_t(object.flags));

		// TODO: relocations
		/*for(auto& rel : object.relocations)
		{
			object_block.AddSignal(rel.offset, 1);
			j++;
		}*/

		object_block.Display(dump);

		unsigned j = 0;
		for(auto& rel : object.relocations)
		{
			Dumper::Entry relocation_entry("Relocation", j + 1, file_offset + object.relocation_offset + i * 10, 8);
			relocation_entry.AddField("Type", Dumper::HexDisplay::Make(4), offset_t(rel.type));
			relocation_entry.AddField("Source", Dumper::HexDisplay::Make(8), offset_t(rel.source));
			relocation_entry.AddField("Target", Dumper::SectionedDisplay<offset_t>::Make(Dumper::HexDisplay::Make(8)), offset_t(rel.target_object), offset_t(rel.target_offset));
			// TODO: addend
			relocation_entry.Display(dump);
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

std::string PMW1Format::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

