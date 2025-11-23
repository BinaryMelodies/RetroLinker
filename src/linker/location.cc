
#include "section.h"
#include "location.h"
#include "position.h"

using namespace Linker;

bool Location::Displace(const Displacement& displacement)
{
	if(section == nullptr)
		return false;
	auto it = displacement.find(section);
	if(it != displacement.end())
	{
		section = it->second.section;
		offset += it->second.offset;
		return true;
	}
	return false;
}

Position Location::GetPosition(bool segment_of) const
{
	/*
		Note that the start of a segment address space, and the actual start of the segment data, might be different.
		For example, for flat .com files on MS-DOS, the first byte of the image is actually at offset 0x100, preceded by 256 bytes of what is called the Program Segment Prefix.
		To accomodate for this, an offset of 0 will be translated to 0x100, but the segment will still start at 0.
		We need to use different calls (Start() or Base()) to retrieve these values.
	*/
	if(!segment_of)
		return section ? section->Start() + offset : Position(offset);
	else
		return section ? section->Base() : Position();
}

bool Location::operator==(const Location& other) const
{
	return section == other.section && offset == other.offset;
}

bool Location::operator<(const Location& other) const
{
	return section < other.section || (section == other.section && offset < other.offset);
}

Location& Location::operator+=(offset_t value)
{
	offset += value;
	return *this;
}

Location& Location::operator-=(offset_t value)
{
	offset -= value;
	return *this;
}

std::ostream& Linker::operator<<(std::ostream& out, const Location& location)
{
	out << "location ";
	if(location.section)
		out << location.section->name << ":";
	out << std::hex << location.offset << std::dec;
	return out;
}

Location Linker::operator+(Location a, offset_t b)
{
	return a += b;
}

Location Linker::operator-(Location a, offset_t b)
{
	return a -= b;
}

