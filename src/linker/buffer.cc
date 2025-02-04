
#include <cstring>
#include "buffer.h"
#include "reader.h"
#include "writer.h"

using namespace Linker;

offset_t Buffer::ImageSize()
{
	return data.size();
}

void Buffer::ReadFile(Reader& rd, offset_t count)
{
	rd.ReadData(count, data);
}

void Buffer::ReadFile(Reader& rd)
{
	/* TODO: is there a better way? */
	offset_t current = rd.Tell();
	rd.SeekEnd();
	offset_t total = rd.Tell();
	rd.Seek(current);
	ReadFile(rd, total - current);
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(Reader& rd)
{
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
	buffer->ReadFile(rd);
	return buffer;
}

std::shared_ptr<Buffer> Buffer::ReadFromFile(Reader& rd, offset_t count)
{
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
	buffer->ReadFile(rd, count);
	return buffer;
}

offset_t Buffer::WriteFile(Writer& wr, offset_t count, offset_t offset)
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

