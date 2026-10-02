
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

void Buffer::Append(std::vector<uint8_t>& additional_data)
{
	data.insert(data.end(), additional_data.begin(), additional_data.end());
}

void Buffer::ReadFile(Reader& rd)
{
	ReadFile(rd, data.size());
}

void Buffer::ReadFileRemaining(Reader& rd)
{
	ReadFile(rd, rd.GetRemainingCount());
}

void Buffer::ReadFile(Reader& rd, offset_t count)
{
	rd.ReadData(count, data);
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(Reader& rd)
{
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(rd.GetRemainingCount());
	buffer->ReadFile(rd);
	return buffer;
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(Reader& rd, offset_t count)
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

