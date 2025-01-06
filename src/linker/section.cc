
#include "section.h"

using namespace Linker;

std::shared_ptr<Section> Section::ReadFromFile(Reader& rd, std::string name, int flags)
{
	std::shared_ptr<Section> section = std::make_shared<Section>(name, flags);
	section->ReadFile(rd);
	return section;
}

std::shared_ptr<Section> Section::ReadFromFile(Reader& rd, offset_t count, std::string name, int flags)
{
	std::shared_ptr<Section> section = std::make_shared<Section>(name, flags);
	section->ReadFile(rd, count);
	return section;
}

void Section::AlterFlags(bool state, unsigned flags_mask)
{
	if(state)
		flags = (section_flags)(flags | flags_mask);
	else
		flags = (section_flags)(flags & ~flags_mask);
}

void Section::SetFlag(unsigned newflags)
{
	AlterFlags(true, newflags & ~(Mergeable|ZeroFilled|Fixed)); /* these flags need special considerations */
	if((newflags & Mergeable))
		SetMergeable(true);
	if((newflags & ZeroFilled))
		SetZeroFilled(true);
	//if((newflags & Fixed) && !(flags & Fixed))
	//	SetAddress(0);
}

unsigned Section::GetFlags() const
{
	return flags;
}

bool Section::IsReadable() const
{
	return flags & Readable;
}

void Section::SetReadable(bool state)
{
	AlterFlags(state, Readable);
}

bool Section::IsWritable() const
{
	return flags & Writable;
}

void Section::SetWritable(bool state)
{
	AlterFlags(state, Writable);
}

bool Section::IsExecable() const
{
	return flags & Execable;
}

void Section::SetExecable(bool state)
{
	AlterFlags(state, Execable);
}

bool Section::IsMergeable() const
{
	return flags & Mergeable;
}

void Section::SetMergeable(bool state)
{
	AlterFlags(state, Mergeable);
}

bool Section::IsFixed() const
{
	return flags & Fixed;
}

bool Section::IsZeroFilled() const
{
	return flags & ZeroFilled;
}

offset_t Section::SetZeroFilled(bool is_zero_filled)
{
	offset_t extra = 0;
	if(is_zero_filled)
	{
		assert(data.size() == 0);
	}
	else if(IsZeroFilled())
	{
		data.resize(size);
		size = 0; /* unneeded */
		extra = data.size();
	}
	AlterFlags(is_zero_filled, ZeroFilled);
	return extra;
}

offset_t Section::GetAlign() const
{
	return IsFixed() ? 0 : align;
}

void Section::SetAlign(offset_t new_align)
{
	assert(new_align != 0);
	assert((new_align & (new_align - 1)) == 0);
	if(IsFixed())
	{
		if((address & (new_align - 1)) != 0)
		{
			Linker::Error << "Error: attempting to set alignment of fixed segment to unsupported value" << std::endl;
		}
	}
	else
	{
		if(new_align > align)
		{
			align = new_align;
		}
	}
}

offset_t Section::GetStartAddress() const
{
	return IsFixed() ? address : 0;
}

offset_t Section::SetAddress(offset_t new_address)
{
	if(IsFixed())
	{
		if(address != new_address)
		{
			Linker::Warning << "Warning: Attempting to change address of fixed section" << std::endl;
		}
	}
	else
	{
		new_address = AlignTo(new_address, align);
		AlterFlags(true, Fixed);
		address = new_address;
	}
	return address;
}

void Section::ResetAddress(offset_t new_address)
{
	if(!IsFixed())
	{
		AlterFlags(true, Fixed);
	}
	address = new_address;
}

offset_t Section::Size() const
{
	return IsZeroFilled() ? size : data.size();
}

offset_t Section::Expand(offset_t new_size)
{
	if(new_size <= Size())
	{
		return 0;
	}
	else if(IsZeroFilled())
	{
		offset_t extra = new_size - size;
		size = new_size;
		return extra;
	}
	else
	{
		offset_t extra = new_size - data.size();
		data.resize(new_size);
		return extra;
	}
}

offset_t Section::RealignEnd(offset_t align)
{
	if(IsFixed())
	{
		return Expand(::AlignTo(GetStartAddress() + Size(), align) - GetStartAddress());
	}
	else
	{
		SetAlign(align);
		return Expand(::AlignTo(Size(), align));
	}
}

int Section::GetByte(offset_t offset)
{
	if(IsZeroFilled())
		return offset < size ? 0 : -1;
	else
		return offset < data.size() ? data[offset] : -1;
}

uint64_t Section::ReadUnsigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	return offset >= data.size() ? 0 : ::ReadUnsigned(bytes, data.size() - offset, data.data() + offset, endiantype);
}

