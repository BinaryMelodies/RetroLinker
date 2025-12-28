
#include "aout.h"
#include "../linker/position.h"
#include "../linker/resolution.h"

using namespace AOut;

::EndianType AOutFormat::GetEndianType() const
{
	switch(cpu)
	{
	case M68K:
	case SPARC: case SPARC64:
	case PPC: case PPC64:
	case M88K:
	case PARISC:
		return ::BigEndian;
	case I386: case AMD64:
	case ARM: case AARCH64:
	case MIPS: /* TODO: check */
	case PDP11:
	case NS32K:
	case VAX:
		return ::LittleEndian; /* Note: actually, PDP-11 endian */
	case UNKNOWN:
	case ALPHA:
	case SUPERH: case SUPERH64:
	case IA64:
	case OR1K:
	case RISCV:
		// TODO
		return ::UndefinedEndian;
	default:
		Linker::FatalError("Internal error: invalid CPU type");
	}
}

AOutFormat::word_size_t AOutFormat::GetWordSize() const
{
	switch(cpu)
	{
	case M68K:
	case SPARC:
	case I386:
	case ARM:
	case MIPS:
		return WordSize32;
	case PDP11:
		return WordSize16;
	default:
		Linker::FatalError("Internal error: invalid CPU type");
	}
}

AOutFormat::Relocation AOutFormat::Relocation::ReadFile16Bit(Linker::Reader& rd, uint16_t offset)
{
	Relocation relocation;
	uint16_t word_value = rd.ReadUnsigned(2);
	relocation.address = offset;
	relocation.relative = (word_value & 1) != 0;
	relocation.size = 2;
	switch((word_value & 0xE))
	{
	case 0x00:
		relocation.segment = Absolute;
		relocation.literal_entry = word_value & 0xFFF0;
		break;
	case 0x02:
		relocation.segment = Text;
		relocation.literal_entry = word_value & 0xFFF0;
		break;
	case 0x04:
		relocation.segment = Data;
		relocation.literal_entry = word_value & 0xFFF0;
		break;
	case 0x06:
		relocation.segment = Bss;
		relocation.literal_entry = word_value & 0xFFF0;
		break;
	case 0x08:
		relocation.segment = External;
		relocation.symbol = word_value >> 4;
		break;
	default:
		relocation.segment = Illegal;
		relocation.literal_entry = word_value & 0xE;
		break;
	}
	return relocation;
}

void AOutFormat::Relocation::WriteFile16Bit(Linker::Writer& wr) const
{
	uint16_t word_value;
	switch(segment)
	{
	case External:
		word_value = 0x08 | (symbol << 4);
		break;
	case Absolute:
		word_value = 0x00;
		break;
	case Text:
		word_value = 0x02;
		break;
	case Data:
		word_value = 0x04;
		break;
	case Bss:
		word_value = 0x06;
		break;
	case Undefined:
	case FileName:
	case Illegal:
	default:
		return;
	}

	if(relative)
		word_value |= 1;

	wr.WriteWord(2, word_value);
}

AOutFormat::Relocation AOutFormat::Relocation::ReadFile32Bit(Linker::Reader& rd)
{
	Relocation relocation;
	relocation.address = rd.ReadUnsigned(4);
	uint32_t word_value = rd.ReadUnsigned(4);
	if((word_value & 0x08000000))
	{
		relocation.segment = External;
		relocation.symbol = word_value & 0x00FFFFFF;
	}
	else
	{
		switch(word_value & 0x00FFFFFF)
		{
		case 0:
			relocation.segment = Undefined;
			break;
		case 1:
			relocation.segment = External;
			break;
		case 2:
			relocation.segment = Absolute;
			break;
		case 4:
			relocation.segment = Text;
			break;
		case 6:
			relocation.segment = Data;
			break;
		case 8:
			relocation.segment = Bss;
			break;
		case 15:
			relocation.segment = FileName;
			break;
		default:
			relocation.segment = Illegal;
			relocation.literal_entry = word_value & 0x00FFFFFF;
			break;
		}
	}
	relocation.relative = (word_value & 0x01000000) != 0;
	relocation.size = 1 << ((word_value >> 25) & 3);
	return relocation;
}

void AOutFormat::Relocation::WriteFile32Bit(Linker::Writer& wr) const
{
	uint32_t word_value;
	switch(segment)
	{
	case Undefined:
		word_value = 0x00;
		break;
	case External:
		word_value = 0x08000000 | symbol;
		break;
	case Absolute:
		word_value = 0x02;
		break;
	case Text:
		word_value = 0x04;
		break;
	case Data:
		word_value = 0x06;
		break;
	case Bss:
		word_value = 0x08;
		break;
	case FileName:
		word_value = 0x0F;
		break;
	case Illegal:
	default:
		return;
	}

	switch(size)
	{
	case 1:
		break;
	case 2:
		word_value |= 0x02000000;
		break;
	case 4:
		word_value |= 0x04000000;
		break;
	case 8:
		word_value |= 0x06000000;
		break;
	default:
		return;
	}

	if(relative)
		word_value |= 0x01000000;

	wr.WriteWord(4, address);
	wr.WriteWord(4, word_value);
}

bool AOutFormat::AttemptFetchMagic(uint8_t signature[4])
{
	/* Check valid magic number */
	uint16_t attempted_magic;
	switch(midmag_endiantype)
	{
	case ::LittleEndian:
	case ::PDP11Endian:
		attempted_magic = signature[0] | (signature[1] << 8);
		break;
	case ::BigEndian: /* TODO: or PDP11 */
		attempted_magic = signature[word_size - 1] | (signature[word_size - 2] << 8);
		break;
	default:
		Linker::FatalError("Internal error: invalid endianness");
	}

	if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
		return false;
	magic = magic_type(attempted_magic);
	return true;
}

bool AOutFormat::CheckFileSizes(Linker::Reader& rd, offset_t image_size)
{
	/* Check if all sizes fit within the image */
	rd.Seek(file_offset + word_size);
	uint32_t full_size = 0;
	uint32_t load_size = 0;
	uint32_t next_size;
	/*
		16-bit: only add text, data and syms
		32-bit: also add trsize and drsize
	*/
	for(int i = 1; i < (word_size == WordSize16 ? 5 : 8); i++)
	{
		if(i == 3 || i == 5)
		{
			/* bss and entry point */
			rd.Skip(word_size);
			continue;
		}
		next_size = rd.ReadUnsigned(word_size);
		if(full_size + next_size < full_size || full_size + next_size > image_size)
			return false;
		full_size += next_size;

		if(word_size == WordSize16 && i != 4)
		{
			// text and data
			load_size += next_size;
		}
	}

	if(word_size == WordSize16)
	{
		rd.Seek(file_offset + 7 * word_size);
		if(rd.ReadUnsigned(word_size) == 0)
		{
			// add relocations (same size as text + data)
			if(full_size + load_size < full_size || full_size + load_size > image_size)
				return false;
			full_size += load_size;
		}
	}

	return true;
}

bool AOutFormat::AttemptReadFile(Linker::Reader& rd, uint8_t signature[4], offset_t image_size)
{
	if(!AttemptFetchMagic(signature))
		return false;

	/* Check valid CPU type (32-bit only) */
	if(word_size == WordSize32)
	{
		switch(endiantype)
		{
		case ::LittleEndian:
			switch(signature[2])
			{
			case MID_PC386:
				cpu = I386;
				break;
			case MID_BFD_ARM:
				cpu = ARM;
				break;
			case MID_MIPS1:
			case MID_MIPS2:
				cpu = MIPS;
				break;
			case 0:
				/* probably VAX */
				cpu = UNKNOWN;
				break;
			default:
				return false;
			}
			break;
		case ::BigEndian:
			switch(signature[1])
			{
			case MID_68010:
			case MID_68020:
				cpu = M68K;
				break;
			case MID_LINUX_SPARC:
				cpu = SPARC;
				break;
			case MID_UNKNOWN:
				/* probably 68000, "old Sun" */
				cpu = UNKNOWN;
				break;
			default:
				return false;
			}
			break;
		default:
			return false;
		}
	}

	return CheckFileSizes(rd, image_size);
}

