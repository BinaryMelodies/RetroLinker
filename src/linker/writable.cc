
#include "segment.h"
#include "writable.h"

using namespace Linker;

Writable::~Writable()
{
}

int Writable::GetByte(offset_t offset)
{
	return -1;
}

offset_t Writable::WriteFile(Writer& wr)
{
	return WriteFile(wr, ActualDataSize());
}

