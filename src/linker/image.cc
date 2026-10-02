
#include <sstream>
#include "buffer.h"
#include "image.h"
#include "segment.h"
#include "writer.h"

using namespace Linker;

std::shared_ptr<const Image> Contents::AsImage() const
{
	// TODO: test
	// TODO: find a more efficient way
	std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
	std::ostringstream oss;
	Writer wr(::UndefinedEndian, &oss);
	WriteFile(wr);
	std::string data = oss.str();
	buffer->data.assign(data.begin(), data.end());
	return buffer;
}

std::shared_ptr<Image> Contents::AsImage()
{
	return std::const_pointer_cast<Image>(const_cast<const Contents *>(this)->AsImage());
}

// TODO: move
offset_t RandomAccessContents::WriteFile(Writer& wr) const
{
	return WriteFile(wr, ImageSize());
}

std::shared_ptr<const Image> Image::AsImage() const
{
	return shared_from_this();
}

uint64_t Image::ReadUnsigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	std::vector<uint8_t> buffer(bytes);
	offset_t count = ReadData(bytes, offset, buffer.data());
	return ::ReadUnsigned(bytes, count, buffer.data(), endiantype);
}

uint64_t Image::ReadUnsigned(size_t bytes, offset_t offset) const
{
	return ReadUnsigned(bytes, offset, ::DefaultEndianType);
}

int64_t Image::ReadSigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	std::vector<uint8_t> buffer(bytes);
	offset_t count = ReadData(bytes, offset, buffer.data());
	return ::ReadSigned(bytes, count, buffer.data(), endiantype);
}

int64_t Image::ReadSigned(size_t bytes, offset_t offset) const
{
	return ReadSigned(bytes, offset, ::DefaultEndianType);
}

int Image::GetByte(offset_t offset) const
{
	return ReadUnsigned(1, offset, ::EndianType(0));
}

offset_t Image::WriteFile(Writer& wr, offset_t count, offset_t offset) const
{
	std::vector<uint8_t> buffer(count);
	offset_t actual_count = ReadData(count, offset, buffer.data());
	wr.WriteData(actual_count, buffer.data());
	return actual_count;
}