uint32_t AOutFormat::GetPageSize() const
{
	switch(system)
	{
	case UNSPECIFIED:
		return 0x00000400;
	case UNIX_V1:
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
		return 0x00002000; // TODO: this is the value up to Version 7, according to apout, for PDP-11
	case LINUX:
		return 0x00000400;
	case FREEBSD:
		return 0x00001000;
	case NETBSD:
		if(page_size != 0)
			return page_size;

		switch(cpu)
		{
		// TODO: other CPUs
		case I386: case AMD64:
		case ARM: case AARCH64:
		case PPC: case PPC64:
		case SUPERH: case SUPERH64:
		case PARISC:
		case OR1K:
		case RISCV:
		case VAX:
		default:
			return page_size = 0x00001000;
		case ALPHA:
		case M68K:
		case SPARC: case SPARC64:
			return page_size = 0x00002000;
		case IA64:
			return page_size = 0x00004000;
		}
		break;
	case DJGPP1:
		return 0x00400000;
	case PDOS386:
	case EMX:
		return 0x00010000;
	}
	Linker::FatalError("Internal error: invalid system type");
}

uint32_t AOutFormat::GetTextOffset() const
{
	switch(system)
	{
	case UNIX_V1:
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
		return GetHeaderSize();
	case UNSPECIFIED:
	case LINUX:
		switch(magic)
		{
		case QMAGIC:
			return 0;
		case ZMAGIC:
			return GetPageSize();
		default:
			return GetHeaderSize();
		}
		break;
	case FREEBSD:
		if(midmag_endiantype == ::BigEndian)
		{
			// Network order
			switch(magic)
			{
			case ZMAGIC:
				return 0;
			default:
				return GetHeaderSize();
			}
		}
		else
		{
			switch(magic)
			{
			case QMAGIC:
				return 0;
			case ZMAGIC:
				return GetPageSize();
			default:
				return GetHeaderSize();
			}
		}
		break;
	case NETBSD:
		if(cpu == UNKNOWN)
		{
			// legacy midmag
			switch(magic)
			{
			case QMAGIC:
				return 0;
			case ZMAGIC:
				return GetPageSize();
			default:
				return GetHeaderSize();
			}
		}
		else
		{
			switch(magic)
			{
			case ZMAGIC:
				return 0;
			default:
				return GetHeaderSize();
			}
		}
		break;
	case DJGPP1:
		return GetHeaderSize();
	case PDOS386:
		return GetHeaderSize();
	case EMX:
		return 0x00000400;
		// TODO
		//return GetPageSize();
	}
	Linker::FatalError("Internal error: invalid system type");
}

uint32_t AOutFormat::GetTextAddress() const
{
	switch(system)
	{
	case UNIX_V1:
		return 0x00008000; // according to apout
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
		return 0; // TODO: according to apout, for PDP-11
	case UNSPECIFIED:
	case LINUX:
		switch(magic)
		{
		case QMAGIC:
			return GetPageSize();
		default:
			return 0;
		}
		break;
	case FREEBSD:
		if(midmag_endiantype == ::BigEndian)
		{
			return GetPageSize();
		}
		else
		{
			switch(magic)
			{
			case QMAGIC:
				return GetPageSize();
			default:
				// TODO: depends on the entry
				return 0;
			}
		}
		break;
	case NETBSD:
		if(cpu == UNKNOWN && magic == ZMAGIC)
		{
			return 0;
		}
		else
		{
			return GetPageSize();
		}
		break;
	case DJGPP1:
		return 0x00001000 + GetHeaderSize();
	case PDOS386:
		if(magic == ZMAGIC || magic == QMAGIC)
			// as per PDOS sources, not supported by the kernel
			return GetPageSize();
		else
			return 0;
	case EMX:
		return GetPageSize();
	}
	Linker::FatalError("Internal error: invalid system type");
}

uint32_t AOutFormat::GetDataOffsetAlign() const
{
	switch(system)
	{
	case UNIX_V1:
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
	case UNSPECIFIED:
	case LINUX:
		return 1;
	case FREEBSD:
		return GetPageSize();
	case NETBSD:
		return GetPageSize();
	case DJGPP1:
		return 1;
	case PDOS386:
		return 1;
	case EMX:
		return 1;
	}
	Linker::FatalError("Internal error: invalid system type");
}

uint32_t AOutFormat::GetDataAddressAlign() const
{
	switch(system)
	{
	case UNIX_V1:
		return 1;
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
		// TODO: according to apout, for PDP-11
		if(magic == NMAGIC)
			return GetPageSize();
		else
			return 1;
	case UNSPECIFIED:
	case LINUX:
	case DJGPP1:
	case EMX:
		if(magic == OMAGIC)
			return 1;
		else
			return GetPageSize();
	case FREEBSD:
	case NETBSD:
		if(magic == ZMAGIC || magic == QMAGIC)
			return GetPageSize();
		else
			return 1;
	case PDOS386:
		if(magic == ZMAGIC || magic == QMAGIC)
			// as per PDOS sources, not supported by the kernel
			return GetPageSize();
		else
			return 1;
	}
	Linker::FatalError("Internal error: invalid system type");
}

void AOutFormat::ReadHeader(Linker::Reader& rd)
{
	code_size = rd.ReadUnsigned(word_size);
	data_size = rd.ReadUnsigned(word_size);
	bss_size = rd.ReadUnsigned(word_size);
	symbol_table_size = rd.ReadUnsigned(word_size);
	entry_address = rd.ReadUnsigned(word_size);
	switch(word_size)
	{
	case WordSize16:
		reserved = rd.ReadUnsigned(word_size);
		relocations_suppressed = rd.ReadUnsigned(word_size);
		break;
	case WordSize32:
		code_relocation_size = rd.ReadUnsigned(word_size);
		data_relocation_size = rd.ReadUnsigned(word_size);
		break;
	}
}

