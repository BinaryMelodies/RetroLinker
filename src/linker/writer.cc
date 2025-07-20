
#include <cstring>
#include "writer.h"

using namespace Linker;

void Writer::WriteData(size_t count, const void * data)
{
	out->write(reinterpret_cast<const char *>(data), count);
}

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
	out->write(data.data(), count);
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
		out->write(buffer.data(), byte_count);
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

void Writer::ForceSeek(offset_t offset)
{
//	out->clear(std::ostream::goodbit);
	out->seekp(0, std::ios_base::end);
//	assert(offset >= offset_t(out->tellp()));
	if(offset < offset_t(out->tellp()))
	{
		out->seekp(offset);
	}
	else
	{
		size_t count = offset - out->tellp();
		for(size_t i = 0; i < count; i++)
			out->put(0);
	}
}

void Writer::Seek(offset_t offset)
{
//	if(!out->seekp(offset)) /* TODO: optimize */
//	{
		ForceSeek(offset); /* force null fill */
//	}
}

void Writer::Skip(offset_t offset)
{
	offset_t current = out->tellp();
//	if(!out->seekp(offset, std::ios_base::cur)) /* TODO: optimize */
//	{
		ForceSeek(current + offset); /* force null fill */
//	}
}

void Writer::SeekEnd(offset_t offset)
{
	if(!out->seekp(offset, std::ios_base::end))
	{
		/* TODO */
	}
}

offset_t Writer::Tell()
{
	return out->tellp();
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

