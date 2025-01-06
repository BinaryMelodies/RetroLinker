
#include <bit>
#include "common.h"

size_t GetOffset(EndianType endiantype, size_t bytes, size_t index)
{
	switch(endiantype)
	{
	case LittleEndian:
		return index;
	case BigEndian:
		return bytes - index - 1;
	case PDP11Endian:
		return bytes > 1 ? bytes - (index ^ 1) - 1 : index;
	case AntiPDP11Endian:
		return bytes > 1 ? index ^ 1 : index;
	default:
		Linker::FatalError("Internal error: invalid endianness");
	}
}

offset_t AlignTo(offset_t value, offset_t align)
{
	return (value + align - 1) & ~(align - 1);
}

uint16_t Swap16(uint16_t value)
{
	return (value << 8) | (value >> 8);
}

uint16_t FromLittleEndian16(uint16_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap16(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return value;
	}
}

uint16_t FromBigEndian16(uint16_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap16(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return Swap16(value);
	}
}

uint32_t Swap32(uint32_t value)
{
	return ((uint32_t)Swap16(value) << 16) | (Swap16(value >> 16));
}

uint32_t Swap32words(uint32_t value)
{
	return (value << 16) | (value >> 16);
}

uint32_t FromLittleEndian32(uint32_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap32(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return value;
	}
}

uint32_t FromBigEndian32(uint32_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap16(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return Swap32(value);
	}
}

uint32_t FromPDP11Endian32(uint32_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap32(Swap32words(value));
	case std::endian::little:
	default: /* this assumes little by default */
		return Swap32words(value);
	}
}

uint64_t Swap64(uint64_t value)
{
	return ((uint64_t)Swap32(value) << 32) | (Swap32(value >> 32));
}

uint64_t Swap64words(uint64_t value)
{
	return ((uint64_t)Swap32words(value) << 32) | (Swap32words(value >> 32));
}

uint64_t FromLittleEndian64(uint64_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap64(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return value;
	}
}

uint64_t FromBigEndian64(uint64_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap16(value);
	case std::endian::little:
	default: /* this assumes little by default */
		return Swap64(value);
	}
}

uint64_t FromPDP11Endian64(uint64_t value)
{
	switch(std::endian::native)
	{
	case std::endian::big:
		return Swap64(Swap64words(value));
	case std::endian::little:
	default: /* this assumes little by default */
		return Swap64words(value);
	}
}

uint64_t ReadUnsigned(size_t bytes, size_t maximum, uint8_t const * data, EndianType endiantype)
{
	uint64_t value = 0;
	if(maximum > bytes)
		maximum = bytes;
	if(maximum == bytes)
	{
		/* Optimization */
		switch(bytes)
		{
		case 1:
			return *(uint8_t *)data;
		case 2:
			value = *(uint16_t *)data;
			switch(endiantype)
			{
			case LittleEndian:
			case PDP11Endian:
				return FromLittleEndian16(value);
			case BigEndian:
			case AntiPDP11Endian:
				return FromBigEndian16(value);
			default:
				Linker::FatalError("Internal error: invalid endianness");
			}
			break;
		case 4:
			value = *(uint32_t *)data;
			switch(endiantype)
			{
			case LittleEndian:
				return FromLittleEndian32(value);
			case BigEndian:
				return FromBigEndian32(value);
			case PDP11Endian:
				return FromPDP11Endian32(value);
			/* TODO: AntiPDP11Endian */
			default:
				Linker::FatalError("Internal error: invalid endianness");
			}
			break;
		case 8:
			value = *(uint64_t *)data;
			switch(endiantype)
			{
			case LittleEndian:
				return FromLittleEndian64(value);
			case BigEndian:
				return FromBigEndian64(value);
			case PDP11Endian:
				return FromPDP11Endian64(value);
			/* TODO: AntiPDP11Endian */
			default:
				Linker::FatalError("Internal error: invalid endianness");
			}
			break;
		}
	}
	for(size_t i = 0; i < maximum; i++)
	{
		value |= data[i] << (GetOffset(endiantype, bytes, i) << 3);
	}
	return value;
}

int64_t SignExtend(size_t bytes, int64_t value)
{
	if(bytes == sizeof(int64_t))
		return value;
	/* Optimization */
	switch(bytes)
	{
	case 1:
		return (int8_t)value;
	case 2:
		return (int16_t)value;
	case 4:
		return (int32_t)value;
	}
	if((value & (int64_t)1 << ((bytes << 3) - 1)))
		value |= (int64_t)-1 << (bytes << 3);
	return value;
}

int64_t ReadSigned(size_t bytes, size_t maximum, uint8_t const * data, EndianType endiantype)
{
	return SignExtend(bytes, (int64_t)ReadUnsigned(bytes, maximum, data, endiantype));
}

void WriteWord(size_t bytes, size_t maximum, uint8_t * data, uint64_t value, EndianType endiantype)
{
	if(maximum > bytes)
		maximum = bytes;
	for(size_t i = 0; i < maximum; i++)
	{
		data[i] = value >> (GetOffset(endiantype, bytes, i) << 3);
	}
}

bool LookupOption(std::map<std::string, std::string>& options, std::string key, std::string& value)
{
	auto entry = options.find(key);
	if(entry != options.end())
	{
		value = entry->second;
		return true;
	}
	else
	{
		return false;
	}
}

std::ostream Linker::Debug(std::cerr.rdbuf());
std::ostream Linker::Warning(std::cerr.rdbuf());
std::ostream Linker::Error(std::cerr.rdbuf());

[[noreturn]] void Linker::FatalError(std::string message)
{
	Linker::Error << message << std::endl;
	throw Linker::Exception(message);
}

