
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
		switch(on_overflow)
		{
		case IgnoreOnOverflow:
			Linker::Debug << "Internal warning: tried reading " << count << " only managed " << actual_count << std::endl;
			break;
		case ReportOnOverflow:
			throw ReadOverflow();
		case TerminateOnOverflow:
			Linker::FatalError("Fatal error: read less than expected");
		}
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
	std::vector<char> data(count);
	ReadData(count, data.data());
	if(terminate_at_null)
	{
		count = strnlen(data.data(), count);
	}
	return std::string(data.data(), count);
}

std::string Reader::ReadASCII(char terminator, size_t maximum)
{
	std::string tmp;
	int c;
	offset_t last_position = offset_t(in->tellg());
	while(tmp.size() < maximum && (c = in->get()) != terminator)
	{
		if(last_position == offset_t(in->tellg()))
			break;
		last_position = offset_t(in->tellg());
		tmp += c;
	}
	return tmp;
}

std::string Reader::ReadASCIIZ(size_t maximum)
{
	return ReadASCII('\0', size_t(-1));
}

std::string Reader::ReadUTF16Data(size_t count, bool terminate_at_null)
{
	std::vector<char> data(2 * count);
	ReadData(count, data.data());
	if(terminate_at_null)
	{
		for(size_t index = 0; index < count * 2; index += 2)
		{
			if(data[index] == 0 && data[index + 1] == 0)
			{
				count = index / 2;
				break;
			}
		}
	}
	return std::string(data.data(), 2 * count);
}

std::string Reader::ReadUTF16Data(const char terminator[2], size_t maximum)
{
	std::string tmp;
	while(tmp.size() < (maximum < (size_t(-1) >> 1) ? 2 * maximum : size_t(-1)))
	{
		uint8_t data[2];
		data[0] = in->get();
		data[1] = in->get();
		if(data[0] == terminator[0] && data[1] == terminator[1])
			break;
		tmp += data[0];
		tmp += data[1];
	}
	return tmp;
}

std::string Reader::ReadUTF16Data(char16_t terminator, size_t maximum, EndianType endiantype)
{
	uint8_t data[2];
	::WriteWord(2, 2, data, terminator, endiantype);
	return ReadUTF16Data(reinterpret_cast<char *>(data), maximum);
}

std::string Reader::ReadUTF16Data(char16_t terminator, size_t maximum)
{
	return ReadUTF16Data(terminator, maximum, endiantype);
}

std::string Reader::ReadUTF16Data(char16_t terminator, EndianType endiantype)
{
	return ReadUTF16Data(terminator, size_t(-1), endiantype);
}

std::string Reader::ReadUTF16ZData(size_t maximum)
{
	static const char zero[2] = { 0, 0 };
	return ReadUTF16Data(zero, maximum);
}

uint64_t Reader::ReadUnsigned(size_t bytes, EndianType endiantype)
{
	std::vector<uint8_t> data(bytes);
	ReadData(bytes, data.data());
	return ::ReadUnsigned(bytes, bytes, data.data(), endiantype);
}

uint64_t Reader::ReadUnsigned(size_t bytes)
{
	return ReadUnsigned(bytes, endiantype);
}

uint64_t Reader::ReadSigned(size_t bytes, EndianType endiantype)
{
	std::vector<uint8_t> data(bytes);
	ReadData(bytes, data.data());
	return ::ReadSigned(bytes, bytes, data.data(), endiantype);
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
		offset_t actual_offset = start_offset + maximum_size + offset;
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

offset_t Reader::GetImageEnd()
{
	if(maximum_size != offset_t(-1))
	{
		return maximum_size;
	}
	else
	{
		offset_t current = Tell();
		SeekEnd();
		offset_t total = Tell();
		Seek(current);
		return total;
	}
}

offset_t Reader::GetRemainingCount()
{
	return GetImageEnd() - Tell();
}