void AOutFormat::ReadFile(Linker::Reader& rd)
{
	// note: bound EMX executables are not parsed here

	std::array<uint8_t, 4> signature;

	file_offset = rd.Tell();

	if(file_offset != 0 && system == UNSPECIFIED)
	{
		// probably a DJGPP executable
		system = DJGPP1;
	}

	offset_t file_end = rd.GetImageEnd();
	rd.ReadData(signature);

	if(file_offset == 0 && (system == UNSPECIFIED || system == DJGPP1)
	&& ((signature[0] == 'M' && signature[1] == 'Z')
		|| (signature[0] == 'Z' && signature[1] == 'M')))
	{
		// try to check for MZ stub
		rd.Seek(0);
		std::array<uint8_t, 2> magic;
		file_offset = Microsoft::FindActualSignature(rd, magic, "\7\1", "\10\1", "\13\1");
		rd.Seek(file_offset);
		rd.ReadData(signature);
	}

	switch(system)
	{
	case UNIX_V1:
		word_size = WordSize16;
		midmag_endiantype = endiantype = ::PDP11Endian;
		magic = magic_type(signature[0] | (signature[1] << 8));
		break;
	case UNSPECIFIED:
	case UNIX:
	case SYSTEM_III:
	case SYSTEM_V:
	case LINUX:
		if(word_size != 0)
		{
			// word_size and endiantype are already set
			midmag_endiantype = endiantype;
			if(!AttemptReadFile(rd, signature.data(), file_end - file_offset))
			{
				Linker::FatalError("Fatal error: Unable to determine file format");
			}
		}
		else
		{
			// TODO: this can probably be simplified
			// suggested algorithm:
			// if first two bytes are not a valid magic number, it is big endian (and thus 32-bit), otherwise little endian
			// if little endian, assume 32-bit and check file size, if too big, must be 16-bit
			// finally, mask out 0x03FF0000 (BSD) to get machine type, or 0x00FF0000 (Linux) if that fails, fallback to VAX

			// Big endian:
			// .. 01 .. ..
			// .. 02 .. ..
			// .. 03 .. ..
			// byte 2 must be 00 (QMAGIC) or 01

			// Little endian:
			// .. .. 64 ..
			// .. .. 67 ..
			// .. .. 97 ..
			// .. .. 98 ..
			// byte 1 must be 00 (QMAGIC) or 01

			if(signature[1] == 0x00 || signature[1] == 0x01)
			{
				/* could be multiple formats, make multiple attempts */
				endiantype = midmag_endiantype = ::LittleEndian;

				word_size = file_offset ? WordSize32 : WordSize16;
				/* if at beginning of file, first attempt 16-bit little endian (PDP-11, most likely input format) */
				if(!AttemptReadFile(rd, signature.data(), file_end - file_offset))
				{
					endiantype = midmag_endiantype = ::LittleEndian;
					word_size = file_offset ? WordSize16 : WordSize32;
					/* then attempt 32-bit little endian (Intel 80386, most likely format found on system) */
					if(!AttemptReadFile(rd, signature.data(), file_end - file_offset))
					{
						endiantype = midmag_endiantype = ::BigEndian;
						word_size = WordSize32;
						/* then attempt 32-bit big endian (Motorola 68000, most likely format if not little endian) */
						if(!AttemptReadFile(rd, signature.data(), file_end - file_offset))
						{
							endiantype = midmag_endiantype = ::BigEndian;
							word_size = WordSize16;
							/* finally, attempt 16-bit big endian (unsure if any ever supported UNIX with a.out) */
							if(!AttemptReadFile(rd, signature.data(), file_end - file_offset))
							{
								Linker::FatalError("Fatal error: Unable to determine file format");
							}
						}
					}
				}
			}
			else
			{
				word_size = WordSize32;

				/* magic value is unrecognizable, attempting to read a couple of common CPU types */
				switch(signature[1])
				{
				case MID_68010:
				case MID_68020:
					midmag_endiantype = ::BigEndian;
					cpu = M68K;
					break;
				case MID_LINUX_SPARC:
					midmag_endiantype = ::BigEndian;
					cpu = SPARC;
					break;
				default:
					switch(signature[2])
					{
					case MID_UNKNOWN:
						midmag_endiantype = ::LittleEndian;
						cpu = UNKNOWN;
						break;
					case MID_PC386:
						midmag_endiantype = ::LittleEndian;
						cpu = I386;
						break;
					case MID_BFD_ARM:
						midmag_endiantype = ::LittleEndian;
						cpu = ARM;
						break;
					case MID_MIPS1:
					case MID_MIPS2:
						midmag_endiantype = ::LittleEndian;
						cpu = MIPS;
						break;
					}
					break;
				}

				if(!AttemptFetchMagic(signature.data()))
				{
					Linker::FatalError("Fatal error: Unable to determine file format");
				}
			}

			endiantype = midmag_endiantype;
		}
		break;

	case FREEBSD:
		if(word_size != 0)
		{
			// word_size and endiantype are already set
			// assume that the midmag is in FreeBSD byte order, otherwise we would not have needed to set word_size and endiantype
			midmag_endiantype = ::LittleEndian;
			magic = magic_type(signature[0] | (signature[1] << 8));
			flags = signature[3];
			mid_value = signature[2] | (signature[3] << 8);
		}
		else
		{
			/* attempt little endian */
			word_size = WordSize32;
			endiantype = ::UndefinedEndian;

			uint16_t attempted_magic = signature[0] | (signature[1] << 8);
			if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
			{
				attempted_magic = signature[3] | (signature[2] << 8);
				if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
				{
					Linker::FatalError("Fatal error: Unable to determine file format");
				}
				else
				{
					midmag_endiantype = ::BigEndian;
					flags = signature[0];
					mid_value = signature[1] | (signature[0] << 8);
				}
			}
			else
			{
				midmag_endiantype = ::LittleEndian;
				flags = signature[3];
				mid_value = signature[2] | (signature[3] << 8);
			}
			magic = magic_type(attempted_magic);
		}

		flags &= 0xFC;
		mid_value &= 0x03FF;

		switch(mid_value)
		{
		case MID_UNKNOWN:
		default:
			cpu = UNKNOWN;
			break;
		case MID_68010:
		case MID_68020:
		case MID_HP200:
		case MID_HP300:
		case MID_HPUX:
			cpu = M68K;
			break;
		case MID_I386:
			cpu = I386;
			break;
		case MID_FREEBSD_SPARC:
			cpu = SPARC;
			break;
		case MID_ARM6:
			cpu = ARM;
			break;
		case MID_HPUX800:
			cpu = PARISC;
			break;
		}

		if(endiantype == ::UndefinedEndian)
			endiantype = GetEndianType();
		break;

	case NETBSD:
		if(word_size != 0)
		{
			// word_size and endiantype are already set
			// assume that the midmag is in NetBSD byte order, otherwise we would not have needed to set word_size and endiantype
			midmag_endiantype = ::LittleEndian;
			magic = magic_type(signature[3] | (signature[2] << 8));
			flags = signature[0];
			mid_value = signature[1] | (signature[0] << 8);
		}
		else
		{
			word_size = WordSize32;
			endiantype = ::UndefinedEndian;

			/* attempt big endian */
			uint16_t attempted_magic = signature[3] | (signature[2] << 8);
			if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
			{
				attempted_magic = signature[0] | (signature[1] << 8);
				if(attempted_magic != OMAGIC && attempted_magic != NMAGIC && attempted_magic != ZMAGIC && attempted_magic != QMAGIC)
				{
					Linker::FatalError("Fatal error: Unable to determine file format");
				}
				else
				{
					midmag_endiantype = ::LittleEndian;
					flags = signature[3];
					mid_value = signature[2] | (signature[3] << 8);
				}
			}
			else
			{
				midmag_endiantype = ::BigEndian;
				flags = signature[0];
				mid_value = signature[1] | (signature[0] << 8);
			}
			magic = magic_type(attempted_magic);
		}

		flags &= 0xFC;
		mid_value &= 0x03FF;

		page_size = 0;
		endiantype = ::UndefinedEndian;
		switch(mid_value)
		{
		case MID_UNKNOWN:
			cpu = UNKNOWN;
			endiantype = midmag_endiantype;
			break;
		default:
			cpu = UNKNOWN;
			// TODO: endianness
			break;
		case MID_68010:
		case MID_68020:
		case MID_HP200:
		case MID_HP300:
		case MID_HPUX:
			cpu = M68K;
			break;
		case MID_NETBSD_M680002K:
			cpu = M68K;
			page_size = 2 * 1000;
			break;
		case MID_NETBSD_M68K4K:
			cpu = M68K;
			page_size = 4 * 1000;
			break;
		case MID_NETBSD_M68K:
			cpu = M68K;
			page_size = 8 * 1000;
			break;
		case MID_PC386:
		case MID_I386:
			cpu = I386;
			break;
		case MID_NETBSD_NS32532K:
			cpu = NS32K;
			break;
		case MID_NETBSD_SPARC:
			cpu = SPARC;
			break;
		case MID_NETBSD_PMAX:
			cpu = MIPS;
			// TODO: endianness
			break;
		case MID_NETBSD_VAX:
			cpu = VAX;
			break;
		case MID_NETBSD_VAX1K:
			cpu = VAX;
			page_size = 1 * 1000;
			break;
		case MID_NETBSD_ALPHA:
			cpu = ALPHA;
			// TODO: endianness
			break;
		case MID_NETBSD_MIPS:
			cpu = MIPS;
			endiantype = ::BigEndian;
			break;
		case MID_ARM6:
			cpu = ARM;
			break;
		case MID_NETBSD_SH3:
			cpu = SUPERH;
			break;
		case MID_NETBSD_POWERPC64:
			cpu = PPC64;
			break;
		case MID_NETBSD_POWERPC:
			cpu = PPC;
			break;
		case MID_MIPS1:
		case MID_MIPS2:
			cpu = MIPS;
			break;
		case MID_NETBSD_M88K:
			cpu = M88K;
			break;
		case MID_NETBSD_HPPA:
		case MID_HPUX800:
			cpu = PARISC;
			break;
		case MID_NETBSD_SH5_64:
			cpu = SUPERH64;
			// TODO: endianness
			break;
		case MID_NETBSD_SPARC64:
			cpu = SPARC64;
			break;
		case MID_NETBSD_X86_64:
			cpu = AMD64;
			break;
		case MID_NETBSD_SH5_32:
			cpu = SUPERH;
			// TODO: endianness
			break;
		case MID_NETBSD_IA64:
			cpu = IA64;
			// TODO: endianness
			break;
		case MID_NETBSD_AARCH64:
			cpu = AARCH64;
			break;
		case MID_NETBSD_OR1K:
			cpu = OR1K;
			// TODO: endianness
			break;
		case MID_NETBSD_RISCV:
			cpu = RISCV;
			// TODO: endianness
			break;
		}

		if(endiantype == ::UndefinedEndian)
			endiantype = GetEndianType();

		GetPageSize();
		break;

	case DJGPP1:
	case PDOS386:
	case EMX:
		word_size = WordSize32;
		endiantype = midmag_endiantype = ::LittleEndian;
		cpu = I386;

		magic = magic_type(signature[0] | (signature[1] << 8));
		mid_value = signature[2];
		flags = signature[3];
		break;
	}

	Linker::Debug << "Debug: a.out endian type: " << endiantype << ", CPU type: " << cpu << ", magic value: " << magic << ", word size: " << word_size << std::endl;

	rd.endiantype = endiantype;

	rd.Seek(file_offset + word_size);

	ReadHeader(rd);

	// we will skip the header, even if it is included in the text section
	uint32_t text_offset = std::max(GetHeaderSize(), GetTextOffset());
	rd.Seek(file_offset + text_offset);
	code = Linker::Section::ReadFromFile(rd, code_size + (text_offset - GetTextOffset()), ".text");

	uint32_t data_offset = AlignTo(GetTextOffset() + code_size, GetDataOffsetAlign());
	rd.Seek(file_offset + data_offset);
	data = Linker::Section::ReadFromFile(rd, data_size, ".data");

	switch(word_size)
	{
	case WordSize16:
		/* this is only for PDP-11 */
		if(relocations_suppressed == 0)
		{
			/* indicates that relocations are present */
			int source_segment = 0;
			for(size_t i = 0; i < code_size + data_size; i += 2)
			{
				if(i == code_size)
				{
					source_segment = 1;
				}

				uint16_t offset = source_segment == 0 ? i : i - code_size;
				Relocation relocation = Relocation::ReadFile16Bit(rd, offset);

				Linker::Debug << "Debug: Offset " << std::hex << i << " relocation " /*<< std::hex << value*/ << std::endl;

				switch(source_segment)
				{
				case 0:
					/* text */
					code_relocations.push_back(relocation);
					break;
				case 1:
					/* data */
					data_relocations.push_back(relocation);
					break;
				}
			}
		}

		if(symbol_table_size != 0)
		{
			for(size_t i = 0; i < symbol_table_size; i += 8)
			{
				Symbol symbol;
				symbol.unknown = rd.ReadUnsigned(2);
				symbol.name_offset = rd.ReadUnsigned(2);
				symbol.type = rd.ReadUnsigned(2);
				symbol.value = rd.ReadUnsigned(2);
				symbols.push_back(symbol);
			}

			offset_t string_table_offset = code_size + data_size;
			if(relocations_suppressed == 0)
			{
				string_table_offset *= 2;
			}
			string_table_offset += file_offset + 0x10 + symbol_table_size;
			Linker::Debug << "Debug: String table offset " << std::hex << string_table_offset << std::endl;
			for(auto& symbol : symbols)
			{
				rd.Seek(string_table_offset + symbol.name_offset);
				symbol.name = rd.ReadASCIIZ();
				Linker::Debug << "Debug: Symbol " << symbol.name << " (offset " << symbol.name_offset << ") of type " << symbol.type << " value " << symbol.value << std::endl;
			}
		}
		break;
	case WordSize32:
		/* Linux a.out */

		code_relocations.clear();
		for(uint32_t rel_off = 0; rel_off < code_relocation_size; rel_off += 8)
		{
			Relocation relocation = Relocation::ReadFile32Bit(rd);
			code_relocations.push_back(relocation);
		}

		data_relocations.clear();
		for(uint32_t rel_off = 0; rel_off < data_relocation_size; rel_off += 8)
		{
			Relocation relocation = Relocation::ReadFile32Bit(rd);
			data_relocations.push_back(relocation);
		}

		// TODO: symbol table
		break;
	}

	/* this is required for module generation */
	bss = std::make_shared<Linker::Section>(".bss");
}

