
#include <cstring>
#include "writer.h"
#include "buffer.h"

using namespace Linker;

size_t Writer::WriteData(size_t max_count, const std::vector<uint8_t>& data, size_t offset)
{
	if(offset >= data.size())
		return 0;
	if(data.size() - offset < max_count)
		max_count = data.size() - offset;
	WriteData(max_count, reinterpret_cast<const void *>(data.data() + offset));
	return max_count;
}

size_t Writer::WriteData(const std::vector<uint8_t>& data, size_t offset)
{
	if(offset >= data.size())
		return 0;
	WriteData(data.size() - offset, reinterpret_cast<const void *>(data.data() + offset));
	return data.size() - offset;
}

void Writer::WriteData(size_t count, std::string text, char padding)
{
	std::vector<char> data(count);
	if(text.size() < count)
	{
		memcpy(data.data(), text.c_str(), text.size());
		memset(data.data() + text.size(), padding, count - text.size());
	}
	else
	{
		memcpy(data.data(), text.c_str(), count);
	}
	WriteData(count, data.data());
}

void Writer::WriteData(std::string text)
{
	WriteData(text.size(), text);
}

void Writer::WriteData(size_t count, std::istream& in)
{
	const size_t buffer_size = std::min(count, size_t(4096));
	std::vector<char> buffer(buffer_size);
//Linker::Debug << "Write total " << count << std::endl;
	while(count > 0)
	{
		offset_t byte_count = buffer_size;
		if(byte_count > count)
			byte_count = count;
//Linker::Debug << "Write " << byte_count << " for total " << count << std::endl;
		in.read(buffer.data(), byte_count);
		WriteData(byte_count, buffer.data());
		count -= byte_count;
	}
}

void Writer::WriteWord(size_t bytes, uint64_t value, EndianType endiantype)
{
	std::vector<uint8_t> data(bytes);
	::WriteWord(bytes, bytes, data.data(), value, endiantype);
	WriteData(bytes, data.data());
}

void Writer::WriteWord(size_t bytes, uint64_t value)
{
	WriteWord(bytes, value, endiantype);
}

void Writer::FillTo(offset_t position)
{
	offset_t location = Tell();
	if(location >= position)
	{
		Seek(position);
		return;
	}
	SeekEnd();
	location = Tell();
	if(location < position)
	{
		Seek(position - 1);
		WriteWord(1, 0);
	}
	else
	{
		Seek(position);
	}
}

void Writer::AlignTo(offset_t align)
{
	FillTo(::AlignTo(Tell(), align));
}

void StreamWriter::_FillNulls(size_t count)
{
	for(size_t i = 0; i < count; i++)
		out->put(0);
}

void StreamWriter::WriteData(size_t count, const void * data)
{
	out->write(reinterpret_cast<const char *>(data), count);
}

void StreamWriter::Seek(offset_t offset)
{
	/* TODO: optimize? */
//	out->clear(std::ostream::goodbit);
	out->seekp(0, std::ios_base::end);
//	assert(offset >= offset_t(out->tellp()));
	if(offset < offset_t(out->tellp()))
	{
		out->seekp(offset);
	}
	else
	{
		_FillNulls(offset - out->tellp());
	}
}

void StreamWriter::Skip(offset_t offset)
{
	/* TODO: optimize? */
	offset_t current = out->tellp();
	Seek(current + offset);
}

void StreamWriter::SeekEnd(offset_t offset)
{
	if(!out->seekp(offset, std::ios_base::end))
	{
		/* TODO */
	}
}

offset_t StreamWriter::Tell()
{
	return out->tellp();
}

void BufferWriter::WriteData(size_t count, const void * data)
{
	buffer->WriteData(count, position, reinterpret_cast<const char *>(data));
	position += count;
}

void BufferWriter::Seek(offset_t offset)
{
	if(offset < buffer->ImageSize())
	{
		buffer->Resize(offset);
	}
	position = offset;
}

void BufferWriter::Skip(offset_t offset)
{
	Seek(position + offset);
}

void BufferWriter::SeekEnd(offset_t offset)
{
	Seek(buffer->ImageSize());
}

offset_t BufferWriter::Tell()
{
	return position;
}

