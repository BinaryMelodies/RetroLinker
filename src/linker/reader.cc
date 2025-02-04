
#include "reader.h"

using namespace Linker;

Reader Reader::CreateWindow(offset_t new_start_offset, offset_t new_maximum_size)
{
	if(new_start_offset > maximum_size)
	{
		new_start_offset = maximum_size;
	}

	if(maximum_size != offset_t(-1))
	{
		if(new_maximum_size == offset_t(-1))
		{
			new_maximum_size = maximum_size - new_start_offset;
		}
		else if(new_start_offset + new_maximum_size > maximum_size)
		{
			new_maximum_size = maximum_size - new_start_offset;
		}
	}

	new_start_offset += start_offset;

	return Reader(new_start_offset, new_maximum_size, endiantype, in);
}

void Reader::ReadData(size_t count, void * data)
{
	size_t permitted_count = count;
	if(maximum_size != offset_t(-1) && offset_t(in->tellg()) + permitted_count > start_offset + maximum_size)
	{
		permitted_count = start_offset + maximum_size - in->tellg();
	}
	in->read(reinterpret_cast<char *>(data), permitted_count);
	size_t actual_count = in->gcount();
	if(actual_count != count)
	{
		Linker::Debug << "Internal warning: tried reading " << count << " only managed " << actual_count << std::endl;
//		Linker::FatalError("Fatal error: read less than expected");
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

std::string Reader::ReadASCII(char terminator, size_t maximum)
{
	std::string tmp;
	int c;
	while(tmp.size() < maximum && (c = in->get()) != terminator)
	{
		tmp += c;
	}
	return tmp;
}

std::string Reader::ReadASCIIZ(size_t maximum)
{
	return ReadASCII('\0', size_t(-1));
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
	if(maximum_size != offset_t(-1) && offset > maximum_size)
	{
		offset = maximum_size;
	}
	in->seekg(start_offset + offset, std::ios_base::beg);
	in->clear();
}

void Reader::Skip(offset_t offset)
{
	if(maximum_size != offset_t(-1) && offset_t(in->tellg()) + offset > start_offset + maximum_size)
	{
		offset = start_offset + maximum_size - in->tellg();
	}
	if(start_offset != 0)
	{
		offset += in->tellg();
		if(offset < start_offset)
		{
			offset = start_offset;
		}
		in->seekg(offset, std::ios_base::beg);
	}
	else
	{
		in->seekg(offset, std::ios_base::cur);
	}
	in->clear();
}

void Reader::SeekEnd(relative_offset_t offset)
{
	if(maximum_size != offset_t(-1))
	{
		offset_t actual_offset = start_offset + maximum_size;
		if(actual_offset < start_offset)
		{
			actual_offset = start_offset;
		}
		else if(actual_offset > start_offset + maximum_size)
		{
			actual_offset = start_offset + maximum_size;
		}
		in->seekg(actual_offset, std::ios_base::beg);
	}
	else
	{
		in->seekg(offset, std::ios_base::end);
	}
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
	if(start_offset != 0)
	{
		return offset_t(in->tellg()) - start_offset;
	}
	else
	{
		return in->tellg();
	}
}

