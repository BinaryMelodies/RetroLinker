
#include "buffer.h"

using namespace Linker;

offset_t Buffer::ActualDataSize()
{
	return data.size();
}

void Buffer::ReadFile(Reader& rd, offset_t count)
{
	data.resize(count);
	rd.ReadData(data.size(), reinterpret_cast<char *>(data.data()));
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

offset_t Buffer::WriteFile(Writer& wr, offset_t count, offset_t offset)
{
	if(offset >= data.size())
		return 0;
	count = std::min(data.size(), offset + count) - offset;
	wr.WriteData(count, reinterpret_cast<char *>(data.data()) + offset);
	return count;
}

int Buffer::GetByte(offset_t offset)
{
	return offset < data.size() ? data[offset] : -1;
}

