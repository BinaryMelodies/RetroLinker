
#include "relocation.h"

using namespace Linker;

Relocation Relocation::Empty()
{
	return Relocation(false, 0, Location(), Target(), Target(), 0, ::DefaultEndianType);
}

Relocation Relocation::Absolute(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype)
{
	return Relocation(false, size, source, target, Target(), addend, endiantype);
}

Relocation Relocation::Absolute(size_t size, Location source, Target target, uint64_t addend)
{
	return Absolute(size, source, target, addend, ::DefaultEndianType);
}

Relocation Relocation::Offset(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype)
{
	/* Intel 8086 specific */
	return Relocation(false, size, source, target, target.GetSegment(), addend, endiantype);
}

Relocation Relocation::Offset(size_t size, Location source, Target target, uint64_t addend)
{
	/* Intel 8086 specific */
	return Offset(size, source, target, addend, ::DefaultEndianType);
}

Relocation Relocation::OffsetFrom(size_t size, Location source, Target target, Target reference, uint64_t addend, EndianType endiantype)
{
	return Relocation(false, size, source, target, reference, addend, endiantype);
}

Relocation Relocation::OffsetFrom(size_t size, Location source, Target target, Target reference, uint64_t addend)
{
	return OffsetFrom(size, source, target, reference, addend, ::DefaultEndianType);
}

Relocation Relocation::Relative(size_t size, Location source, Target target, uint64_t addend, EndianType endiantype)
{
	return Relocation(false, size, source, target, Target(source), addend, endiantype);
}

Relocation Relocation::Relative(size_t size, Location source, Target target, uint64_t addend)
{
	return Relative(size, source, target, addend, ::DefaultEndianType);
}

Relocation Relocation::Paragraph(Location source, Target target, uint64_t addend)
{
	/* Intel 8086 specific */
	return Relocation(true, 2, source, target, Target(), addend, ::LittleEndian);
}

Relocation Relocation::Selector(Location source, Target target, uint64_t addend)
{
	/* Intel 8086 specific */
	return Relocation(true, 2, source, target, Target(), addend, ::LittleEndian);
}

Relocation Relocation::Segment(size_t size, Location source, Target target, uint64_t addend)
{
	/* Zilog Z8000 specific */
	return Relocation(true, size, source, target, Target(), addend, ::BigEndian);
}

Relocation Relocation::ParagraphDifference(Location source, Target target, Target reference, uint64_t addend)
{
	/* Intel 8086 specific */
	return Relocation(true, 2, source, target, reference, addend, ::LittleEndian);
}

Relocation& Relocation::SetMask(uint64_t new_mask)
{
	mask = new_mask;
	return *this;
}

Relocation& Relocation::SetShift(int new_shift)
{
	shift = new_shift;
	return *this;
}

Relocation& Relocation::SetSubtract(bool to_subtract)
{
	subtract = to_subtract;
	return *this;
}

bool Relocation::Displace(const Displacement& displacement)
{
	return source.Displace(displacement) | target.Displace(displacement) | reference.Displace(displacement);
}

bool Relocation::Resolve(Module& object, Resolution& resolution)
{
	Position target_position, reference_position;
	if(!target.Lookup(object, target_position) || !reference.Lookup(object, reference_position))
	{
		return false;
	}
//	Linker::Debug << "Resolved " << target << " to " << target_position.address << ", reference " << reference_position.address << std::endl;
	offset_t value = target_position.address - reference_position.address;
	if(subtract)
		value = -value;
	if(segment_of)
	{
		/* TODO: signal appropriate error if "target_position.address - reference_position.address" != 0 in protected mode */
		resolution = Resolution(addend + (value >> 4),
			target_position.segment,
			reference_position.segment);
		return true;
	}
	else
	{
		resolution = Resolution(addend + value,
			target_position.segment,
			reference_position.segment);
		return true;
	}
}

uint64_t Relocation::ReadUnsigned()
{
	uint64_t value = source.section->ReadUnsigned(size, source.offset, endiantype);
	value &= mask;
	if(shift < 0)
		value >>= -shift;
	else if(shift > 0)
		value <<= shift;
	return value;
}

int64_t Relocation::ReadSigned()
{
	int64_t value = source.section->ReadSigned(size, source.offset, endiantype);
	value &= mask; /* TODO: sign extend from highest bit */
	if(shift < 0)
		value >>= -shift;
	else if(shift > 0)
		value <<= shift;
	return value;
}

void Relocation::WriteWord(uint64_t value)
{
	if(shift < 0)
		value <<= -shift;
	else
		value >>= shift; /* TODO: signed */
	if(mask != (uint64_t)-1)
	{
		uint64_t word = source.section->ReadUnsigned(size, source.offset, endiantype);
		value = (value & mask) | (word & ~mask);
	}
	source.section->WriteWord(size, source.offset, value, endiantype);
}

void Relocation::AddCurrentValue()
{
//	Linker::Debug << "Debug: add current value of " << ReadUnsigned() << std::endl;
	addend += ReadUnsigned();
}

bool Relocation::IsRelative() const
{
	if(const Linker::Location * frame = std::get_if<Linker::Location>(&reference.target))
	{
		return source == *frame;
	}
	return false;
}

std::ostream& Linker::operator<<(std::ostream& out, const Relocation& relocation)
{
	out << relocation.size << " byte relocation(at " << relocation.source << " to ";
	if(relocation.subtract)
	{
		out << "negated ";
	}
	if(relocation.segment_of)
	{
		out << "segment of ";
	}
	out << relocation.target;
	out << " relative to " << relocation.reference;
	if(relocation.addend)
	{
		out << " add " << (int64_t)relocation.addend;
	}
	out << ")";
	return out;
}

