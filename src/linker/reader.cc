
#include "reader.h"
#include "image.h"

using namespace Linker;

Reader::OverflowHandlingRequest Reader::global_overflow_behavior = Reader::OverflowHandlingRequest::Default;

std::shared_ptr<Reader> Reader::CreateWindow(offset_t new_start_offset, offset_t new_maximum_size, offset_t displacement)
{
//	if(auto window_reader = dynamic_cast<WindowReader *>(this))
//	{
//		Linker::Debug << "Debug: Old window " << std::hex << window_reader->start_offset << " -> " << window_reader->maximum_size << " : " << window_reader->window_offset << std::endl;
//	}
//	Linker::Debug << "Debug: Create " << std::hex << new_start_offset << " -> " << new_maximum_size << " : " << displacement << std::endl;

	auto window_reader = std::make_shared<WindowReader>(endiantype, shared_from_this(), new_start_offset, new_maximum_size, displacement);
//	Linker::Debug << "Debug: New window " << std::hex << window_reader->start_offset << " -> " << window_reader->maximum_size << " : " << window_reader->window_offset << std::endl;
	return window_reader;
}

void Reader::ReadData(size_t count, void * data)
{
	offset_t _off = Tell();
	size_t actual_count = Read(reinterpret_cast<char *>(data), count);
	if(actual_count != count)
	{
		Linker::Error << "Error: Reading error at offset 0x" << std::hex << _off << ": tried reading " << std::dec << count << " only managed " << std::dec << actual_count << std::endl;
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
	offset_t last_position = Tell();
	while(tmp.size() < maximum && (c = ReadUnsigned(1)) != terminator)
	{
		if(last_position == Tell())
			break;
		last_position = Tell();
		tmp += c;
	}
	return tmp;
}

std::string Reader::ReadASCIIZ(size_t maximum)
{
	return ReadASCII('\0', maximum);
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
		data[0] = ReadUnsigned(1);
		data[1] = ReadUnsigned(1);
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

offset_t Reader::GetImageEnd()
{
	offset_t current = Tell();
	SeekEnd();
	offset_t total = Tell();
	Seek(current);
	return total;
}

offset_t Reader::GetRemainingCount()
{
	return GetImageEnd() - Tell();
}

size_t StreamReader::Read(void * data, size_t max_count)
{
	in->read(reinterpret_cast<char *>(data), max_count);
	return in->gcount();
}

void StreamReader::Seek(offset_t offset)
{
	in->clear();
	in->seekg(offset, std::ios_base::beg);
	in->clear();
}

void StreamReader::Skip(relative_offset_t offset)
{
	in->clear();
	in->seekg(offset, std::ios_base::cur);
	in->clear();
}

void StreamReader::SeekEnd(relative_offset_t offset)
{
	in->clear();
	in->seekg(offset, std::ios_base::end);
	in->clear();
}

offset_t StreamReader::Tell()
{
	return in->tellg();
}

#if 0
void WindowReader::_FixupWindow()
{
	if(auto window_reader = std::dynamic_pointer_cast<WindowReader>(reader))
	{
		// TODO: how to handle displacement?
		if(window_reader->maximum_size != offset_t(-1))
		{
			if(start_offset > window_reader->maximum_size)
			{
				start_offset = window_reader->maximum_size;
			}

			if(maximum_size == offset_t(-1)
			|| start_offset + maximum_size > maximum_size)
			{
				maximum_size = window_reader->maximum_size - start_offset;
			}
		}

		start_offset += window_reader->start_offset;
		displacement += window_reader->displacement;
		reader = window_reader->reader;
	}
}
#endif

size_t WindowReader::Read(void * data, size_t max_count)
{
	offset_t current = reader->Tell();
//	Linker::Debug << "Debug: WindowReader::Read." << std::hex << current << "(" << std::hex << max_count << ");" << std::endl;
	if(current < start_offset)
	{
		reader->Seek(start_offset);
		current = start_offset;
	}

	size_t permitted_count = max_count;
	if(maximum_size != offset_t(-1) && current + permitted_count > start_offset + maximum_size)
	{
		permitted_count = start_offset + maximum_size - reader->Tell();
	}
//	Linker::Debug << "Debug: actual Read." << std::hex << current << "(" << std::hex << permitted_count << ");" << std::endl;
	return reader->Read(data, permitted_count);
}

void WindowReader::Seek(offset_t offset)
{
	if(offset < window_offset)
	{
		offset = 0;
	}
	else
	{
		offset -= window_offset;
	}

	// check if the position overflows the read window limit
	if(maximum_size != offset_t(-1) && offset > maximum_size)
	{
		offset = maximum_size;
	}
	reader->Seek(start_offset + offset);
}

void WindowReader::Skip(relative_offset_t offset)
{
	// check if the position overflows the read window limit
	if(maximum_size != offset_t(-1) && reader->Tell() + offset > start_offset + maximum_size)
	{
		offset = start_offset + maximum_size - reader->Tell();
	}

	if(start_offset != 0)
	{
		// do not permit seeking before the start of the read window
		offset_t actual_offset = reader->Tell();
		if(offset < 0 && offset_t(-offset) <= actual_offset)
		{
			actual_offset = 0;
		}
		else
		{
			actual_offset += offset;
		}

		if(actual_offset < start_offset)
		{
			actual_offset = start_offset;
		}
		reader->Seek(actual_offset);
	}
	else
	{
		reader->Skip(offset);
	}
}

void WindowReader::SeekEnd(relative_offset_t offset)
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
		reader->Seek(actual_offset);
	}
	else
	{
		reader->SeekEnd(offset);
	}
}

offset_t WindowReader::Tell()
{
	offset_t actual_offset = reader->Tell();
	if(actual_offset < start_offset)
	{
		actual_offset = 0;
	}
	else
	{
		actual_offset -= start_offset;
	}
	return actual_offset + window_offset;
}

offset_t WindowReader::GetImageEnd()
{
	if(maximum_size != offset_t(-1))
	{
		return maximum_size;
	}
	else
	{
		return Reader::GetImageEnd();
	}
}

size_t ImageReader::Read(void * data, size_t max_count)
{
	size_t actual_read = image->ReadData(max_count, position, data);
	position += actual_read;
	return actual_read;
}

void ImageReader::Seek(offset_t offset)
{
	position = offset;
}

void ImageReader::Skip(relative_offset_t offset)
{
	position += offset;
}

void ImageReader::SeekEnd(relative_offset_t offset)
{
	offset_t total_size = image->ImageSize();
	if(total_size == offset_t(-1))
	{
		FatalError("Fatal error: attempting to go to end to image of undetermined size");
	}
	position = total_size + offset;
}

offset_t ImageReader::Tell()
{
	return position;
}

offset_t ImageReader::GetImageEnd()
{
	return image->ImageSize();
}

