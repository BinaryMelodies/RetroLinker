
#include <sstream>
#include "buffer.h"
#include "image.h"
#include "segment.h"
#include "writer.h"

using namespace Linker;

std::shared_ptr<const ActualImage> Image::AsImage() const
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

std::shared_ptr<ActualImage> Image::AsImage()
{
	return std::const_pointer_cast<ActualImage>(const_cast<const Image *>(this)->AsImage());
}

offset_t Image::WriteFile(Writer& wr) const
{
	return WriteFile(wr, ImageSize());
}

std::shared_ptr<const ActualImage> ActualImage::AsImage() const
{
	return shared_from_this();
}

uint64_t ActualImage::ReadUnsigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	uint8_t buffer[bytes];
	offset_t count = ReadData(bytes, offset, buffer);
	return ::ReadUnsigned(bytes, count, buffer, endiantype);
}

uint64_t ActualImage::ReadUnsigned(size_t bytes, offset_t offset) const
{
	return ReadUnsigned(bytes, offset, ::DefaultEndianType);
}

int64_t ActualImage::ReadSigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	uint8_t buffer[bytes];
	offset_t count = ReadData(bytes, offset, buffer);
	return ::ReadSigned(bytes, count, buffer, endiantype);
}

int64_t ActualImage::ReadSigned(size_t bytes, offset_t offset) const
{
	return ReadSigned(bytes, offset, ::DefaultEndianType);
}

int ActualImage::GetByte(offset_t offset) const
{
	return ReadUnsigned(1, offset, ::EndianType(0));
}