offset_t AOutFormat::ImageSize() const
{
	switch(word_size)
	{
	case WordSize16:
		return 8 * word_size + (code->ImageSize() + data->ImageSize()) * (relocations_suppressed == 0 ? 2 : 1);
	case WordSize32:
		return 8 * word_size + code->ImageSize() + data->ImageSize() + code_relocations.size() * 8 + data_relocations.size() * 8;
	default:
		assert(false);
	}
}

void AOutFormat::WriteHeader(Linker::Writer& wr) const
{
	wr.WriteWord(word_size, magic | (uint32_t(mid_value) << 16) | (uint32_t(flags) << 24));
	wr.WriteWord(word_size, code_size);
	wr.WriteWord(word_size, data_size);
	wr.WriteWord(word_size, bss_size);
	wr.WriteWord(word_size, symbol_table_size);
	wr.WriteWord(word_size, entry_address);
	switch(word_size)
	{
	case WordSize16:
		wr.WriteWord(word_size, reserved);
		wr.WriteWord(word_size, relocations_suppressed);
		break;
	case WordSize32:
		wr.WriteWord(word_size, code_relocation_size);
		wr.WriteWord(word_size, data_relocation_size);
		break;
	}
}

offset_t AOutFormat::WriteFile(Linker::Writer& wr) const
{
	if(system == DJGPP1 && stub.filename != "")
	{
		stub.WriteStubImage(wr);
	}

	wr.endiantype = endiantype;

	WriteHeader(wr);

	// we will skip the header, even if it is included in the text section
	uint32_t text_offset = std::max(GetHeaderSize(), GetTextOffset());
	wr.Seek(file_offset + text_offset);

	code->WriteFile(wr);

	uint32_t data_offset = AlignTo(GetTextOffset() + code_size, GetDataOffsetAlign());
	wr.Seek(file_offset + data_offset);

	data->WriteFile(wr);

	// TODO: 16-bit relocations

	for(auto& rel : code_relocations)
	{
		rel.WriteFile32Bit(wr);
	}

	for(auto& rel : data_relocations)
	{
		rel.WriteFile32Bit(wr);
	}

	// TODO: symbol table

	return ImageSize();
}


