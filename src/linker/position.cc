
#include <iostream>
#include <string>
#include "position.h"
#include "segment.h"

using namespace Linker;

offset_t Position::GetSegmentOffset() const
{
	return address - segment->GetStartAddress();
}

Position& Position::operator+=(offset_t value)
{
	address += value;
	return *this;
}

Position& Position::operator-=(offset_t value)
{
	address -= value;
	return *this;
}

Position Linker::operator+(Position a, offset_t b)
{
	return a += b;
}

Position Linker::operator-(Position a, offset_t b)
{
	return a -= b;
}

bool Linker::operator ==(const Position& a, const Position& b)
{
	return a.address == b.address && a.segment == b.segment;
}

bool Linker::operator !=(const Position& a, const Position& b)
{
	return !(a == b);
}

std::ostream& Linker::operator<<(std::ostream& out, const Position& position)
{
	out << "position(" << std::hex << position.address << std::dec;
	if(position.segment)
		out << " in " << position.segment->name;
	out << ")";
	return out;
}

