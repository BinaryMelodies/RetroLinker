
#include "section.h"
#include "segment.h"
#include "writer.h"

using namespace Linker;

bool Segment::IsMissing()
{
	return data_size == 0 && zero_fill == 0 && optional_extra == 0;
}

void Segment::Fill()
{
	for(auto& section : sections)
	{
		if(section->IsZeroFilled())
		{
			Linker::Debug << section->name << ", " << section->GetStartAddress() << ", " << section->Size() << std::endl;
			section->SetZeroFilled(false);
		}
	}
	data_size += zero_fill;
	zero_fill = 0;
}

void Segment::RealignEnd(offset_t align)
{
	if(sections.size() == 0)
		return;
	std::shared_ptr<Section> tail = sections.back();
	offset_t extra = tail->RealignEnd(align);
	if(tail->IsZeroFilled())
		zero_fill += extra;
	else
		data_size += extra;
	if(align > this->align)
		this->align = align;
}

void Segment::Append(std::shared_ptr<Section> section)
{
	if(section == nullptr)
		return;
	assert(section->segment.use_count() == 0);
	offset_t align = section->GetAlign();
	offset_t address;
	if(sections.size() > 0)
	{
		assert(!section->IsFixed());
		RealignEnd(section->GetAlign());
		address = base_address + TotalSize();
	}
	else
	{
		address = base_address;
	}
	section->segment = shared_from_this();
	section->SetAddress(address);
	/* TODO: is this needed? */
	if(sections.size() > 0)
	{
		section->bias = sections.back()->bias + sections.back()->Size();
	}
	else
	{
		section->bias = base_address;
	}
	if(section->IsZeroFilled())
	{
		zero_fill += section->Size();
	}
	else
	{
		if(zero_fill != 0)
		{
			Linker::Warning << "Warning: Filling in zero filled sections" << std::endl;
			Fill();
		}
		data_size += section->Size();
	}
	sections.push_back(section);
	if(align > this->align)
		this->align = align;
}

offset_t Segment::WriteFile(std::ostream& out, offset_t size, offset_t offset) const
{
	offset_t count = 0;
	for(auto& section : sections)
	{
		if(!section->IsZeroFilled())
		{
			if(offset > section->ImageSize())
			{
				offset -= section->ImageSize();
			}
			else
			{
				offset_t written = section->WriteFile(out, size, offset);
				offset = 0;
				count += written;
				if(size < written)
					return count;
				else
					size -= written;
			}
		}
	}
	return count;
}

offset_t Segment::WriteFile(std::ostream& out) const
{
	offset_t count = 0;
	for(auto& section : sections)
	{
		if(!section->IsZeroFilled())
			count += section->WriteFile(out);
	}
	return count;
}

offset_t Segment::WriteFile(Writer& wr, offset_t count, offset_t offset) const
{
	return WriteFile(*wr.out, count, offset);
}

offset_t Segment::WriteFile(Writer& wr) const
{
	return WriteFile(*wr.out);
}

size_t Segment::ReadData(size_t bytes, offset_t offset, void * buffer) const
{
	size_t total_count = 0;
	for(auto& section : sections)
	{
		if(offset < section->Size())
		{
			size_t actual_count = ReadData(bytes, offset, buffer);
			total_count += actual_count;
			if(actual_count >= bytes)
				return total_count;
			bytes -= actual_count;
			offset = 0;
			buffer = reinterpret_cast<char *>(buffer) + actual_count;
		}
		else
		{
			offset -= section->Size();
		}
	}
	return total_count;
}

offset_t Segment::TotalSize()
{
	return data_size + zero_fill;
}

offset_t Segment::ImageSize() const /* should be always equivalent to data_size */
{
	offset_t sum = 0;
	for(auto& section : sections)
	{
		if(!section->IsZeroFilled())
			sum += section->Size();
	}
	assert(data_size == sum);
	return sum;
}

offset_t Segment::GetStartAddress()
{
	return base_address;
}

offset_t Segment::GetEndAddress()
{
	return base_address + TotalSize();
}

void Segment::SetEndAddress(offset_t address)
{
	offset_t current_address = GetEndAddress();
	if(current_address > address)
	{
		Linker::Debug << "Internal warning: attempting to decrease segment size, ignoring" << std::endl;
	}
	if(current_address >= address)
	{
		return;
	}
	if(sections.size() == 0)
	{
		Linker::Error << "Error: no sections present to set end address of, ignoring" << std::endl;
	}
	else if(sections.back()->IsZeroFilled())
	{
		offset_t extra = address - current_address;
		sections.back()->Expand(sections.back()->Size() + extra);
		zero_fill += extra;
		Linker::Debug << "Debug: extended zero filled segment from " << (sections.back()->Size() - extra) << " to " << sections.back()->Size() << std::endl;
//		Linker::Warning << "Internal error: attempting to increase segment size with zero-fill, ignoring" << std::endl;
//		return;
	}
	else
	{
		data_size += sections.back()->Expand(address - sections.back()->GetStartAddress());
	}
}

void Segment::AlignEndAddress(offset_t align)
{
	SetEndAddress(::AlignTo(GetEndAddress(), align));
}

void Segment::ShiftAddress(int64_t amount)
{
	offset_t new_base_address;
	if(amount < 0)
		new_base_address = (base_address + amount + align - 1) & ~(align - 1);
	else
		new_base_address = ::AlignTo(base_address + amount, align);
	amount = new_base_address - base_address;
//	Linker::Debug << "New base address: 0x" << std::hex << new_base_address << " (attempted 0x" << std::hex << base_address + amount << ") instead of 0x" << std::hex << base_address << std::endl;
	base_address = new_base_address;

	for(auto& section : sections)
	{
		section->ResetAddress(section->GetStartAddress() + amount);
		/* note: biases don't get altered */
	}
}

void Segment::SetStartAddress(offset_t address)
{
	ShiftAddress(address - base_address);
}

std::shared_ptr<Segment> Segment::shared_from_this()
{
	return std::static_pointer_cast<Segment>(ActualImage::shared_from_this());
}

std::ostream& Linker::operator<<(std::ostream& out, const Segment& segment)
{
	out << "segment " << segment.name;
	return out;
}