void AOutFormat::Dump(Dumper::Dumper& dump) const
{
	switch(system)
	{
	default:
	case UNSPECIFIED:
	case LINUX:
	case FREEBSD:
	case NETBSD:
		dump.SetEncoding(Dumper::Block::encoding_default);
		break;
	case DJGPP1:
	case EMX:
	case PDOS386:
		dump.SetEncoding(Dumper::Block::encoding_cp437);
		break;
	}

	dump.SetTitle("UNIX a.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 2 * word_size);

	static const std::map<offset_t, std::string> system_descriptions =
	{
		{ LINUX, "Linux (pre-1.2)" },
		{ FREEBSD, "FreeBSD (pre-3.0)" },
		{ NETBSD, "NetBSD (pre-1.5)" },
		{ DJGPP1, "DJGPP (pre-1.11)" },
		{ PDOS386, "PDOS386" },
		{ EMX, "EMX" },
	};
	file_region.AddOptionalField("Parsing as", Dumper::ChoiceDisplay::Make(system_descriptions), offset_t(system));
	static const std::map<offset_t, std::string> wordsize_descriptions =
	{
		{ 2, "16-bit" },
		{ 4, "32-bit" },
	};
	file_region.AddField("Word size", Dumper::ChoiceDisplay::Make(wordsize_descriptions), offset_t(word_size));
	static const std::map<offset_t, std::string> endiantype_descriptions =
	{
		{ ::LittleEndian, "little endian" },
		{ ::BigEndian, "big endian" },
	};
	file_region.AddField("Endianness", Dumper::ChoiceDisplay::Make(endiantype_descriptions), offset_t(endiantype));
	file_region.Display(dump);

	Dumper::Region header_region("Header", file_offset, GetHeaderSize(), 2 * word_size);
	static const std::map<offset_t, std::string> magic_descriptions =
	{
		{ OMAGIC, "\"OMAGIC\"" },
		{ NMAGIC, "\"NMAGIC\"" },
		{ ZMAGIC, "\"ZMAGIC\"" },
		{ QMAGIC, "\"QMAGIC\"" },
	};
	header_region.AddField("File type", Dumper::ChoiceDisplay::Make(magic_descriptions, Dumper::HexDisplay::Make(4)), offset_t(magic));
	if(word_size == WordSize32)
	{
		static const std::map<offset_t, std::string> freebsd_midmag_descriptions =
		{
			{ ::LittleEndian, "FreeBSD order (little endian)" },
			{ ::BigEndian, "NetBSD order (big endian)" },
		};

		static const std::map<offset_t, std::string> netbsd_midmag_descriptions =
		{
			{ ::LittleEndian, "little endian" },
			{ ::BigEndian, "NetBSD order (big endian)" },
		};

		static const std::map<offset_t, std::string> linux_mid_descriptions =
		{
			{ MID_LINUX_OLDSUN2, "Old SUN2" },
			{ MID_68010, "68010" },
			{ MID_68020, "68020" },
			{ MID_LINUX_SPARC, "SPARC" },
			{ MID_PC386, "386" },
			{ MID_MIPS1, "MIPS1" },
			{ MID_MIPS2, "MIPS2" },
		};

		static const std::map<offset_t, std::string> freebsd_mid_descriptions =
		{
			{ MID_UNKNOWN, "unknown" },
			{ MID_68010, "68010" },
			{ MID_68020, "68020" },
			{ MID_I386, "I386" },
			{ MID_FREEBSD_SPARC, "SPARC" },
			{ MID_ARM6, "ARM6" },
			{ MID_HP200, "HP200" },
			{ MID_HP300, "HP300" },
			{ MID_HPUX800, "HPUX800" },
			{ MID_HPUX, "HPUX" },
		};

		static const std::map<offset_t, std::string> netbsd_mid_descriptions =
		{
			{ MID_UNKNOWN, "unknown" },
			{ MID_68010, "SUN010" },
			{ MID_68020, "SUN020" },
			{ MID_PC386, "PC386" },
			{ MID_I386, "I386 (BSD binary)" },
			{ MID_NETBSD_M68K, "M68K (with 8K pages, BSD binary)" },
			{ MID_NETBSD_M68K4K, "M68K4K (M68K with 4K pages, BSD binary)" },
			{ MID_NETBSD_NS32532K, "NS32532" },
			{ MID_NETBSD_SPARC, "SPARC" },
			{ MID_NETBSD_PMAX, "PMAX" },
			{ MID_NETBSD_VAX1K, "VAX (with 1K pages)" },
			{ MID_NETBSD_ALPHA, "ALPHA (BSD binary)" },
			{ MID_NETBSD_MIPS, "MIPS (big-endian)" },
			{ MID_ARM6, "ARM6" },
			{ MID_NETBSD_M680002K, "M680002K (M68000 with 2K pages)" },
			{ MID_NETBSD_SH3, "SH3" },
			{ MID_NETBSD_POWERPC64, "POWERPC64 (big-endian)" },
			{ MID_NETBSD_POWERPC, "POWERPC (big-endian)" },
			{ MID_NETBSD_VAX, "VAX" },
			{ MID_MIPS1, "MIPS1" },
			{ MID_MIPS2, "MIPS2" },
			{ MID_NETBSD_M88K, "M88K (BSD binary)" },
			{ MID_NETBSD_HPPA, "HPPA (HP PA-RISC)" },
			{ MID_NETBSD_SH5_64, "SH5_64 (LP64)" },
			{ MID_NETBSD_SPARC64, "SPARC64 (LP64)" },
			{ MID_NETBSD_X86_64, "X86_64" },
			{ MID_NETBSD_SH5_32, "SH5_32 (ILP32)" },
			{ MID_NETBSD_IA64, "IA64 (Itanium)" },
			{ MID_NETBSD_AARCH64, "AARCH64" },
			{ MID_NETBSD_OR1K, "OR1K (OpenRISC 1000)" },
			{ MID_NETBSD_RISCV, "RISCV" },
			{ MID_FREEBSD_SPARC, "SPARC" },
			{ MID_HP200, "HP200" },
			{ MID_HP300, "HP300" },
			{ MID_HPUX800, "HPUX800" },
			{ MID_HPUX, "HPUX" },
		};

		switch(system)
		{
		case DJGPP1:
		case EMX:
		case PDOS386:
		case UNIX_V1:
		case UNIX:
		case SYSTEM_III:
		case SYSTEM_V:
			break;
		case UNSPECIFIED:
		case LINUX:
			header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(linux_mid_descriptions, Dumper::HexDisplay::Make(2)), offset_t(mid_value & 0xFF));
			header_region.AddOptionalField("Flags", Dumper::HexDisplay::Make(2), offset_t(flags & 0xFF));
			break;
		case FREEBSD:
			header_region.AddField("Midmag byte order", Dumper::ChoiceDisplay::Make(freebsd_midmag_descriptions), offset_t(midmag_endiantype));
			header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(freebsd_mid_descriptions, Dumper::HexDisplay::Make(2)), offset_t(mid_value & 0xFF));
			header_region.AddOptionalField("Flags", Dumper::HexDisplay::Make(2), offset_t(flags & 0xFC));
			// TODO: describe flags
			break;
		case NETBSD:
			header_region.AddField("Midmag byte order", Dumper::ChoiceDisplay::Make(netbsd_midmag_descriptions), offset_t(midmag_endiantype));
			header_region.AddField("Machine type", Dumper::ChoiceDisplay::Make(netbsd_mid_descriptions, Dumper::HexDisplay::Make(2)), offset_t(mid_value & 0xFF));
			header_region.AddOptionalField("Flags", Dumper::HexDisplay::Make(2), offset_t(flags & 0xFC));
			// TODO: describe flags
			break;
		}
	}
	header_region.AddField("Entry", Dumper::HexDisplay::Make(2 * word_size), offset_t(entry_address));
	header_region.Display(dump);

	// TODO: print symbols, strings, relocations

	Dumper::Block text_block("Text", GetTextOffset(), code->AsImage(), GetTextAddress(), 2 * word_size, 2 * word_size);
	for(auto& rel : code_relocations)
	{
		if(rel.segment != Relocation::Absolute)
			text_block.AddSignal(rel.address, rel.size);
	}
	text_block.Display(dump);

	uint32_t data_offset = AlignTo(GetTextOffset() + code->ImageSize(), GetDataOffsetAlign());
	uint32_t data_address = AlignTo(GetTextAddress() + code->ImageSize(), GetDataAddressAlign());

	Dumper::Block data_block("Data", data_offset, data->AsImage(), data_address, 2 * word_size, 2 * word_size);
	for(auto& rel : data_relocations)
	{
		if(rel.segment != Relocation::Absolute)
			data_block.AddSignal(rel.address, rel.size);
	}
	data_block.Display(dump);

	Dumper::Region bss_region("BSS", data_offset + data->ImageSize(), bss_size, 2 * word_size);
	bss_region.AddField("Address", Dumper::HexDisplay::Make(2 * word_size), data_address + data->ImageSize());
	bss_region.Display(dump);

	static const std::map<offset_t, std::string> source_segment_description =
	{
		{ 0, "Text" },
		{ 1, "Data" },
	};

	static const std::map<offset_t, std::string> target_segment_description =
	{
		{ Relocation::Undefined, "undefined" },
		{ Relocation::External, "external" },
		{ Relocation::Absolute, "absolute" },
		{ Relocation::Text, "text" },
		{ Relocation::Data, "data" },
		{ Relocation::Bss, "bss" },
		{ Relocation::FileName, "filename" },
		{ Relocation::Illegal, "illegal entry" },
	};

	uint32_t rel_index = 0;

	offset_t relocation_offset = data_offset + data->ImageSize();
	offset_t text_size = word_size == WordSize32 ? code_relocations.size() * 8 : relocations_suppressed == 0 ? code->ImageSize() : 0;
	if(text_size != 0)
	{
		Dumper::Region text_rel_region("Text relocation region", relocation_offset, code_relocations.size() * (word_size == WordSize16 ? 2 : 8), 2 * word_size);
		text_rel_region.Display(dump);

		for(auto& rel : code_relocations)
		{
			Dumper::Entry rel_entry("Relocation", rel_index + 1, 2 * word_size);
			rel_entry.AddField("File offset", Dumper::HexDisplay::Make(2 * word_size), offset_t(relocation_offset + rel_index * (word_size == WordSize16 ? 2 : 8)));
			rel_entry.AddField("Source segment", Dumper::ChoiceDisplay::Make(source_segment_description), offset_t(0));
			rel_entry.AddField("Source offset", Dumper::HexDisplay::Make(2 * word_size), offset_t(rel.address));
			rel_entry.AddOptionalField("Symbol index", Dumper::DecDisplay::Make(), offset_t(rel.symbol));
			rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make("relative", "absolute"), offset_t(rel.relative));
			rel_entry.AddField("Target", Dumper::ChoiceDisplay::Make(target_segment_description), offset_t(rel.segment));
			rel_entry.AddField("Size", Dumper::DecDisplay::Make(" bytes"), offset_t(rel.size));
			// TODO: literal_entry
			rel_entry.Display(dump);
			rel_index ++;
		}
	}

	offset_t data_size = word_size == WordSize32 ? data_relocations.size() * 8 : relocations_suppressed == 0 ? data->ImageSize() : 0;
	if(data_size != 0)
	{
		Dumper::Region data_rel_region("Data relocation region", relocation_offset + code_relocations.size() * (word_size == WordSize16 ? 2 : 8), data_relocations.size() * (word_size == WordSize16 ? 2 : 8), 2 * word_size);
		data_rel_region.Display(dump);

		for(auto& rel : data_relocations)
		{
			Dumper::Entry rel_entry("Relocation", rel_index + 1, 2 * word_size);
			rel_entry.AddField("File offset", Dumper::HexDisplay::Make(2 * word_size), offset_t(relocation_offset + rel_index * (word_size == WordSize16 ? 2 : 8)));
			rel_entry.AddField("Source segment", Dumper::ChoiceDisplay::Make(source_segment_description), offset_t(1));
			rel_entry.AddField("Source offset", Dumper::HexDisplay::Make(2 * word_size), offset_t(rel.address));
			rel_entry.AddOptionalField("Symbol index", Dumper::DecDisplay::Make(), offset_t(rel.symbol));
			rel_entry.AddField("Type", Dumper::ChoiceDisplay::Make("relative", "absolute"), offset_t(rel.relative));
			rel_entry.AddField("Target", Dumper::ChoiceDisplay::Make(target_segment_description), offset_t(rel.segment));
			rel_entry.AddField("Size", Dumper::DecDisplay::Make(" bytes"), offset_t(rel.size));
			// TODO: literal_entry
			rel_entry.Display(dump);
			rel_index ++;
		}
	}
}

