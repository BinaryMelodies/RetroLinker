
#include "w3exe.h"

using namespace Microsoft;

void W3Format::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	file_offset = rd.Tell();
	rd.Skip(2); // "W3"
	unknown = rd.ReadUnsigned(2);
	uint32_t entry_count = rd.ReadUnsigned(4); // TODO: unknown if 16-bit or 32-bit
	rd.Skip(8);
	for(uint32_t i = 0; i < entry_count; i++)
	{
		Entry entry;
		rd.ReadData(entry.filename);
		entry.file_offset = rd.ReadUnsigned(4);
		entry.unknown = rd.ReadUnsigned(4);
		entries.push_back(entry);
	}
	// TODO: read contents
}

offset_t W3Format::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(file_offset);
	wr.WriteData("W3");
	wr.WriteWord(2, unknown);
	wr.WriteWord(4, entries.size()); // TODO: unknown if 16-bit or 32-bit
	wr.Skip(8);
	for(auto& entry : entries)
	{
		wr.WriteData(entry.filename);
		wr.WriteWord(4, entry.file_offset);
		wr.WriteWord(4, entry.unknown);
	}
	// TODO: write contents

	return offset_t(-1);
}

