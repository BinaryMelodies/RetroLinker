
#include "image.h"
#include "segment.h"

using namespace Linker;

Image::~Image()
{
}

int Image::GetByte(offset_t offset)
{
	return -1;
}

offset_t Image::WriteFile(Writer& wr)
{
	return WriteFile(wr, ImageSize());
}