uint64_t Section::ReadUnsigned(size_t bytes, offset_t offset) const
{
	return ReadUnsigned(bytes, offset, ::DefaultEndianType);
}

int64_t Section::ReadSigned(size_t bytes, offset_t offset, EndianType endiantype) const
{
	return offset >= data.size() ? 0 : ::ReadSigned(bytes, data.size() - offset, data.data() + offset, endiantype);
}

uint64_t Section::ReadSigned(size_t bytes, offset_t offset) const
{
	return ReadSigned(bytes, offset, ::DefaultEndianType);
}

void Section::WriteWord(size_t bytes, offset_t offset, uint64_t value, EndianType endiantype)
{
	if(value == 0 && (offset > data.size() || IsZeroFilled()))
		return;
	assert(!IsZeroFilled());
	Expand(offset + bytes);
	::WriteWord(bytes, bytes, data.data() + offset, value, endiantype);
}

void Section::WriteWord(size_t bytes, offset_t offset, uint64_t value)
{
	WriteWord(bytes, offset, value, ::DefaultEndianType);
}

void Section::WriteWord(size_t bytes, uint64_t value, EndianType endiantype)
{
	WriteWord(bytes, data.size(), value, endiantype);
}

void Section::WriteWord(size_t bytes, uint64_t value)
{
	WriteWord(bytes, value, ::DefaultEndianType);
}

offset_t Section::Append(const void * new_data, size_t length)
{
	assert(!IsZeroFilled());
	offset_t offset = Size();
	data.insert(data.end(), reinterpret_cast<const uint8_t *>(new_data), reinterpret_cast<const uint8_t *>(new_data) + length);
	return offset;
}

offset_t Section::Append(const char * new_data)
{
	return Append(new_data, strlen(new_data));
}

offset_t Section::Append(const Section& other)
{
	/* only append segment if it belongs to this collection */
	assert(other.collection_name == "" || other.collection_name == name);

	assert(!other.IsFixed());
	assert(IsMergeable() == other.IsMergeable());
	if(!IsFixed() && align < other.align)
	{
		align = other.align;
	}
	if(IsMergeable())
	{
		Linker::FatalError("Internal error: unimplemented: appending mergeable sections"); /* TODO - probably with OMF */
	}
	RealignEnd(other.align);
	offset_t offset = Size();
	if(IsZeroFilled())
	{
		assert(other.IsZeroFilled());
		size += other.size;
	}
	else
	{
		if(other.IsZeroFilled())
		{
			Linker::Warning << "Warning: zero filled section concatenated to non-zero filled section" << std::endl;
			Expand(Size() + other.size);
		}
		else
		{
			data.insert(data.end(), other.data.begin(), other.data.end());
		}
	}
	return offset;
}

offset_t Section::Append(Buffer& buffer)
{
	if(Section * section = dynamic_cast<Section *>(&buffer))
	{
		return Append(*section);
	}
	else
	{
		return Append(buffer.data.data(), buffer.data.size());
	}
}

Position Section::Start() const
{
	auto the_segment = segment.lock();
	assert(the_segment != nullptr);
//	Linker::Debug << "Start " << name << " = " << segment << ":" << address << std::endl;
	return Position(address, the_segment);
}

Position Section::Base() const
{
//	Linker::Debug << "Bias " << name << " = " << bias << std::endl;
	return Start() - bias;
}

void Section::ReadFile(std::istream& in)
{
	SetZeroFilled(false);
	in.read(reinterpret_cast<char *>(data.data()), data.size());
}

void Section::ReadFile(Reader& in)
{
	SetZeroFilled(false);
	in.ReadData(data.size(), reinterpret_cast<char *>(data.data()));
}

offset_t Section::WriteFile(std::ostream& out, offset_t size, offset_t offset) const
{
	if(offset >= data.size())
		return 0;
	size = std::min(size + offset, data.size()) - offset;
	out.write(reinterpret_cast<const char *>(data.data()) + offset, size);
	return size;
}

offset_t Section::WriteFile(std::ostream& out) const
{
	return WriteFile(out, data.size());
}

void Section::Reset()
{
	data.clear();
	size = 0;
	align = 1;
	bias = 0;
	resource_type = "    ";
	resource_id = uint16_t(0);
	segment = std::weak_ptr<Segment>();

	if(IsFixed())
		address = 0;
	else
		align = 1;
}

std::ostream& Linker::operator<<(std::ostream& out, const Section& section)
{
	if(section.IsReadable())
		out << "R";
	if(section.IsWritable())
		out << "W";
	if(section.IsExecable())
		out << "X";
	if(section.IsZeroFilled())
		out << "Z";
	out << " section(" << section.name << " size 0x" << std::hex << section.Size() << std::dec;
	out << ")";
	return out;
}

