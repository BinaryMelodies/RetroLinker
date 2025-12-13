
#include "w3w4.h"

using namespace Microsoft;

// W3Format

void W3Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	file_end = rd.GetImageEnd();
	rd.Skip(2); // "W3"
	system_version.minor = rd.ReadUnsigned(1);
	system_version.major = rd.ReadUnsigned(1);
	uint16_t entry_count = rd.ReadUnsigned(2);
	rd.Skip(10);
	for(uint16_t entry_index = 0; entry_index < entry_count; entry_index++)
	{
		Entry entry;
		entry.filename = rd.ReadData(8);
		entry.filename.erase(entry.filename.find_last_not_of(' ') + 1);
		entry.file_offset = rd.ReadUnsigned(4);
		entry.header_size = rd.ReadUnsigned(4);
		entries.emplace_back(entry);
	}

	for(auto& entry : entries)
	{
		rd.Seek(entry.file_offset);
		entry.contents = std::make_shared<LEFormat>();
		entry.contents->ReadFile(rd);
	}
}

offset_t W3Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	wr.WriteData("W3");
	wr.WriteWord(1, system_version.minor);
	wr.WriteWord(1, system_version.major);
	wr.WriteWord(2, entries.size());
	wr.Skip(8);
	for(auto& entry : entries)
	{
		wr.WriteData(8, entry.filename, ' ');
		wr.WriteWord(4, entry.file_offset);
		wr.WriteWord(4, entry.header_size);
	}
	// TODO: write contents

	return offset_t(-1);
}

void W3Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_windows1252);

	dump.SetTitle("W3 format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.AddField("System version", Dumper::VersionDisplay::Make(), offset_t(system_version.major), offset_t(system_version.minor));
	file_region.AddField("Entry count", Dumper::DecDisplay::Make(), offset_t(entries.size()));
	file_region.Display(dump);

	uint16_t entry_count = 0;
	for(auto& entry : entries)
	{
		uint32_t entry_end = uint32_t(entry_count + 1) < entries.size() ? entries[entry_count + 1].file_offset : file_end;
		Dumper::Region entry_region("W3 format entry", entry.file_offset, std::max(entry_end, entry.file_offset) - entry.file_offset, 8);
		entry_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(entry_count + 1));
		entry_region.AddField("Filename", Dumper::StringDisplay::Make("\""), entry.filename);
		entry_region.AddField("Header size", Dumper::HexDisplay::Make(), offset_t(entry.header_size));
		entry_region.Display(dump);

		entry.contents->Dump(dump);

		entry_count ++;
	}
}

// W4Format

void W4Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	file_end = rd.GetImageEnd();
	rd.Skip(2); // "W4"
	system_version.minor = rd.ReadUnsigned(1);
	system_version.major = rd.ReadUnsigned(1);
	chunk_size = rd.ReadUnsigned(2);
	uint16_t chunk_count = rd.ReadUnsigned(2);
	rd.Skip(8); // "DS" and null bytes
	for(uint16_t chunk_index = 0; chunk_index < chunk_count; chunk_index++)
	{
		Chunk chunk;
		chunk.file_offset = rd.ReadUnsigned(4);
		if(chunks.size() > 0)
			chunks.back().length = chunk.file_offset - chunks.back().file_offset;
		chunks.emplace_back(chunk);
	}
	if(chunks.size() > 0)
		chunks.back().length = file_end - chunks.back().file_offset;

	// TODO: read and decode contents
}

offset_t W4Format::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	wr.WriteData("W4");
	wr.WriteWord(1, system_version.minor);
	wr.WriteWord(1, system_version.major);
	wr.WriteWord(2, chunk_size);
	wr.WriteWord(2, chunks.size());
	wr.WriteData("DS");
	wr.Skip(6);
	for(auto& chunk : chunks)
	{
		wr.WriteWord(4, chunk.file_offset);
	}
	// TODO: write contents

	return offset_t(-1);
}

void W4Format::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_windows1252);

	dump.SetTitle("W4 format");
	Dumper::Region file_region("File", file_offset, file_end - file_offset, 8);
	file_region.AddField("System version", Dumper::VersionDisplay::Make(), offset_t(system_version.major), offset_t(system_version.minor));
	file_region.AddField("Chunk size", Dumper::HexDisplay::Make(4), offset_t(chunk_size));
	file_region.AddField("Chunk count", Dumper::DecDisplay::Make(), offset_t(chunks.size()));
	file_region.Display(dump);

	uint16_t chunk_count = 0;
	for(auto& chunk : chunks)
	{
		Dumper::Region chunk_region("Chunk", chunk.file_offset, chunk.length, 8);
		chunk_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(chunk_count + 1));
		chunk_region.Display(dump);
		chunk_count ++;
	}

	// TODO: display entries
}

