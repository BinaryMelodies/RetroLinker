
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

class ImageStreambuf : public std::basic_streambuf<char>
{
public:
	std::shared_ptr<Linker::ActualImage> image;
	offset_t image_offset = 0;
	offset_t image_displacement;

	ImageStreambuf(std::shared_ptr<Linker::ActualImage> image, offset_t image_displacement = 0)
		: image(image), image_displacement(image_displacement)
	{
	}

protected:
	pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
	{
		if((which & std::ios_base::in) != 0)
		{
			switch(dir)
			{
			case std::ios_base::beg:
				if(off < 0 || offset_t(off) < image_displacement)
					image_offset = 0;
				image_offset = off - image_displacement;
				if(image_offset > image->ImageSize())
					image_offset = image->ImageSize();
				break;
			case std::ios_base::end:
				if(off < 0 && offset_t(-off) > image->ImageSize())
					image_offset = 0;
				else
					image_offset = image->ImageSize() + off; // TODO: check overflow
				break;
			case std::ios_base::cur:
				if(off < 0 && offset_t(-off) > image_offset)
					image_offset = 0;
				else
					image_offset += off; // TODO: check overflow
				break;
			}
		}
		if(image_offset > image->ImageSize())
			image_offset = image->ImageSize();
		return pos_type(image_offset + image_displacement);
	}

	pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override
	{
		if((which & std::ios_base::in) != 0)
		{
			if(pos < 0 || offset_t(pos) < image_displacement)
				image_offset = 0;
			else
				image_offset = offset_t(pos) - image_displacement;
			if(image_offset > image->ImageSize())
				image_offset = image->ImageSize();
		}
		return pos_type(image_offset + image_displacement);
	}

	std::streamsize xsgetn(char_type * s, std::streamsize count) override
	{
		std::streamsize result = image->ReadData(count, image_offset, s);
		image_offset += result;
		return result;
	}
};

class BitStream
{
protected:
	std::shared_ptr<Linker::ActualImage> contents;
	offset_t contents_offset = 0;
	uint8_t byte = 0;
	unsigned bit_count = 0;

	void NextByte()
	{
		if(contents_offset >= contents->ImageSize())
			return;
		byte = contents->GetByte(contents_offset);
		bit_count = 8;
		contents_offset++;
	}

	unsigned ReadSomeBits(unsigned count, uint8_t& read_bits)
	{
		if(bit_count == 0)
		{
			NextByte();
			if(bit_count == 0)
				return 0;
		}
		if(count > bit_count)
			count = bit_count;

		read_bits = byte & ((1 << count) - 1);
		byte >>= count;
		bit_count -= count;
		return count;
	}

public:
	BitStream(std::shared_ptr<Linker::ActualImage> contents)
		: contents(contents)
	{
	}

	uint32_t ReadBits(unsigned count)
	{
		uint32_t value = 0;
		uint32_t shift = 0;
		while(count > 0)
		{
			uint8_t bit_sequence = 0;
 			unsigned read_bit_count = ReadSomeBits(count, bit_sequence);
			if(read_bit_count == 0)
				return value;
			value |= uint32_t(bit_sequence) << shift;
			shift += read_bit_count;
			count -= read_bit_count;
		}
		return value;
	}

	bool IsOver() const
	{
		return contents_offset >= contents->ImageSize() && bit_count == 0;
	}
};

std::shared_ptr<Linker::Buffer> W4Format::DecompressW4()
{
	std::shared_ptr<Linker::Buffer> buffer = std::make_shared<Linker::Buffer>();
	uint32_t chunk_index = 0;
	for(auto& chunk : chunks)
	{
		std::vector<uint8_t> bytes;
		BitStream bitstream(chunk.contents);
		bool chunk_over = false;
		while(!chunk_over && !bitstream.IsOver())
		{
			size_t depth = 0;
			size_t count = 0;
			switch(bitstream.ReadBits(2))
			{
			case 0b00:
				//Linker::Debug << "Debug: 0b00" << std::endl;
				depth = bitstream.ReadBits(6);
				if(depth == 0)
				{
					chunk_over = true;
				}
				break;
			case 0b01:
				//Linker::Debug << "Debug: 0b01" << std::endl;
				bytes.push_back(bitstream.ReadBits(7) | 0x80);
				//Linker::Debug << std::hex << int(bytes[bytes.size() - 1]) << std::endl;
				break;
			case 0b10:
				//Linker::Debug << "Debug: 0b10" << std::endl;
				bytes.push_back(bitstream.ReadBits(7));
				//Linker::Debug << std::hex << int(bytes[bytes.size() - 1]) << std::endl;
				break;
			case 0b11:
				//Linker::Debug << "Debug: 0b11" << std::endl;
				switch(bitstream.ReadBits(1))
				{
				case 0b0:
					depth = 64 + bitstream.ReadBits(8);
					break;
				case 0b1:
					depth = 320 + bitstream.ReadBits(12);
					if(depth == 4415)
						continue;
					break;
				}
				break;
			}
			if(depth != 0)
			{
				size_t count_bits = 0;
				while(bitstream.ReadBits(1) == 0)
				{
					count_bits ++;
					if(count_bits == 9)
					{
						Linker::FatalError("Fatal error: illegal encoding in W4 compression");
					}
				}
				count = bitstream.ReadBits(count_bits) + (1 << count_bits) + 1;

				//Linker::Debug << "Debug: repeat " << std::dec << depth << " bytes for " << count << " bytes" << std::endl;
				for(size_t char_index = 0; char_index < count; char_index++)
				{
					bytes.push_back(bytes[bytes.size() - depth]);
					//Linker::Debug << std::hex << int(bytes[bytes.size() - 1]) << std::endl;
				}
			}
		}
		buffer->Append(bytes);
		buffer->Resize(chunk_size * (chunk_index + 1));
		chunk_index ++;
	}
	return buffer;
}

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

	for(auto& chunk : chunks)
	{
		rd.Seek(chunk.file_offset);
		chunk.contents = Linker::Buffer::ReadFromFile(rd, chunk.length);
	}

	std::shared_ptr<Linker::Buffer> image = DecompressW4();

#if 0
	Dumper::Dumper dump(std::cout);
	dump.SetEncoding(Dumper::Block::encoding_windows1252);
	Dumper::Block decompressed_block("Decompressed block", 0, image, 0, 8);
	decompressed_block.Display(dump);
#endif

	ImageStreambuf sb(image, file_offset);
	std::istream in(&sb);
	Linker::Reader image_rd(::LittleEndian, &in);
	w3format.ReadFile(image_rd);
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
		Dumper::Block chunk_block("Chunk", chunk.file_offset, std::const_pointer_cast<Linker::ActualImage>(chunk.contents->AsImage()), 0, 8); // TODO: remove const_pointer_cast
		chunk_block.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(chunk_count + 1));
		chunk_block.Display(dump);

		chunk_count ++;
	}

	w3format.Dump(dump);
}

