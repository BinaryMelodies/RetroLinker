
#include <cstring>
#include "buffer.h"
#include "reader.h"
#include "writer.h"

using namespace Linker;

offset_t Buffer::ImageSize() const
{
	return data.size();
}

void Buffer::Resize(offset_t new_size)
{
	data.resize(new_size);
}

offset_t Buffer::Expand(offset_t new_size)
{
	if(new_size <= ImageSize())
	{
		return 0;
	}
	else
	{
		offset_t extra = new_size - ImageSize();
		Resize(new_size);
		return extra;
	}
}

offset_t Buffer::WriteData(size_t bytes, offset_t offset, const void * buffer)
{
	offset_t expand_count = Expand(offset + bytes);
	std::copy(reinterpret_cast<const uint8_t *>(buffer), reinterpret_cast<const uint8_t *>(buffer) + bytes, data.data() + offset);
	return expand_count;
}

offset_t Buffer::WriteWord(size_t bytes, offset_t offset, uint64_t value, EndianType endiantype)
{
	offset_t expand_count = Expand(offset + bytes);
	::WriteWord(bytes, bytes, data.data() + offset, value, endiantype);
	return expand_count;
}

offset_t Buffer::WriteWord(size_t bytes, offset_t offset, uint64_t value)
{
	return WriteWord(bytes, offset, value, ::DefaultEndianType);
}

offset_t Buffer::WriteWord(size_t bytes, uint64_t value, EndianType endiantype)
{
	return WriteWord(bytes, ImageSize(), value, endiantype);
}

offset_t Buffer::WriteWord(size_t bytes, uint64_t value)
{
	return WriteWord(bytes, value, ::DefaultEndianType);
}

offset_t Buffer::Append(const void * new_data, size_t length)
{
	offset_t old_size = data.size();
	data.insert(data.end(), reinterpret_cast<const uint8_t *>(new_data), reinterpret_cast<const uint8_t *>(new_data) + length);
	return old_size;
}

offset_t Buffer::Append(const char * new_data)
{
	return Append(new_data, strlen(new_data));
}

offset_t Buffer::Append(std::vector<uint8_t>& additional_data)
{
	return Append(additional_data.data(), additional_data.size());
}

void Buffer::ReadFile(const std::shared_ptr<Reader>& rd)
{
	ReadFile(rd, data.size());
}

void Buffer::ReadFileRemaining(const std::shared_ptr<Reader>& rd)
{
	ReadFile(rd, rd->GetRemainingCount());
}

void Buffer::ReadFile(const std::shared_ptr<Reader>& rd, offset_t count)
{
	rd->ReadData(count, data);
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(const std::shared_ptr<Reader>& rd)
{
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(rd->GetRemainingCount());
	buffer->ReadFile(rd);
	return buffer;
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(const std::shared_ptr<Reader>& rd, offset_t count)
{
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
	buffer->ReadFile(rd, count);
	return buffer;
}

offset_t Buffer::WriteFile(Writer& wr, offset_t count, offset_t offset) const
{
	return wr.WriteData(count, data, offset);
}

size_t Buffer::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	if(offset >= data.size())
		return 0;
	if(offset + bytes > data.size())
	{
		bytes = data.size() - offset;
	}
	memcpy(buffer, data.data() + offset, bytes);
	return bytes;
}

