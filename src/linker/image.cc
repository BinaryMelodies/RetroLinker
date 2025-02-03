
#include "image.h"
#include "segment.h"

using namespace Linker;

std::shared_ptr<ActualImage> Image::AsImage()
{
	// TODO
	return nullptr;
}

offset_t Image::WriteFile(Writer& wr)
{
	return WriteFile(wr, ImageSize());
}

std::shared_ptr<ActualImage> ActualImage::AsImage()
{
	return shared_from_this();
}

