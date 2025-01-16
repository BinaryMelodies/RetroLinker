
#include "reader.h"

using namespace Linker;

void Reader::ReadData(size_t count, void * data)
{
	in->read(reinterpret_cast<char *>(data), count);
	size_t actual_count = in->gcount();
	if(actual_count != count)
	{
		Linker::Debug << "Internal warning: tried reading " << count << " only managed " << actual_count;
	}
}

void Reader::ReadData(size_t count, std::vector<uint8_t>& data, size_t offset)
{
	data.resize(offset + count);
	ReadData(count, reinterpret_cast<void *>(data.data() + offset));
}

void Reader::ReadData(std::vector<uint8_t>& data, size_t offset)
{
	if(offset >= data.size())
		return;
	ReadData(data.size() - offset, reinterpret_cast<void *>(data.data() + offset));
}

std::string Reader::ReadData(size_t count, bool terminate_at_null)
{
	char data[count];
	ReadData(count, data);
	if(terminate_at_null)
	{
		count = strnlen(data, count);
	}
	return std::string(data, count);
}

std::string Reader::ReadASCIIZ(size_t maximum)
{
	std::string tmp;
	int c;
	while(tmp.size() < maximum && (c = in->get()) != 0)
	{
		tmp += c;
	}
	return tmp;
}

uint64_t Reader::ReadUnsigned(size_t bytes, EndianType endiantype)
{
	uint8_t data[bytes];
	ReadData(bytes, data);
	return ::ReadUnsigned(bytes, bytes, data, endiantype);
}

uint64_t Reader::ReadUnsigned(size_t bytes)
{
	return ReadUnsigned(bytes, endiantype);
}

uint64_t Reader::ReadSigned(size_t bytes, EndianType endiantype)
{
	uint8_t data[bytes];
	ReadData(bytes, data);
	return ::ReadSigned(bytes, bytes, data, endiantype);
}

uint64_t Reader::ReadSigned(size_t bytes)
{
	return ReadSigned(bytes, endiantype);
}

void Reader::Seek(offset_t offset)
{
	in->seekg(offset, std::ios_base::beg);
	in->clear();
}

void Reader::Skip(offset_t offset)
{
	in->seekg(offset, std::ios_base::cur);
	in->clear();
}

void Reader::SeekEnd(relative_offset_t offset)
{
	in->seekg(offset, std::ios_base::end);
	in->clear();
}

offset_t Reader::Tell()
{
#if 0
	if(in->fail())
		in->clear();
	if(in->eof())
	{
		in->clear();
		SeekEnd();
	}
	offset_t value = in->tellg();
	if(value == offset_t(-1))
	{
	Linker::Debug << "no!" << std::endl;
		in->clear();
		value = in->tellg();
	}
	return value;
#endif
	return in->tellg();
}

