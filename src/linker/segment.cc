
#include "section.h"
#include "segment.h"

using namespace Linker;

bool Segment::IsMissing()
{
	return data_size == 0 && zero_fill == 0 && optional_extra == 0;
}

void Segment::Fill()
{
	for(Section * section : sections)
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
	Section * tail = sections.back();
	offset_t extra = tail->RealignEnd(align);
	if(tail->IsZeroFilled())
		zero_fill += extra;
	else
		data_size += extra;
	if(align > this->align)
		this->align = align;
}

void Segment::Append(Section * section)
{
	if(section == nullptr)
		return;
	assert(section->segment == nullptr);
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
	section->segment = this;
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

offset_t Segment::WriteFile(std::ostream& out, offset_t size, offset_t offset)
{
	offset_t count = 0;
	for(Section * section : sections)
	{
		if(!section->IsZeroFilled())
		{
			if(offset > section->ActualDataSize())
			{
				offset -= section->ActualDataSize();
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

offset_t Segment::WriteFile(std::ostream& out)
{
	offset_t count = 0;
	for(Section * section : sections)
	{
		if(!section->IsZeroFilled())
			count += section->WriteFile(out);
	}
	return count;
}

offset_t Segment::WriteFile(Writer& wr, offset_t count, offset_t offset)
{
	return WriteFile(*wr.out, count, offset);
}

offset_t Segment::WriteFile(Writer& wr)
{
	return WriteFile(*wr.out);
}

int Segment::GetByte(offset_t offset)
{
	for(Section * section : sections)
	{
		if(offset < section->Size())
		{
			return section->GetByte(offset);
		}
		else
		{
			offset -= section->Size();
		}
	}
	return -1;
}

offset_t Segment::TotalSize()
{
	return data_size + zero_fill;
}

offset_t Segment::ActualDataSize() /* should be always equivalent to data_size */
{
	offset_t sum = 0;
	for(Section * section : sections)
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
		zero_fill += address - current_address;
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

	for(Section * section : sections)
	{
		section->ResetAddress(section->GetStartAddress() + amount);
		/* note: biases don't get altered */
	}
}

void Segment::SetStartAddress(offset_t address)
{
	ShiftAddress(address - base_address);
}

std::ostream& Linker::operator<<(std::ostream& out, const Segment& segment)
{
	out << "segment " << segment.name;
	return out;
}