/* * * Reader * * */

std::shared_ptr<AOutFormat> AOutFormat::CreateReader(word_size_t word_size, ::EndianType endiantype, system_type system)
{
	std::shared_ptr<AOutFormat> reader = std::make_shared<AOutFormat>();
	reader->word_size = word_size;
	reader->endiantype = endiantype;
	reader->system = system;
	return reader;
}

void AOutFormat::GenerateModule(Linker::Module& module) const
{
	switch(cpu)
	{
	case M68K:
		module.cpu = Linker::Module::M68K;
		break;
	case I386:
		module.cpu = /*option_16bit ? Linker::Module::I86 :*/ Linker::Module::I386; /* TODO */
		break;
	case SPARC:
		module.cpu = Linker::Module::SPARC;
		break;
	case ARM:
		module.cpu = Linker::Module::ARM;
		break;
	case MIPS:
		module.cpu = Linker::Module::MIPS;
		break;
	case UNKNOWN:
		if(word_size == WordSize16)
		{
			if(endiantype == ::LittleEndian || endiantype == ::PDP11Endian)
			{
				/* make a guess */
				module.cpu = Linker::Module::PDP11;
			}
		}
		else
		{
			if(endiantype == ::BigEndian)
			{
				module.cpu = Linker::Module::M68K;
			}
		}
		break;
	default:
		/* unknown CPU */
		break;
	}

	std::shared_ptr<Linker::Section> linker_code, linker_data, linker_bss;
	linker_code = std::dynamic_pointer_cast<Linker::Section>(code);
	linker_data = std::dynamic_pointer_cast<Linker::Section>(data);
	linker_bss  = std::dynamic_pointer_cast<Linker::Section>(bss);

	linker_bss->SetZeroFilled(true);
	linker_bss->Expand(bss_size);

	linker_code->SetReadable(true);
	linker_data->SetReadable(true);
	linker_bss->SetReadable(true);

	linker_code->SetExecutable(true);
	linker_data->SetWritable(true);
	linker_bss->SetWritable(true);

	module.AddSection(linker_code);
	module.AddSection(linker_data);
	module.AddSection(linker_bss);

	for(const Symbol& symbol : symbols)
	{
		offset_t offset;
		std::shared_ptr<Linker::Section> section;
		switch(symbol.type)
		{
		case 0x20:
			/* external or common */
			if(symbol.value != 0)
			{
				module.AddCommonSymbol(Linker::SymbolDefinition::CreateCommon(symbol.name, "", symbol.value, 1 /* TODO: alignment */));
			}
			continue;
		case 0x01:
			/* absolute */
			offset = symbol.value;
			section = nullptr;
			break;
		case 0x22:
		case 0x02:
			/* text based */
			offset = symbol.value;
			section = linker_code;
			break;
		case 0x23:
		case 0x03:
			/* data based */
			offset = symbol.value - code_size;
			section = linker_data;
			break;
		case 0x24:
		case 0x04:
			/* bss based */
			offset = symbol.value - code_size - data_size;
			section = linker_bss;
			break;
		default:
			Linker::Error << "Internal error: unhandled symbol type" << std::endl;
			continue;
		}
		Linker::Debug << "Debug: Add symbol " << symbol.name << ": " << Linker::Location(section, offset) << std::endl;
		module.AddGlobalSymbol(symbol.name, Linker::Location(section, offset));
	}

	for(int i = 0; i < 2; i++)
	{
		std::shared_ptr<Linker::Section> section = i == 0 ? linker_code : linker_data;
		for(auto& rel : i == 0 ? code_relocations : data_relocations)
		{
			/* TODO: only PDP-11 specific */
			Linker::Location rel_source = Linker::Location(section, rel.address);
			Linker::Target rel_target;
			uint32_t base = section->ReadUnsigned(2, rel.address); // 6
			bool is_relative = rel.relative;
			if(is_relative)
			{
				base += rel.address; // E
				if(i == 1)
					base += code_size;
			}
			uint32_t addend = 0;

			/* the values stored in the object file are already relative to the start of the code segment */
			switch(rel.segment)
			{
			case Relocation::Absolute:
				/* ignore */
				continue;
			case Relocation::Text:
				/* text based variable */
				rel_target = Linker::Target(Linker::Location(linker_code, base));
				break;
			case Relocation::Data:
				/* data based variable */
				rel_target = Linker::Target(Linker::Location(linker_data, base - code_size));
				break;
			case Relocation::Bss:
				/* bss based variable */
				rel_target = Linker::Target(Linker::Location(linker_bss, base - code_size - data_size));
				break;
			case Relocation::External:
				/* external symbol */
				{
					const Symbol& symbol = symbols[rel.symbol];
#if 0
					if(symbol.name == ".text")
					{
						rel_target = Linker::Target(Linker::Location(linker_code, base), true);
					}
					else if(symbol.name == ".data")
					{
						rel_target = Linker::Target(Linker::Location(linker_data, base), true);
					}
					else if(symbol.name == ".bss")
					{
						rel_target = Linker::Target(Linker::Location(linker_bss, base), true);
					}
					else
#endif
					{
						addend = -2;
						rel_target = Linker::Target(Linker::SymbolName(symbol.name));
					}
				}
				break;
			default:
				Linker::Error << "Internal error: Invalid relocation segment" << std::endl;
				continue; // TODO: show error message
			}

			Linker::Relocation obj_rel =
				is_relative
				? Linker::Relocation::Relative(2, rel_source, rel_target, addend, ::LittleEndian)
				: Linker::Relocation::Absolute(2, rel_source, rel_target, addend, ::LittleEndian);
			//obj_rel.AddCurrentValue();
			module.AddRelocation(obj_rel);
			Linker::Debug << "Debug: a.out relocation " << obj_rel << std::endl;
		}
	}
}

