
#include "writer.h"

using namespace Linker;

void Writer::WriteData(size_t count, const void * data)
{
	out->write(reinterpret_cast<const char *>(data), count);
}

void Writer::WriteData(size_t count, std::string text, char padding)
{
	char data[count];
	if(text.size() < sizeof data)
	{
		memcpy(data, text.c_str(), text.size());
		memset(data + text.size(), padding, sizeof data - text.size());
	}
	else
	{
		memcpy(data, text.c_str(), sizeof data);
	}
	out->write(data, count);
}

void Writer::WriteData(std::string text)
{
	WriteData(text.size(), text);
}

void Writer::WriteData(size_t count, std::istream& in)
{
	char buffer[count < 4096 ? count : 4096];
//Linker::Debug << "Write total " << count << std::endl;
	while(count > 0)
	{
		offset_t byte_count = sizeof buffer;
		if(byte_count > count)
			byte_count = count;
//Linker::Debug << "Write " << byte_count << " for total " << count << std::endl;
		in.read(buffer, byte_count);
		out->write(buffer, byte_count);
		count -= byte_count;
	}
}

void Writer::WriteWord(size_t bytes, uint64_t value, EndianType endiantype)
{
	uint8_t data[bytes];
	::WriteWord(bytes, bytes, data, value, endiantype);
	WriteData(bytes, data);
}

void Writer::WriteWord(size_t bytes, uint64_t value)
{
	WriteWord(bytes, value, endiantype);
}

void Writer::ForceSeek(offset_t offset)
{
//	out->clear(std::ostream::goodbit);
	out->seekp(0, std::ios_base::end);
//	assert(offset >= (offset_t)out->tellp());
	if(offset < (offset_t)out->tellp())
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