/* * * Writer * * */

std::shared_ptr<AOutFormat> AOutFormat::CreateWriter(system_type system, magic_type magic)
{
	std::shared_ptr<AOutFormat> writer = std::make_shared<AOutFormat>();
	writer->system = system;
	writer->magic = magic;
	return writer;
}

std::shared_ptr<AOutFormat> AOutFormat::CreateWriter(system_type system)
{
	return CreateWriter(system, GetDefaultMagic(system));
}

static Linker::OptionDescription<offset_t> p_code_base_address("code_base_address", "Starting address of code section");
static Linker::OptionDescription<offset_t> p_data_align("data_align", "Alignment of data section");
static Linker::OptionDescription<offset_t> p_code_end_align("code_end_align", "Alignment of end of code section");
static Linker::OptionDescription<offset_t> p_data_end_align("data_end_align", "Alignment of end of data section");
static Linker::OptionDescription<offset_t> p_bss_end_align("bss_end_align", "Alignment of end of bss section");

std::vector<Linker::OptionDescription<void> *> AOutFormat::ParameterNames =
{
	&p_code_base_address,
	&p_data_align,
	&p_code_end_align,
	&p_data_end_align,
	&p_bss_end_align,
};

std::vector<Linker::OptionDescription<void> *> AOutFormat::GetLinkerScriptParameterNames()
{
	return ParameterNames;
}

std::shared_ptr<Linker::OptionCollector> AOutFormat::GetOptions()
{
	return std::make_shared<AOutOptionCollector>();
}

void AOutFormat::SetOptions(std::map<std::string, std::string>& options)
{
	AOutOptionCollector collector;
	collector.ConsiderOptions(options);
	stub.filename = collector.stub();

	/* TODO: other parameters */
}

void AOutFormat::OnNewSegment(std::shared_ptr<Linker::Segment> segment)
{
	if(segment->name == ".code")
	{
		code = segment;
	}
	else if(segment->name == ".data")
	{
		data = segment;
	}
	else if(segment->name == ".bss")
	{
		bss = segment;
	}
	else
	{
		Linker::Error << "Error: unknown segment `" << segment->name << "` for format, ignoring" << std::endl;
	}
}

void AOutFormat::CreateDefaultSegments()
{
	if(code == nullptr)
	{
		code = std::make_shared<Linker::Segment>(".code");
	}
	if(data == nullptr)
	{
		data = std::make_shared<Linker::Segment>(".data");
	}
	if(bss == nullptr)
	{
		bss = std::make_shared<Linker::Segment>(".bss");
	}
}

std::unique_ptr<Script::List> AOutFormat::GetScript(Linker::Module& module)
{
	static const char * SimpleScript = R"(
".code"
{
	at ?code_base_address?;
	all not write align ?align?;
	align ?code_end_align?;
};

".data"
{
	at align(here, ?data_align?);
	all not zero align ?align?;
	align ?data_end_align?;
};

".bss"
{
	all align ?align?;
	align ?bss_end_align?;
};
)";

	if(linker_script != "")
	{
		return SegmentManager::GetScript(module);
	}
	else
	{
		return Script::parse_string(SimpleScript);
	}
}

void AOutFormat::Link(Linker::Module& module)
{
	std::unique_ptr<Script::List> script = GetScript(module);

	ProcessScript(script, module);

	CreateDefaultSegments();
}

std::shared_ptr<Linker::Segment> AOutFormat::GetCodeSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(code);
}

std::shared_ptr<Linker::Segment> AOutFormat::GetDataSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(data);
}

std::shared_ptr<Linker::Segment> AOutFormat::GetBssSegment()
{
	return std::dynamic_pointer_cast<Linker::Segment>(bss);
}

void AOutFormat::ProcessModule(Linker::Module& module)
{
	uint32_t text_address = GetTextAddress();
	if(GetTextOffset() < GetHeaderSize())
		text_address += GetHeaderSize() - GetTextOffset();

	linker_parameters["code_base_address"] = Linker::Location(text_address);
	switch(system)
	{
	case DJGPP1:
		linker_parameters["code_end_align"] = Linker::Location(0x1000);
		linker_parameters["data_end_align"] = Linker::Location(0x1000);
		linker_parameters["bss_end_align"] = Linker::Location(0x1000);
		linker_parameters["align"] = Linker::Location(4); // TODO
		break;
	case EMX:
		linker_parameters["code_end_align"] = Linker::Location(0x1000);
		linker_parameters["data_end_align"] = Linker::Location(1);
		linker_parameters["bss_end_align"] = Linker::Location(1);
		linker_parameters["align"] = Linker::Location(1);
		break;
	case PDOS386:
	default:
		linker_parameters["code_end_align"] = Linker::Location(4); /* TODO: is this needed? */
		linker_parameters["data_end_align"] = Linker::Location(4); /* TODO: is this needed? */
		linker_parameters["bss_end_align"] = Linker::Location(4); /* TODO: is this needed? */
		linker_parameters["align"] = Linker::Location(4); /* TODO: is this needed? */
		break;
	}
	linker_parameters["data_align"] = Linker::Location(GetDataAddressAlign());

	Link(module);

	std::map<uint32_t, Relocation> code_relocation_map, data_relocation_map; /* only used by PDOS386 OMAGIC */

	for(Linker::Relocation& rel : module.GetRelocations())
	{
		Linker::Resolution resolution;
		if(!rel.Resolve(module, resolution))
		{
			Linker::Error << "Error: Unable to resolve relocation: " << rel << ", ignoring" << std::endl;
		}
		rel.WriteWord(resolution.value);
		if(system == PDOS386 && resolution.target != nullptr && resolution.reference == nullptr)
		{
			Relocation relocation;
			if(resolution.target == code)
				relocation.segment = Relocation::Text;
			else if(resolution.target == data)
				relocation.segment = Relocation::Data;
			else if(resolution.target == bss)
				relocation.segment = Relocation::Bss;
			else
				Linker::FatalError("Internal error: invalid target segment for relocation");
			if(rel.size != 1 && rel.size != 2 && rel.size != 4 && rel.size != 8)
			{
				Linker::Error << "Error: format does not support " << (rel.size << 3) << "-bit relocations, ignoring" << std::endl;
				continue;
			}
			relocation.relative = rel.IsRelative();
			relocation.size = rel.size;
			if(rel.size != 4)
			{
				Linker::Warning << "Warning: PDOS/386 only supports 32-bit relocations, generating anyway" << std::endl;
			}
			Linker::Position position = rel.source.GetPosition();
			if(position.segment == code)
			{
				relocation.address = position.address - GetCodeSegment()->base_address;
				code_relocation_map[relocation.address] = relocation;
			}
			else if(position.segment == data)
			{
				relocation.address = position.address - GetDataSegment()->base_address;
				data_relocation_map[relocation.address] = relocation;
			}
			else if(position.segment != nullptr)
			{
				Linker::Error << "Error: Unable to generate relocation for segment `" << position.segment->name << "', ignoring" << std::endl;
			}
			else
			{
				Linker::Error << "Error: Unable to generate global relocation, ignoring" << std::endl;
			}
		}
	}

	// TODO: this only works for 32-bit formats
	for(auto it : code_relocation_map)
	{
		code_relocations.push_back(it.second);
	}

	for(auto it : data_relocation_map)
	{
		data_relocations.push_back(it.second);
	}

	Linker::Location stack_top;
	if(module.FindGlobalSymbol(".stack_top", stack_top))
	{
		Linker::Warning << "Warning: setting stack top not supported, ignoring" << std::endl;
	}

	/* TODO: can it be changed? (check on DJGPP and PDOS386 as well!) */
	Linker::Location entry;
	if(module.FindGlobalSymbol(".entry", entry))
	{
		entry_address = entry.GetPosition().address;
	}
	else
	{
		entry_address = GetCodeSegment()->base_address;
		Linker::Warning << "Warning: no entry point specified, using beginning of .code segment" << std::endl;
	}
}

void AOutFormat::CalculateValues()
{
	if(system == DJGPP1 && stub.filename != "")
	{
		file_offset = stub.GetStubImageSize();
	}

	endiantype = GetEndianType();
	word_size = GetWordSize();
	code_size = GetCodeSegment()->data_size;
	if(GetTextOffset() < GetHeaderSize())
	{
		// include header in code size as well
		code_size += GetHeaderSize() - GetTextOffset();
	}

	if(system != EMX)
	{
		data_size = GetDataSegment()->data_size;
		bss_size  = GetBssSegment()->zero_fill;
	}
	else
	{
		// align end of data section
		data_size = AlignTo(GetDataSegment()->data_size, 0x1000);
		// bss must follow the data immediately, fit as much of bss into the data section as we can
		offset_t data_size_extra = data_size - GetDataSegment()->data_size;
		bss_size = AlignTo(std::max(GetBssSegment()->zero_fill, data_size_extra) - data_size_extra, 8);
	}
	symbol_table_size = 0;
	switch(word_size)
	{
	case WordSize16:
		reserved = 0;
		if(code_relocations.size() != 0 || data_relocations.size() != 0)
		{
			relocations_suppressed = 0;
		}
		break;
	case WordSize32:
		code_relocation_size = code_relocations.size() * 8;
		data_relocation_size = data_relocations.size() * 8;
		break;
	}
}

void AOutFormat::GenerateFile(std::string filename, Linker::Module& module)
{
	if(system == DEFAULT)
	{
		switch(module.cpu)
		{
		case Linker::Module::I386:
			if(magic == OMAGIC)
				system = PDOS386; // TODO: should this really be the default?
			else
				system = DJGPP1; // TODO: should this really be the default?
			break;
		case Linker::Module::PDP11:
		case Linker::Module::VAX:
			system = UNIX;
			break;
		case Linker::Module::M68K:
			// TODO: set to SUNOS
		default:
			Linker::Error << "Error: no system specified, defaulting to UNIX Version 7" << std::endl;
			system = UNIX;
			break;
		}
	}

	switch(module.cpu)
	{
	case Linker::Module::I386:
		cpu = I386;
		switch(system)
		{
		default:
		case LINUX:
		case EMX:
		case PDOS386:
			mid_value = MID_PC386;
			break;
		case NETBSD:
		case FREEBSD:
			mid_value = MID_I386;
			break;
		case DJGPP1:
			mid_value = 0;
			break;
		}
		flags = 0;
		break;
	case Linker::Module::M68K:
		cpu = M68K;
		mid_value = MID_68020; /* TODO: maybe M68010 is enough? */
		flags = 0;
		break;
	default:
		Linker::Error << "Error: Format only supports Intel 80386 and Motorola 68000 binaries" << std::endl;
	}

	word_size = GetWordSize();
	Linker::OutputFormat::GenerateFile(filename, module);
}

std::string AOutFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	switch(system)
	{
	case DJGPP1:
	case PDOS386:
		return filename + ".exe";
	case LINUX:
	case FREEBSD:
	case NETBSD:
		return filename;
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

std::string AOutFormat::GetDefaultExtension(Linker::Module& module) const
{
	switch(system)
	{
	case DJGPP1:
	case PDOS386:
		return "a.exe";
	case LINUX:
	case FREEBSD:
	case NETBSD:
		return "a.out";
	default:
		Linker::FatalError("Internal error: invalid target system");
	}
}

