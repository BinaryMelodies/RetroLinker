#ifndef COFF_H
#define COFF_H

#include <array>
#include <map>
#include "cpm68k.h"
#include "mzexe.h"
#include "../common.h"
#include "../linker/module.h"
#include "../linker/options.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

namespace COFF
{
	/**
	 * @brief The UNIX COFF file format
	 *
	 * Originally introduced in UNIX System V, it replaced the previous a.out format.
	 * It was later adopted, often with extensions, on many UNIX and non-UNIX operating systems.
	 *
	 * The current implementation supports the following formats:
	 * - DJGPP version 1.11 or later, running on top of MS-DOS
	 * - Digital Research Concurrent DOS 68K
	 */
	class COFFFormat : public virtual Linker::InputFormat, public virtual Linker::SegmentManager
	{
	public:
		/* * * General members * * */

		/**
		 * @brief Represents the first 16-bit word of a COFF file
		 */
		enum cpu
		{
			CPU_UNKNOWN = 0,

			//// main supported types

			/** @brief Intel x86-32, introduced with the Intel 80386
			 *
			 * Value as used by UNIX, Microsoft and DJGPP
			 */
			CPU_I386  = 0x014C,

			/** @brief Motorola 68000 and compatibles
			 *
			 * Value as used by UNIX and Digital Research Concurrent DOS 68K
			 */
			CPU_M68K  = 0x0150,

			//// GNU binutils output

			/** @brief WDC 65C816
			 *
			 * Value as used by GNU binutils
			 */
			CPU_W65   = 0x6500,

			/** @brief Zilog Z80
			 *
			 * Value as used by GNU binutils
			 */
			CPU_Z80   = 0x805A,

			/** @brief Zilog Z8000
			 *
			 * Value as used by GNU binutils
			 */
			CPU_Z8K   = 0x8000,

			//// other CPU types, included for completeness sake

			/** @brief Intel x86-16, introduced with the Intel 8086
			 *
			 * Value as used by UNIX
			 */
			CPU_I86   = 0x0148,

			/** @brief Intel x86-16, introduced with the Intel 286
			 *
			 * Value as used by UNIX
			 */
			CPU_I286  = 0x014A,

			/** @brief National Semiconductor NS32000
			 *
			 * Value as used by UNIX
			 */
			CPU_NS32K = 0x0154,

			/** @brief IBM System/370
			 *
			 * Value as used by UNIX
			 */
			CPU_I370  = 0x0158,

			/** @brief MIPS architecture
			 *
			 * Value as used by UNIX
			 */
			CPU_MIPS  = 0x0160,

			/** @brief Motorola 88000
			 *
			 * Value as used by UNIX and also defined in ECOFF
			 */
			CPU_M88K  = 0x016D,

			/** @brief AT&T Bellmac 32, Western Electric 32000 and family
			 *
			 * Value as used by UNIX
			 */
			CPU_WE32K = 0x0170,

			/** @brief DEC VAX
			 *
			 * Value as used by UNIX
			 */
			CPU_VAX   = 0x0178,

			/** @brief AMD 29000
			 *
			 * Value as used by UNIX
			 */
			CPU_AM29K = 0x017A,

			/** @brief DEC Alpha
			 *
			 * Value as used by UNIX
			 */
			CPU_ALPHA = 0x0183,

			/** @brief IBM PowerPC, 32-bit
			 *
			 * Value as defined by XCOFF
			 */
			CPU_PPC   = 0x01DF,

			/** @brief IBM PowerPC, 64-bit
			 *
			 * Value as defined by XCOFF
			 */
			CPU_PPC64 = 0x01F7,

			/** @brief SHARC from Analog Devices */
			CPU_SHARC = 0x521C,

			//// Texas Instruments values

			CPU_C5400 = 0x0098,
			CPU_C6000 = 0x0099,
			CPU_C5500 = 0x009C,
			CPU_MSP430 = 0x00A0,

			//// Microsoft values

			/** @brief Intel i860
			 *
			 * Value as shown in early Microsoft documentation
			 */
			CPU_I860  = 0x014D,

			/** @brief Hitachi SuperH family
			 *
			 * Value as defined by Microsoft
			 */
			CPU_SH    = 0x01A2,

			/** @brief ARM, also known as ARM32 or AArch32; also represents Thumb
			 *
			 * Value as defined by Microsoft
			 */
			CPU_ARM   = 0x01C0,

			/** @brief Matsushita AM33
			 *
			 * Value as defined by Microsoft
			 */
			CPU_AM33  = 0x01D3,

			/** @brief Intel Itanium architecture, also known as IA-64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_IA64  = 0x0200,

			/** @brief Hewlett-Packard PA-RISC
			 *
			 * Value as defined by Microsoft
			 */
			CPU_HPPA  = 0x0290,

			/** @brief EFI bytecode
			 *
			 * Value as defined by Microsoft
			 */
			CPU_EFI   = 0x0EBC,

			/** @brief RISC-V 32
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV32 = 0x5032,

			/** @brief RISC-V 64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV64 = 0x5064,

			/** @brief RISC-V 128
			 *
			 * Value as defined by Microsoft
			 */
			CPU_RISCV128 = 0x5128,

			/** @brief x86-64, introduced by AMD
			 *
			 * Value as defined by Microsoft
			 */
			CPU_AMD64 = 0x8664,

			/** @brief Mitsubishi M32R
			 *
			 * Value as defined by Microsoft
			 */
			CPU_M32R  = 0x9041,

			/** @brief ARM64, also known as AArch64
			 *
			 * Value as defined by Microsoft
			 */
			CPU_ARM64 = 0xAA64,

			//// overloaded values (greater or equal to 0x10000)
			// for now, there are no overloaded values
		};

		/** @brief Variants of the COFF file format */
		enum COFFVariantType
		{
			/** @brief Standard COFF variant (also for 32-bit ECOFF) */
			COFF = 1,
			/** @brief ECOFF 64-bit version */
			ECOFF = 2,
			/** @brief 32-bit XCOFF from IBM */
			XCOFF32 = 3,
			/** @brief 64-bit XCOFF from IBM */
			XCOFF64 = 4,
			/** @brief Microsoft PE/COFF variant */
			PECOFF = 5,
			/** @brief Texas Instruments COFF variant (COFF2) */
			TICOFF = 6,
			/** @brief Texas Instruments COFF variant (COFF1) */
			TICOFF1 = 7,
		};

		static constexpr COFFVariantType AnyCOFFVariant = COFFVariantType(0);

		struct MachineType
		{
			cpu actual_cpu;
			::EndianType endian;
			COFFVariantType coff_variant = AnyCOFFVariant;
		};

		static const std::map<uint32_t, MachineType> MACHINE_TYPES;

		offset_t file_size;

		/** @brief The actual value of the magic number (COFF name: f_magic) */
		char signature[2] = { };

		/**
		 * @brief Retrieves the natural byte order for the architecture
		 */
		::EndianType GetEndianType() const;

		enum relocation_format_type
		{
			COFF_10,
			COFF_14,
			COFF_16,
			ECOFF_8,
			ECOFF_16,
			XCOFF_10,
			XCOFF_14,
			TICOFF_10,
			TICOFF_12,
		};

		relocation_format_type relocation_format;

		constexpr size_t GetRelocationEntrySize() const
		{
			switch(relocation_format)
			{
			case COFF_10:
				return 10;
			case COFF_14:
				return 14;
			case COFF_16:
				return 16;
			case ECOFF_8:
				return 8;
			case ECOFF_16:
				return 16;
			case XCOFF_10:
				return 10;
			case XCOFF_14:
				return 14;
			case TICOFF_10:
				return 10;
			case TICOFF_12:
				return 12;
			}
			assert(false);
		}

		/**
		 * @brief A generic COFF relocation
		 */
		class _Relocation
		{
		public:
			virtual ~_Relocation();
			virtual offset_t GetAddress() const = 0;
			virtual size_t GetSize() const = 0;
			virtual size_t GetEntrySize() const = 0;
			virtual void WriteFile(Linker::Writer& wr) const = 0;
			virtual void FillEntry(Dumper::Entry& entry) const = 0;
		};

		/**
		 * @brief A generic COFF relocation
		 */
		class Relocation
		{
		public:
			// UNIX relocations

			/** @brief No relocation */
			static constexpr uint16_t R_ABS = 0;
			/** @brief 16-bit direct address of symbol */
			static constexpr uint16_t R_DIR16 = 1;
			/** @brief 16-bit relative address of symbol */
			static constexpr uint16_t R_REL16 = 2;
			/** @brief 16-bit indirect address of symbol */
			static constexpr uint16_t R_IND16 = 3;
			/** @brief 24-bit direct address of symbol */
			static constexpr uint16_t R_DIR24 = 4;
			/** @brief 24-bit relative address of symbol */
			static constexpr uint16_t R_REL24 = 5;
			/** @brief 32-bit direct address of symbol */
			static constexpr uint16_t R_DIR32 = 6;
			/** @brief 8-bit low byte of address of symbol */
			static constexpr uint16_t R_OFF8 = 7;
			/** @brief 8-bit high byte of address of symbol */
			static constexpr uint16_t R_OFF16 = 8;
			/** @brief (Intel x86) 16-bit segment selector of symbol */
			static constexpr uint16_t R_SEG12 = 9;
			/** @brief (WE32K) 32-bit direct address of symbol, byte swapped */
			static constexpr uint16_t R_DIR32S = 10;
			/** @brief Auxiliary relocation */
			static constexpr uint16_t R_AUX = 11;
			/** @brief 16-bit (WE32K) optimized indirect address of symbol */
			static constexpr uint16_t R_OPT16 = 12;
			/** @brief 24-bit indirect address of symbol */
			static constexpr uint16_t R_IND24 = 13;
			/** @brief 32-bit indirect address of symbol */
			static constexpr uint16_t R_IND32 = 14;
			/** @brief 8-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELBYTE = 15;
			/** @brief 16-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELWORD = 16;
			/** @brief 32-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELLONG = 17;
			/** @brief 8-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRBYTE = 18;
			/** @brief 16-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRWORD = 19;
			/** @brief 32-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRLONG = 20;

			// Microsoft relocations (Intel 386)

			/** @brief No relocation */
			static constexpr uint16_t REL_I386_ABSOLUTE = 0;
			/** @brief 16-bit virtual address of symbol (not supported) */
			static constexpr uint16_t REL_I386_DIR16 = 1;
			/** @brief 16-bit relative address of symbol (not supported) */
			static constexpr uint16_t REL_I386_REL16 = 2;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_I386_DIR32 = 6;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_I386_DIR32NB = 7;
			/** @brief 16-bit segment selector of symbol (not supported) */
			static constexpr uint16_t REL_I386_SEG12 = 9;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_I386_SECTION = 10;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_I386_SECREL = 11;
			/** @brief CLR token */
			static constexpr uint16_t REL_I386_TOKEN = 12;
			/** @brief 7-bit offset from section base */
			static constexpr uint16_t REL_I386_SECREL7 = 13;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_I386_REL32 = 20;

			// Microsoft relocations (AMD64)

			/** @brief No relocation */
			static constexpr uint16_t REL_AMD64_ABSOLUTE = 0;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR64 = 1;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR32 = 2;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR32NB = 3;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_AMD64_REL32 = 4;
			/** @brief 32-bit relative address of symbol minus 1 */
			static constexpr uint16_t REL_AMD64_REL32_1 = 5;
			/** @brief 32-bit relative address of symbol minus 2 */
			static constexpr uint16_t REL_AMD64_REL32_2 = 6;
			/** @brief 32-bit relative address of symbol minus 3 */
			static constexpr uint16_t REL_AMD64_REL32_3 = 7;
			/** @brief 32-bit relative address of symbol minus 4 */
			static constexpr uint16_t REL_AMD64_REL32_4 = 8;
			/** @brief 32-bit relative address of symbol minus 5 */
			static constexpr uint16_t REL_AMD64_REL32_5 = 9;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_AMD64_SECTION = 10;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_AMD64_SECREL = 11;
			/** @brief 7-bit offset from section base */
			static constexpr uint16_t REL_AMD64_SECREL7 = 12;
			/** @brief CLR token */
			static constexpr uint16_t REL_AMD64_TOKEN = 13;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_SREL32 = 14;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_PAIR = 15;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_SSPAN32 = 16;

			// Microsoft relocations (ARM)

			/** @brief No relocation */
			static constexpr uint16_t REL_ARM_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM_ADDR32 = 1;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ARM_ADDR32NB = 2;
			/** @brief 24-bit ARM relative address of symbol */
			static constexpr uint16_t REL_ARM_BRANCH24 = 3;
			/** @brief Two 16-bit instructions with 11 bits of the relative address of symbol each */
			static constexpr uint16_t REL_ARM_BRANCH11 = 4;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_ARM_REL32 = 10;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ARM_SECTION = 14;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ARM_SECREL = 15;
			/** @brief Two 32-bit ARM instructions with 32 bits of the virtual address of the symbol */
			static constexpr uint16_t REL_ARM_MOV32 = 16;
			/** @brief Two 32-bit Thumb instructions with 32 bits of the virtual address of the symbol */
			static constexpr uint16_t REL_THUMB_MOV32 = 17;
			/** @brief 24-bit Thumb relative address of symbol */
			static constexpr uint16_t REL_THUMB_BRANCH24 = 20;
			/** @brief 23-bit Thumb BLX relative address of symbol */
			static constexpr uint16_t REL_THUMB_BLX23 = 21;
			/** @brief ? */
			static constexpr uint16_t REL_ARM_PAIR = 22;

			// Microsoft relocations (ARM64)

			/** @brief No relocation */
			static constexpr uint16_t REL_ARM64_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR32 = 1;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR32NB = 2;
			/** @brief 26-bit ARM relative address of symbol */
			static constexpr uint16_t REL_ARM64_BRANCH26 = 3;
			/** @brief Page base of symbol for ADRP */
			static constexpr uint16_t REL_ARM64_PAGEBASE_REL21 = 4;
			/** @brief 12-bit relative address of symbol for ADR */
			static constexpr uint16_t REL_ARM64_REL21 = 5;
			/** @brief 12-bit page offset of symbol for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_PAGEOFFSET_12A = 6;
			/** @brief 12-bit page offset of symbol for LDR */
			static constexpr uint16_t REL_ARM64_PAGEOFFSET_12L = 7;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ARM64_SECREL = 8;
			/** @brief Low 12 bits of offset from section start for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_SECREL_LOW12A = 9;
			/** @brief Bits 12 to 23 of offset from section start for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_SECREL_HIGH12A = 10;
			/** @brief Low 12 bits of offset from section start for LDR */
			static constexpr uint16_t REL_ARM64_SECREL_LOW12L = 11;
			/** @brief CLR token */
			static constexpr uint16_t REL_ARM64_TOKEN = 12;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ARM64_SECTION = 13;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR64 = 14;
			/** @brief 19-bit relative address of symbol */
			static constexpr uint16_t REL_ARM64_BRANCH19 = 15;
			/** @brief 14-bit relative address of symbol for TBZ/TBNZ */
			static constexpr uint16_t REL_ARM64_BRANCH14 = 16;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_ARM64_REL32 = 17;

			// TODO: Microsoft relocations (MIPS)

			// Microsoft relocations (Alpha)

			/** @brief No relocation */
			static constexpr uint16_t REL_ALPHA_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLONG = 1;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFQUAD = 2;
			/** @brief 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPREL32 = 3;
			/** @brief 16-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_LITERAL = 4;
			/** @brief reserved */
			static constexpr uint16_t REL_ALPHA_LITUSE = 5;
			/** @brief reserved */
			static constexpr uint16_t REL_ALPHA_GPDISP = 6;
			/** @brief 21-bit relative address of symbol */
			static constexpr uint16_t REL_ALPHA_BRADDR = 7;
			/** @brief 14-bit hint for jump target */
			static constexpr uint16_t REL_ALPHA_HINT = 8;
			/** @brief 32-bit virtual address of symbol split into two 16-bit values; this relocation must be followed by an ABSOLUTE or MATCH relocation */
			static constexpr uint16_t REL_ALPHA_INLINE_REFLONG = 9;
			/** @brief High 16 bits of 32-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFHI = 10;
			/** @brief Low 16 bits of 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLO = 11;
			/** @brief Displacement value for a preceding REFHI, SECRELHI, REFQ3 or REFQ2 relocation */
			static constexpr uint16_t REL_ALPHA_PAIR = 12;
			/** @brief Displacement value for a preceding REFLONG relocation */
			static constexpr uint16_t REL_ALPHA_MATCH = 13;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ALPHA_SECTION = 14;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ALPHA_SECREL = 15;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLONGNB = 16;
			/** @brief Low 16 bits of 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ALPHA_SECRELLO = 17;
			/** @brief High 16 bits of 32-bit offset from section start; this relocation must be followed by a PAIR relocation (debugging) */
			static constexpr uint16_t REL_ALPHA_SECRELHI = 18;
			/** @brief The second most significant 16 bits of the 64-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFQ3 = 19;
			/** @brief The third most significant 16 bits of the 64-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFQ2 = 20;
			/** @brief The least significant 16 bits of the 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFQ1 = 21;
			/** @brief Low 16 bits of 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPRELLO = 22;
			/** @brief High 16 bits of 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPRELHI = 23;

			// TODO: Microsoft relocations (PowerPC)
			// TODO: Microsoft relocations (IA64)

			// TODO: Microsoft relocations (SuperH)

			// Other relocation types

			// https://github.com/aixoss/binutils/blob/master/include/coff/internal.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z80.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z8k.h

			static constexpr uint16_t R_Z80_IMM8  = 0x22;
			static constexpr uint16_t R_Z80_IMM16 = 0x01;
			static constexpr uint16_t R_Z80_IMM24 = 0x33;
			static constexpr uint16_t R_Z80_IMM32 = 0x17;
			static constexpr uint16_t R_Z80_OFF8  = 0x32;
			static constexpr uint16_t R_Z80_JR    = 0x02;

			static constexpr uint16_t R_Z8K_IMM4L = 0x23;
			static constexpr uint16_t R_Z8K_IMM4H = 0x24;
			static constexpr uint16_t R_Z8K_DISP7 = 0x25; /* djnz */
			static constexpr uint16_t R_Z8K_IMM8  = 0x22;
			static constexpr uint16_t R_Z8K_IMM16 = 0x01;
			static constexpr uint16_t R_Z8K_REL16 = 0x04;
			static constexpr uint16_t R_Z8K_IMM32 = 0x11;
			static constexpr uint16_t R_Z8K_JR    = 0x02; /* jr */
			static constexpr uint16_t R_Z8K_CALLR = 0x05; /* callr */

			static constexpr uint16_t R_W65_ABS8     = 0x01;
			static constexpr uint16_t R_W65_ABS16    = 0x02;
			static constexpr uint16_t R_W65_ABS24    = 0x03;
			static constexpr uint16_t R_W65_ABS8S8   = 0x04;
			static constexpr uint16_t R_W65_ABS8S16  = 0x05;
			static constexpr uint16_t R_W65_ABS16S8  = 0x06;
			static constexpr uint16_t R_W65_ABS16S16 = 0x07;
			static constexpr uint16_t R_W65_PCR8     = 0x08;
			static constexpr uint16_t R_W65_PCR16    = 0x09;
			static constexpr uint16_t R_W65_DP       = 0x0A;

			/**
			 * @brief Address of the relocation (COFF name: r_vaddr)
			 */
			offset_t address;
			/**
			 * @brief Index of symbol in symbol table (COFF name: r_symndx)
			 */
			uint32_t symbol_index;
			/**
			 * @brief (COFF name: r_offset)
			 */
			uint32_t offset;
			/**
			 * @brief Type of relocation (COFF name: r_type)
			 */
			uint16_t type;
			/**
			 * @brief Assorted information (COFF name: r_bits, r_size, r_stuff etc.)
			 */
			uint16_t information;

			void Read(Linker::Reader& rd, const COFFFormat& coff);

			size_t GetSize(const COFFFormat& coff) const;
			void WriteFile(Linker::Writer& wr, const COFFFormat& coff) const;
			void FillEntry(Dumper::Entry& entry, const COFFFormat& coff) const;
		};

		/**
		 * @brief The standard UNIX COFF relocation format
		 */
		class UNIXRelocation : public _Relocation
		{
		public:
			// UNIX relocations

			/** @brief No relocation */
			static constexpr uint16_t R_ABS = 0;
			/** @brief 16-bit direct address of symbol */
			static constexpr uint16_t R_DIR16 = 1;
			/** @brief 16-bit relative address of symbol */
			static constexpr uint16_t R_REL16 = 2;
			/** @brief 16-bit indirect address of symbol */
			static constexpr uint16_t R_IND16 = 3;
			/** @brief 24-bit direct address of symbol */
			static constexpr uint16_t R_DIR24 = 4;
			/** @brief 24-bit relative address of symbol */
			static constexpr uint16_t R_REL24 = 5;
			/** @brief 32-bit direct address of symbol */
			static constexpr uint16_t R_DIR32 = 6;
			/** @brief 8-bit low byte of address of symbol */
			static constexpr uint16_t R_OFF8 = 7;
			/** @brief 8-bit high byte of address of symbol */
			static constexpr uint16_t R_OFF16 = 8;
			/** @brief (Intel x86) 16-bit segment selector of symbol */
			static constexpr uint16_t R_SEG12 = 9;
			/** @brief (WE32K) 32-bit direct address of symbol, byte swapped */
			static constexpr uint16_t R_DIR32S = 10;
			/** @brief Auxiliary relocation */
			static constexpr uint16_t R_AUX = 11;
			/** @brief 16-bit (WE32K) optimized indirect address of symbol */
			static constexpr uint16_t R_OPT16 = 12;
			/** @brief 24-bit indirect address of symbol */
			static constexpr uint16_t R_IND24 = 13;
			/** @brief 32-bit indirect address of symbol */
			static constexpr uint16_t R_IND32 = 14;
			/** @brief 8-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELBYTE = 15;
			/** @brief 16-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELWORD = 16;
			/** @brief 32-bit direct (symbol relative) address of symbol */
			static constexpr uint16_t R_RELLONG = 17;
			/** @brief 8-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRBYTE = 18;
			/** @brief 16-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRWORD = 19;
			/** @brief 32-bit PC relative address of symbol */
			static constexpr uint16_t R_PCRLONG = 20;

			// Microsoft relocations (Intel 386)

			/** @brief No relocation */
			static constexpr uint16_t REL_I386_ABSOLUTE = 0;
			/** @brief 16-bit virtual address of symbol (not supported) */
			static constexpr uint16_t REL_I386_DIR16 = 1;
			/** @brief 16-bit relative address of symbol (not supported) */
			static constexpr uint16_t REL_I386_REL16 = 2;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_I386_DIR32 = 6;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_I386_DIR32NB = 7;
			/** @brief 16-bit segment selector of symbol (not supported) */
			static constexpr uint16_t REL_I386_SEG12 = 9;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_I386_SECTION = 10;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_I386_SECREL = 11;
			/** @brief CLR token */
			static constexpr uint16_t REL_I386_TOKEN = 12;
			/** @brief 7-bit offset from section base */
			static constexpr uint16_t REL_I386_SECREL7 = 13;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_I386_REL32 = 20;

			// Microsoft relocations (AMD64)

			/** @brief No relocation */
			static constexpr uint16_t REL_AMD64_ABSOLUTE = 0;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR64 = 1;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR32 = 2;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_AMD64_ADDR32NB = 3;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_AMD64_REL32 = 4;
			/** @brief 32-bit relative address of symbol minus 1 */
			static constexpr uint16_t REL_AMD64_REL32_1 = 5;
			/** @brief 32-bit relative address of symbol minus 2 */
			static constexpr uint16_t REL_AMD64_REL32_2 = 6;
			/** @brief 32-bit relative address of symbol minus 3 */
			static constexpr uint16_t REL_AMD64_REL32_3 = 7;
			/** @brief 32-bit relative address of symbol minus 4 */
			static constexpr uint16_t REL_AMD64_REL32_4 = 8;
			/** @brief 32-bit relative address of symbol minus 5 */
			static constexpr uint16_t REL_AMD64_REL32_5 = 9;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_AMD64_SECTION = 10;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_AMD64_SECREL = 11;
			/** @brief 7-bit offset from section base */
			static constexpr uint16_t REL_AMD64_SECREL7 = 12;
			/** @brief CLR token */
			static constexpr uint16_t REL_AMD64_TOKEN = 13;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_SREL32 = 14;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_PAIR = 15;
			/** @brief ? */
			static constexpr uint16_t REL_AMD64_SSPAN32 = 16;

			// Microsoft relocations (ARM)

			/** @brief No relocation */
			static constexpr uint16_t REL_ARM_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM_ADDR32 = 1;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ARM_ADDR32NB = 2;
			/** @brief 24-bit ARM relative address of symbol */
			static constexpr uint16_t REL_ARM_BRANCH24 = 3;
			/** @brief Two 16-bit instructions with 11 bits of the relative address of symbol each */
			static constexpr uint16_t REL_ARM_BRANCH11 = 4;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_ARM_REL32 = 10;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ARM_SECTION = 14;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ARM_SECREL = 15;
			/** @brief Two 32-bit ARM instructions with 32 bits of the virtual address of the symbol */
			static constexpr uint16_t REL_ARM_MOV32 = 16;
			/** @brief Two 32-bit Thumb instructions with 32 bits of the virtual address of the symbol */
			static constexpr uint16_t REL_THUMB_MOV32 = 17;
			/** @brief 24-bit Thumb relative address of symbol */
			static constexpr uint16_t REL_THUMB_BRANCH24 = 20;
			/** @brief 23-bit Thumb BLX relative address of symbol */
			static constexpr uint16_t REL_THUMB_BLX23 = 21;
			/** @brief ? */
			static constexpr uint16_t REL_ARM_PAIR = 22;

			// Microsoft relocations (ARM64)

			/** @brief No relocation */
			static constexpr uint16_t REL_ARM64_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR32 = 1;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR32NB = 2;
			/** @brief 26-bit ARM relative address of symbol */
			static constexpr uint16_t REL_ARM64_BRANCH26 = 3;
			/** @brief Page base of symbol for ADRP */
			static constexpr uint16_t REL_ARM64_PAGEBASE_REL21 = 4;
			/** @brief 12-bit relative address of symbol for ADR */
			static constexpr uint16_t REL_ARM64_REL21 = 5;
			/** @brief 12-bit page offset of symbol for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_PAGEOFFSET_12A = 6;
			/** @brief 12-bit page offset of symbol for LDR */
			static constexpr uint16_t REL_ARM64_PAGEOFFSET_12L = 7;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ARM64_SECREL = 8;
			/** @brief Low 12 bits of offset from section start for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_SECREL_LOW12A = 9;
			/** @brief Bits 12 to 23 of offset from section start for ADD/ADDS */
			static constexpr uint16_t REL_ARM64_SECREL_HIGH12A = 10;
			/** @brief Low 12 bits of offset from section start for LDR */
			static constexpr uint16_t REL_ARM64_SECREL_LOW12L = 11;
			/** @brief CLR token */
			static constexpr uint16_t REL_ARM64_TOKEN = 12;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ARM64_SECTION = 13;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ARM64_ADDR64 = 14;
			/** @brief 19-bit relative address of symbol */
			static constexpr uint16_t REL_ARM64_BRANCH19 = 15;
			/** @brief 14-bit relative address of symbol for TBZ/TBNZ */
			static constexpr uint16_t REL_ARM64_BRANCH14 = 16;
			/** @brief 32-bit relative address of symbol */
			static constexpr uint16_t REL_ARM64_REL32 = 17;

			// TODO: Microsoft relocations (MIPS)

			// Microsoft relocations (Alpha)

			/** @brief No relocation */
			static constexpr uint16_t REL_ALPHA_ABSOLUTE = 0;
			/** @brief 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLONG = 1;
			/** @brief 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFQUAD = 2;
			/** @brief 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPREL32 = 3;
			/** @brief 16-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_LITERAL = 4;
			/** @brief reserved */
			static constexpr uint16_t REL_ALPHA_LITUSE = 5;
			/** @brief reserved */
			static constexpr uint16_t REL_ALPHA_GPDISP = 6;
			/** @brief 21-bit relative address of symbol */
			static constexpr uint16_t REL_ALPHA_BRADDR = 7;
			/** @brief 14-bit hint for jump target */
			static constexpr uint16_t REL_ALPHA_HINT = 8;
			/** @brief 32-bit virtual address of symbol split into two 16-bit values; this relocation must be followed by an ABSOLUTE or MATCH relocation */
			static constexpr uint16_t REL_ALPHA_INLINE_REFLONG = 9;
			/** @brief High 16 bits of 32-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFHI = 10;
			/** @brief Low 16 bits of 32-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLO = 11;
			/** @brief Displacement value for a preceding REFHI, SECRELHI, REFQ3 or REFQ2 relocation */
			static constexpr uint16_t REL_ALPHA_PAIR = 12;
			/** @brief Displacement value for a preceding REFLONG relocation */
			static constexpr uint16_t REL_ALPHA_MATCH = 13;
			/** @brief 16-bit section index (debugging) */
			static constexpr uint16_t REL_ALPHA_SECTION = 14;
			/** @brief 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ALPHA_SECREL = 15;
			/** @brief 32-bit relative virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFLONGNB = 16;
			/** @brief Low 16 bits of 32-bit offset from section start (debugging) */
			static constexpr uint16_t REL_ALPHA_SECRELLO = 17;
			/** @brief High 16 bits of 32-bit offset from section start; this relocation must be followed by a PAIR relocation (debugging) */
			static constexpr uint16_t REL_ALPHA_SECRELHI = 18;
			/** @brief The second most significant 16 bits of the 64-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFQ3 = 19;
			/** @brief The third most significant 16 bits of the 64-bit virtual address of symbol; this relocation must be followed by a PAIR relocation */
			static constexpr uint16_t REL_ALPHA_REFQ2 = 20;
			/** @brief The least significant 16 bits of the 64-bit virtual address of symbol */
			static constexpr uint16_t REL_ALPHA_REFQ1 = 21;
			/** @brief Low 16 bits of 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPRELLO = 22;
			/** @brief High 16 bits of 32-bit global pointer relative address of symbol */
			static constexpr uint16_t REL_ALPHA_GPRELHI = 23;

			// TODO: Microsoft relocations (PowerPC)
			// TODO: Microsoft relocations (IA64)

			// TODO: Microsoft relocations (SuperH)

			COFFVariantType coff_variant;
			cpu cpu_type;

			offset_t address = 0;
			uint32_t symbol_index = 0;
			uint16_t type = 0;
			uint32_t information = 0;

			UNIXRelocation(COFFVariantType coff_variant, cpu cpu_type)
				: coff_variant(coff_variant), cpu_type(cpu_type)
			{
			}

			void Read(Linker::Reader& rd);

			offset_t GetAddress() const override;

			size_t GetSize() const override;

			size_t GetEntrySize() const override;

			void WriteFile(Linker::Writer& wr) const override;

			void FillEntry(Dumper::Entry& entry) const override;
		};

		/**
		 * @brief A relocation, as stored by the Z80/Z8000 backend
		 */
		class ZilogRelocation : public _Relocation
		{
		public:
			// https://github.com/aixoss/binutils/blob/master/include/coff/internal.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z80.h
			// https://github.com/aixoss/binutils/blob/master/include/coff/z8k.h

			static constexpr uint16_t R_Z80_IMM8  = 0x22;
			static constexpr uint16_t R_Z80_IMM16 = 0x01;
			static constexpr uint16_t R_Z80_IMM24 = 0x33;
			static constexpr uint16_t R_Z80_IMM32 = 0x17;
			static constexpr uint16_t R_Z80_OFF8  = 0x32;
			static constexpr uint16_t R_Z80_JR    = 0x02;

			static constexpr uint16_t R_Z8K_IMM4L = 0x23;
			static constexpr uint16_t R_Z8K_IMM4H = 0x24;
			static constexpr uint16_t R_Z8K_DISP7 = 0x25; /* djnz */
			static constexpr uint16_t R_Z8K_IMM8  = 0x22;
			static constexpr uint16_t R_Z8K_IMM16 = 0x01;
			static constexpr uint16_t R_Z8K_REL16 = 0x04;
			static constexpr uint16_t R_Z8K_IMM32 = 0x11;
			static constexpr uint16_t R_Z8K_JR    = 0x02; /* jr */
			static constexpr uint16_t R_Z8K_CALLR = 0x05; /* callr */

			static constexpr uint16_t R_W65_ABS8     = 0x01;
			static constexpr uint16_t R_W65_ABS16    = 0x02;
			static constexpr uint16_t R_W65_ABS24    = 0x03;
			static constexpr uint16_t R_W65_ABS8S8   = 0x04;
			static constexpr uint16_t R_W65_ABS8S16  = 0x05;
			static constexpr uint16_t R_W65_ABS16S8  = 0x06;
			static constexpr uint16_t R_W65_ABS16S16 = 0x07;
			static constexpr uint16_t R_W65_PCR8     = 0x08;
			static constexpr uint16_t R_W65_PCR16    = 0x09;
			static constexpr uint16_t R_W65_DP       = 0x0A;

			cpu cpu_type;

			/**
			 * @brief Address of the relocation (COFF name: r_vaddr)
			 */
			uint32_t address;
			/**
			 * @brief Index of symbol in symbol table (COFF name: r_symndx)
			 */
			uint32_t symbol_index;
			/**
			 * @brief (COFF name: r_offset)
			 */
			uint32_t offset;
			/**
			 * @brief Type of relocation (COFF name: r_type)
			 */
			uint16_t type;
			/**
			 * @brief unknown (COFF name: r_stuff)
			 */
			uint16_t data;

			ZilogRelocation(cpu cpu_type)
				: cpu_type(cpu_type)
			{
			}

			void Read(Linker::Reader& rd);

			offset_t GetAddress() const override;

			size_t GetSize() const override;

			size_t GetEntrySize() const override;

			void WriteFile(Linker::Writer& wr) const override;

			void FillEntry(Dumper::Entry& entry) const override;
		};

		/**
		 * @brief A COFF symbol
		 */
		class Symbol
		{
		public:
			/**
			 * @brief Symbol name (COFF name: n_name, if it fits inside field)
			 */
			std::string name;
			/**
			 * @brief The index of the symbol name within the string table, if not stored directly in the entry, 0 otherwise (COFF name: n_name)
			 */
			uint32_t name_index;
			/**
			 * @brief The actual value of the symbol (COFF name: n_value)
			 */
			uint32_t value;
			/**
			 * @brief The number of the section, with special values 0 (N_UNDEF), 1 (N_ABS) and 2 (N_DEBUG) (COFF name: n_scnum)
			 */
			uint16_t section_number;
			/**
			 * @brief The symbol type (COFF name: n_type)
			 */
			uint16_t type;
			/**
			 * @brief COFF name: n_sclass, typical values are 2 (C_EXT), 3 (C_STAT)
			 *
			 * The fields storage_class, section_number and value interact in non-obvious ways
			 */
			uint8_t storage_class;
			/**
			 * @brief Signifies how many extra entries are present, these should be skipped, 0 is a typical value (COFF name: n_numaux)
			 */
			uint8_t auxiliary_count;

			void Read(Linker::Reader& rd);

			bool IsExternal() const;

			class AuxiliaryEntry
			{
			public:
				virtual ~AuxiliaryEntry() = default;
				virtual void Read(Linker::Reader& rd) = 0;
				virtual void Write(Linker::Writer& wr) const = 0;
				virtual void FillDumpData(Dumper::Entry& entry) const = 0;
			};

			class FileNameAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				std::string file_name;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class SectionAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				int32_t section_length;
				uint16_t relocation_entry_count;
				uint16_t line_number_count;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class TagNameAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				// skip 6 bytes
				uint16_t size;
				// skip 4 bytes
				int32_t next_entry_index;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class EndOfStructureAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				int32_t tag_index;
				// skip 2 bytes
				uint16_t size;
				// skip 4 bytes

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class FunctionAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				int32_t tag_index;
				int32_t size;
				int32_t pointer_to_line_number;
				int32_t next_entry_index;
				uint16_t transfer_table_index;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class ArrayAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				int32_t tag_index;
				uint16_t declaration_line_number;
				uint16_t size;
				std::array<uint16_t, 4> dimensions;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class BeginOrEndAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				// skip 4 bytes
				uint16_t line_number;
				// skip 6 bytes
				int32_t next_entry_index; // TODO: only for beginning

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			class StructureAuxiliaryEntry : public AuxiliaryEntry
			{
			public:
				// TODO
				int32_t tag_index;
				// skip 2 bytes
				uint16_t size;

				void Read(Linker::Reader& rd) override;
				void Write(Linker::Writer& wr) const override;
				void FillDumpData(Dumper::Entry& entry) const override;
			};

			std::unique_ptr<AuxiliaryEntry> auxiliary_entry = nullptr;

			enum
			{
				/** @brief Pysical end of function */
				C_EFCN = uint8_t(-1),
				C_NULL = 0,
				/** @brief Automatic variable */
				C_AUTO,
				/** @brief external symbol */
				C_EXT,
				/** @brief static variable */
				C_STAT,
				/** @brief register variable */
				C_REG,
				/** @brief external definition */
				C_EXTDEF,
				/** @brief label */
				C_LABEL,
				/** @brief undefined label */
				C_ULABEL,
				/** @brief member of structure */
				C_MOS,
				/** @brief function argument */
				C_ARG,
				/** @brief structure tag */
				C_STRTAG,
				/** @brief member of union */
				C_MOU,
				/** @brief union tag */
				C_UNTAG,
				/** @brief type definition */
				C_TPDEF,
				/** @brief uninitialized static */
				C_USTATIC,
				/** @brief enumeration tag */
				C_ENTAG,
				/** @brief member of enumeration */
				C_MOE,
				/** @brief register parameter */
				C_REGPARM,
				/** @brief bit field */
				C_FIELD,
				/** @brief begin/end block */
				C_BLOCK = 100,
				/** @brief begin/end function */
				C_FCN,
				/** @brief end of structure */
				C_EOS,
				/** @brief file name */
				C_FILE,
				/** @brief */
				C_LINE,
				/** @brief duplicated tag */
				C_ALIAS,
				/** @brief static but avoids name conflict */
				C_HIDDEN,
				/** @brief shadow symbol */
				C_SHADOW,
				/** @brief external with weak linkage */
				C_WEAKEXT
			};

			enum
			{
				/** @brief Debugging symbol */
				N_DEBUG = uint16_t(-2),
				/** @brief Absolute symbol */
				N_ABS = uint16_t(-1),
				/** @brief Undefined external symbol */
				N_UNDEF = 0,
			};

			enum
			{
				T_NULL = 0,
				/** @brief Function argument */
				T_ARG,
				/** @brief Character (C type char) */
				T_CHAR,
				/** @brief Short integer (C type short) */
				T_SHORT,
				/** @brief Integer (C type int) */
				T_INT,
				/** @brief Long integer (C type long) */
				T_LONG,
				/** @brief Float (C type float) */
				T_FLOAT,
				/** @brief Double (C type double) */
				T_DOUBLE,
				/** @brief Structure (C keyword struct) */
				T_STRUCT,
				/** @brief Union (C keyword union) */
				T_UNION,
				/** @brief Enumeration (C keyword enum) */
				T_ENUM,
				/** @brief Member of enumeration */
				T_MOE,
				/** @brief Unsigned character (C type unsigned char) */
				T_UCHAR,
				/** @brief Unsigned short integer (C type unsigned short) */
				T_USHORT,
				/** @brief Unsigned integer (C type unsigned int) */
				T_UINT,
				/** @brief Unsigned long integer (C type unsigned long) */
				T_ULONG,
			};

			enum
			{
				/** @brief Not a derived type */
				DT_NON = 0,
				/** @brief Pointer to */
				DT_PTR,
				/** @brief Function returning */
				DT_FCN,
				/** @brief Array of */
				DT_ARY,
			};
		};

		/**
		 * @brief A COFF section
		 */
		class Section
		{
		public:
			/**
			 * @brief The name of the section (COFF name: s_name)
			 */
			std::string name;
			/**
			 * @brief The physical address of the section (expected to be identical to the virtual address) (COFF name: s_paddr)
			 *
			 * For PE/COFF, this field is reinterpreted as the size of the section in memory
			 */
			offset_t physical_address = 0;
			/**
			 * @brief The virtual address of the section (COFF name: s_vaddr)
			 */
			offset_t address = 0;
			/**
			 * @brief The size of the section (COFF name: s_size)
			 */
			offset_t size = 0;
			/**
			 * @brief Offset of stored image data from COFF header start (COFF name: s_scnptr)
			 */
			offset_t section_pointer = 0;
			/**
			 * @brief Offset to COFF relocations (COFF name: s_relptr)
			 */
			offset_t relocation_pointer = 0;
			/**
			 * @brief unused (COFF name: s_lnnoptr)
			 */
			offset_t line_number_pointer = 0;
			/**
			 * @brief COFF relocation count (COFF name: s_nreloc)
			 */
			uint32_t relocation_count = 0;
			/**
			 * @brief unused (COFF name: s_nlnno)
			 */
			uint32_t line_number_count = 0;
			/**
			 * @brief COFF section flags, determines the type of the section (text, data, bss, etc.) (COFF name: s_flags)
			 */
			uint32_t flags = 0;
			/**
			 * @brief TICOFF specific field
			 */
			uint16_t memory_page_number = 0;

			/**
			 * @brief The stored image data
			 */
			std::shared_ptr<Linker::Contents> image;

			/**
			 * @brief Collection of COFF relocations
			 */
			std::vector<std::unique_ptr<_Relocation>> _relocations;

			std::vector<std::unique_ptr<Relocation>> relocations;

			/* COFF section flags (common to all variants) */
			/** @brief COFF section flag: Section contains executable instructions (COFF name: STYP_TEXT) */
			static constexpr uint32_t TEXT  = 0x0020;
			/** @brief COFF section flag: Section contains initialized data (COFF name: STYP_DATA) */
			static constexpr uint32_t DATA  = 0x0040;
			/** @brief COFF section flag: Section contains uninitialized data (COFF name: STYP_BSS) */
			static constexpr uint32_t BSS   = 0x0080;

			/* UNIX COFF section flags */
			struct COFF_Flags
			{
				/** @brief COFF section flag: Dummy section (COFF name: STYP_DSECT) */
				static constexpr uint32_t DSECT  = 0x0001;
				/** @brief COFF section flag: Noload section (COFF name: STYP_NOLOAD) */
				static constexpr uint32_t NOLOAD = 0x0002;
				/** @brief COFF section flag: Grouped section, formed from input sections (COFF name: STYP_GROUP) */
				static constexpr uint32_t GROUP  = 0x0004;
				/** @brief COFF section flag: Padding section (COFF name: STYP_PAD) */
				static constexpr uint32_t PAD    = 0x0008;
				/** @brief COFF section flag: Copy section (COFF name: STYP_COPY) */
				static constexpr uint32_t COPY   = 0x0010;
				/** @brief COFF section flag: Comment section (COFF name: STYP_INFO) */
				static constexpr uint32_t INFO   = 0x0200;
				/** @brief COFF section flag: Overlay section (COFF name: STYP_OVER) */
				static constexpr uint32_t OVER   = 0x0400;
				/** @brief COFF section flag: Library section (also used by X/GEM to store library information) (COFF name: STYPE_LIB) */
				static constexpr uint32_t LIB    = 0x0800;
			};

			/* ECOFF section flags */
			struct ECOFF_Flags
			{
				/** @brief ECOFF section flag: Section contains read-only data (ECOFF name: STYP_RDATA) */
				static constexpr uint32_t RDATA    = 0x00000100;
				/** @brief ECOFF section flag: "Small data" (ECOFF name: STYP_SDATA) */
				static constexpr uint32_t SDATA    = 0x00000200;
				/** @brief ECOFF section flag: "Small bss" (ECOFF name: STYP_SBSS) */
				static constexpr uint32_t SBSS     = 0x00000400;
				/** @brief ECOFF section flag: (ECOFF name: STYP_UCODE) */
				static constexpr uint32_t UCODE    = 0x00000800;
				/** @brief ECOFF section flag: Global offset table (ECOFF name: STYP_GOT) */
				static constexpr uint32_t GOT      = 0x00001000;
				/** @brief ECOFF section flag: Dynamic linking information (ECOFF name: STYP_DYNAMIC) */
				static constexpr uint32_t DYNAMIC  = 0x00002000;
				/** @brief ECOFF section flag: Dynamic linking symbol table (ECOFF name: STYP_DYNSYM) */
				static constexpr uint32_t DYNSYM   = 0x00004000;
				/** @brief ECOFF section flag: Dynamic relocation information (ECOFF name: STYP_REL_DYN) */
				static constexpr uint32_t REL_DYN  = 0x00008000;
				/** @brief ECOFF section flag: Dynamic linking string table (ECOFF name: STYP_DYNSTR) */
				static constexpr uint32_t DYNSTR   = 0x00010000;
				/** @brief ECOFF section flag: Dynamic symbol hash table (ECOFF name: STYP_HASH) */
				static constexpr uint32_t HASH     = 0x00020000;
				/** @brief ECOFF section flag: Shared library dependency list (ECOFF name: STYP_DSOLIST) */
				static constexpr uint32_t DSOLIST  = 0x00040000;
				/** @brief ECOFF section flag: Additional dynamic linking symbol table (ECOFF name: STYP_MSYM) */
				static constexpr uint32_t MSYM     = 0x00080000;
				/** @brief ECOFF section flag: Multiple bit flag values */
				static constexpr uint32_t EXTMASK  = 0x0FF00000;
				/** @brief ECOFF section flag: Additional dynamic linking information (ECOFF name: STYP_CONFLICT) */
				static constexpr uint32_t CONFLICT = 0x00100000;
				/** @brief ECOFF section flag: Termination text (ECOFF name: STYP_FINI) */
				static constexpr uint32_t FINI     = 0x01000000;
				/** @brief ECOFF section flag: Comment section (ECOFF name: STYP_COMMENT) */
				static constexpr uint32_t COMMENT  = 0x02000000;
				/** @brief ECOFF section flag: Read-only constants (ECOFF name: STYP_RCONST) */
				static constexpr uint32_t RCONST   = 0x02200000;
				/** @brief ECOFF section flag: Exception scope table (ECOFF name: STYP_XDATA) */
				static constexpr uint32_t XDATA    = 0x02400000;
				/** @brief ECOFF section flag: Initialized TLS data (ECOFF name: STYP_TLSDATA) */
				static constexpr uint32_t TLSDATA  = 0x02500000;
				/** @brief ECOFF section flag: Uninitialized TLS data (ECOFF name: STYP_TLSBSS) */
				static constexpr uint32_t TLSBSS   = 0x02600000;
				/** @brief ECOFF section flag: Initialization for TLS data (ECOFF name: STYP_TLSINIT) */
				static constexpr uint32_t TLSINIT  = 0x02700000;
				/** @brief ECOFF section flag: Exception procedure table (ECOFF name: STYP_PDATA) */
				static constexpr uint32_t PDATA    = 0x02800000;
				/** @brief ECOFF section flag: Address literals (ECOFF name: STYP_LITA) */
				static constexpr uint32_t LITA     = 0x04000000;
				/** @brief ECOFF section flag: 8-byte literals (ECOFF name: STYP_LIT8) */
				static constexpr uint32_t LIT8     = 0x08000000;
				/** @brief ECOFF section flag: 4-byte literals (ECOFF name: STYP_LIT8) */
				static constexpr uint32_t LIT4     = 0x10000000;
				/** @brief ECOFF section flag: the s_nreloc field overflowed (ECOFF name: S_NRELOC_OVFL) */
				static constexpr uint32_t NRELOC_OVERFLOWED = 0x20000000;
				/** @brief ECOFF section flag: Initialization text (ECOFF name: STYP_INIT) */
				static constexpr uint32_t INIT     = 0x80000000;
			};

			/* PE section flags */
			struct PECOFF_Flags
			{
				/** @brief PE section flag: Section should not be padded */
				static constexpr uint32_t TYPE_NO_PAD = 0x00000008;
				/** @brief PE section flag: reserved */
				static constexpr uint32_t LNK_OTHER = 0x00000100;
				/** @brief PE section flag: Section contains comments */
				static constexpr uint32_t LNK_INFO = 0x00000200;
				/** @brief PE section flag: Section should be removed when generating image */
				static constexpr uint32_t LNK_REMOVE = 0x00000800;
				/** @brief PE section flag: Section contains COMDAT data */
				static constexpr uint32_t LNK_COMDAT = 0x00001000;
				/** @brief PE section flag: Section data accessed through the global pointer */
				static constexpr uint32_t GPREL = 0x00008000;
				/** @brief PE section flag: reserved */
				static constexpr uint32_t MEM_PURGEABLE = 0x00010000;
				/** @brief PE section flag: reserved */
				static constexpr uint32_t MEM_16BIT = 0x00020000;
				/** @brief PE section flag: reserved */
				static constexpr uint32_t MEM_LOCKED = 0x00040000;
				/** @brief PE section flag: reserved */
				static constexpr uint32_t MEM_PRELOAD = 0x00080000;
				/** @brief PE section flag: alignment mask */
				static constexpr uint32_t ALIGN_MASK = 0x00F00000;
				/** @brief PE section flag: alignment shift */
				static constexpr uint32_t ALIGN_SHIFT = 20;
				/** @brief PE section flag: Section contains more than 65535 relocations */
				static constexpr uint32_t LNK_NRELOC_OVFL = 0x01000000;
				/** @brief PE section flag: Section can be discarded */
				static constexpr uint32_t MEM_DISCARDABLE = 0x02000000;
				/** @brief PE section flag: Section cannot be cached */
				static constexpr uint32_t MEM_NOT_CACHED = 0x04000000;
				/** @brief PE section flag: Section is not pageable */
				static constexpr uint32_t MEM_NOT_PAGED = 0x08000000;
				/** @brief PE section flag: Section can be shared in memory */
				static constexpr uint32_t MEM_SHARED = 0x10000000;
				/** @brief PE section flag: Section data can be executed */
				static constexpr uint32_t MEM_EXECUTE = 0x20000000;
				/** @brief PE section flag: Section can be read from */
				static constexpr uint32_t MEM_READ = 0x40000000;
				/** @brief PE section flag: Section can be written to */
				static constexpr uint32_t MEM_WRITE = 0x80000000;
			};

			/* XCOFF section flags */
			struct XCOFF_Flags
			{
				/** @brief XCOFF section flag: Padding section (XCOFF name: STYP_PAD) */
				static constexpr uint32_t PAD = 0x0008;
				/** @brief XCOFF section flag: DWARF relocation section (XCOFF name: STYP_DWARF) */
				static constexpr uint32_t DWARF = 0x0010;
				/** @brief XCOFF section flag: Exception section (XCOFF name: STYP_EXCEPT) */
				static constexpr uint32_t EXCEPT = 0x0100;
				/** @brief XCOFF section flag: Comment section (XCOFF name: STYP_INFO) */
				static constexpr uint32_t INFO = 0x0200;
				/** @brief XCOFF section flag: Initialized thread-local data (XCOFF name: STYP_TDATA) */
				static constexpr uint32_t TDATA = 0x0400;
				/** @brief XCOFF section flag: Uninitialized thread-local data (XCOFF name: STYP_TBSS) */
				static constexpr uint32_t TBSS = 0x0800;
				/** @brief XCOFF section flag: Loader section (XCOFF name: STYP_LOADER) */
				static constexpr uint32_t LOADER = 0x1000;
				/** @brief XCOFF section flag: Debug section (XCOFF name: STYP_DEBUG) */
				static constexpr uint32_t DEBUG = 0x2000;
				/** @brief XCOFF section flag: Type-check section (XCOFF name: STYP_TYPCHK) */
				static constexpr uint32_t TYPCHK = 0x4000;
				/** @brief XCOFF section flag: (XCOFF name: STYP_OVERFLO) */
				static constexpr uint32_t OVERFLO = 0x8000;
			};

			/* TICOFF section flags */
			struct TICOFF_Flags
			{
				/** @brief COFF section flag: Dummy section (COFF name: STYP_DSECT) */
				static constexpr uint32_t DSECT  = 0x00000001;
				/** @brief COFF section flag: Noload section (COFF name: STYP_NOLOAD) */
				static constexpr uint32_t NOLOAD = 0x00000002;
				/** @brief COFF section flag: Grouped section, formed from input sections (COFF name: STYP_GROUP) */
				static constexpr uint32_t GROUP  = 0x00000004;
				/** @brief COFF section flag: Padding section (COFF name: STYP_PAD) */
				static constexpr uint32_t PAD    = 0x00000008;
				/** @brief COFF section flag: Copy section (COFF name: STYP_COPY) */
				static constexpr uint32_t COPY   = 0x00000010;
				/** @brief TI COFF section flag: Alignment used as a blocking factor (TI COFF name: STYP_BLOCK) */
				static constexpr uint32_t BLOCK  = 0x00010000;
				/** @brief TI COFF section flag: Section should not be changed (TI COFF name: STYP_PASS) */
				static constexpr uint32_t PASS   = 0x00020000;
				/** @brief TI COFF section flag: Conditional linking required (TI COFF name: STYP_CLINK) */
				static constexpr uint32_t CLINK  = 0x00040000;
				/** @brief TI COFF section flag: Section contains vector table (TI COFF name: STYP_VECTOR) */
				static constexpr uint32_t VECTOR = 0x00080000;
				/** @brief TI COFF section flag: Section has been padded (TI COFF name: STYP_PADDED) */
				static constexpr uint32_t PADDED = 0x00100000;
			};

			void Clear();

			Section(uint32_t flags = 0, std::shared_ptr<Linker::Contents> image = nullptr)
				: flags(flags), image(image)
			{
			}

			virtual ~Section();

			bool PresentInFile(COFFVariantType coff_variant) const;
			bool PresentInMemory(COFFVariantType coff_variant) const;

			/** @brief Reads an entry in the section header table */
			void ReadSectionHeader(Linker::Reader& rd, COFFFormat& coff_format);

			/** @brief Writes an entry in the section header table */
			void WriteSectionHeader(Linker::Writer& wr, const COFFFormat& coff_format);

			/** @brief Retrieves the size of the section (for PE, the size of the section as stored in the file) */
			virtual uint32_t ImageSize(const COFFFormat& coff_format) const;

			/** @brief Reads the section contents from a stream, can be overloaded by subclasses */
			virtual void ReadSectionData(Linker::Reader& rd, const COFFFormat& coff_format);
			/** @brief Writes the section contents to a stream, can be overloaded by subclasses */
			virtual void WriteSectionData(Linker::Writer& wr, const COFFFormat& coff_format) const;
			/** @brief Displays the section information and contents */
			virtual void Dump(Dumper::Dumper& dump, const COFFFormat& format, unsigned section_index) const;
		};

		/**
		 * @brief The list of COFF sections
		 */
		std::vector<std::shared_ptr<Section>> sections;

		/**
		 * @brief Section count (COFF name: f_nscns)
		 */
		uint16_t section_count = 0;
		/**
		 * @brief Time stamp, unused (COFF name: f_timdat)
		 */
		uint32_t timestamp = 0;
		/**
		 * @brief Offset to the first symbol (COFF name: f_symptr)
		 */
		offset_t symbol_table_offset = 0;
		/**
		 * @brief The number of symbols (COFF name: f_nsyms)
		 */
		uint32_t symbol_count = 0;
		/**
		 * @brief The symbols stored inside the COFF file
		 */
		std::vector<std::unique_ptr<Symbol>> symbols;
		/**
		 * @brief The size of the optional header (COFF: f_opthdr)
		 */
		uint32_t optional_header_size = 0;
		/**
		 * @brief COFF flags, such as whether the file is executable (f_flags)
		 */
		uint16_t flags = 0;
		/**
		 * @brief TI system target ID
		 */
		uint16_t target = 0;

		/**
		 * @brief An abstract class to represent the optional header
		 */
		class OptionalHeader
		{
		public:
			virtual ~OptionalHeader();
			/**
			 * @brief Returns size of optional header
			 */
			virtual uint32_t GetSize() const = 0;
			virtual void ReadFile(Linker::Reader& rd) = 0;
			virtual void WriteFile(Linker::Writer& wr) const = 0;
			/**
			 * @brief Sets up fields to be consistent
			 *
			 * @return Number of extra bytes following symbol table
			 */
			virtual offset_t CalculateValues(COFFFormat& coff) = 0;
			/**
			 * @brief Retrieves any additional data from the file corresponding to this type of optional header
			 */
			virtual void PostReadFile(COFFFormat& coff, Linker::Reader& rd);
			/**
			 * @brief Stores any additional data in the file corresponding to this type of optional header
			 */
			virtual void PostWriteFile(const COFFFormat& coff, Linker::Writer& wr) const;

			virtual void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const;
		};

		/**
		 * @brief The optional header instance used for reading/writing the COFF file
		 */
		std::unique_ptr<OptionalHeader> optional_header = nullptr;

		struct CDOS68K_Relocation
		{
			size_t size;
			CDOS68K_Relocation(size_t size = 0)
				: size(size)
			{
			}
			operator size_t() const;
			static CDOS68K_Relocation Create(size_t size, uint32_t offset, const COFFFormat& format);
		};

		/**
		 * @brief Concurrent DOS 68K requires a special block of data to represent "crunched" relocations (see CPM68KWriter for more details)
		 */
		std::map<uint32_t, CDOS68K_Relocation> relocations; /* CDOS68K */

		/**
		 * @brief A simplified class to represent an optional header of unknown structure
		 */
		class UnknownOptionalHeader : public OptionalHeader
		{
		public:
			std::unique_ptr<Linker::Buffer> buffer = nullptr;

			UnknownOptionalHeader(offset_t size)
				: buffer(std::make_unique<Linker::Buffer>(size))
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const override;
		};

		/**
		 * @brief A standard 28 byte a.out optional header, used by DJGPP
		 */
		class AOutHeader : public OptionalHeader
		{
		public:
			/**
			 * @brief Type of executable, most typically ZMAGIC (COFF name: magic)
			 */
			uint16_t magic = 0;
			/**
			 * @brief Version stamp (COFF name: vstamp)
			 */
			uint16_t version_stamp = 0;

			/**
			 * @brief Code size (COFF name: tsize)
			 */
			uint32_t code_size = 0;
			/**
			 * @brief Data size (COFF name: dsize)
			 */
			uint32_t data_size = 0;
			/**
			 * @brief BSS size (COFF name: bsize)
			 */
			uint32_t bss_size = 0;
			/**
			 * @brief Initial value of eip (COFF name: entry)
			 */
			uint32_t entry_address = 0;
			/**
			 * @brief Address of code (COFF name: text_start)
			 */
			uint32_t code_address = 0;
			/**
			 * @brief Address of data (COFF name: data_start)
			 */
			uint32_t data_address = 0;

			AOutHeader(uint16_t magic = 0)
				: magic(magic)
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

		protected:
			virtual void DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const;

		public:
			void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const override;
		};

		/**
		 * @brief Concurrent DOS 68K/FlexOS 386 optional header
		 * Concurrent DOS 68K uses the typical a.out header with two additional fields for the offset to relocations and the size of the stack
		 */
		class FlexOSAOutHeader : public AOutHeader
		{
		public:
			FlexOSAOutHeader(uint16_t magic = 0)
				: AOutHeader(magic)
			{
			}

			/**
			 * @brief The offset to the crunched relocation data within the file
			 */
			uint32_t relocations_offset = 0;
			/**
			 * @brief Size of stack for execution
			 */
			uint32_t stack_size = 0;

			/* TODO: magic not needed for CDOS68K? */

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			void PostReadFile(COFFFormat& coff, Linker::Reader& rd) override;

			void PostWriteFile(const COFFFormat& coff, Linker::Writer& wr) const override;

		protected:
			void DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const override;
		};

		/**
		 * @brief GNU a.out optional header
		 * TODO
		 */
		class GNUAOutHeader : public OptionalHeader
		{
		public:
			/* Note: untested */
			uint32_t info = 0;
			uint32_t code_size = 0;
			uint32_t data_size = 0;
			uint32_t bss_size = 0;
			uint32_t symbol_table_size = 0;
			uint32_t entry_address = 0;
			uint32_t code_relocation_size = 0;
			uint32_t data_relocation_size = 0;

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& wr) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const override;
		};

		/**
		 * @brief 56 byte long header used on MIPS
		 *
		 * TODO: untested
		 */
		class MIPSAOutHeader : public AOutHeader
		{
		public:
			// https://web.archive.org/web/20140723105157/http://www-scf.usc.edu/~csci402/ncode/coff_8h-source.html
			/* bss_start */
			uint32_t bss_address;
			/* gpr_mask */
			uint32_t gpr_mask;
			/* cprmask */
			uint32_t cpr_mask[4];
			/* gp_value */
			uint32_t gp_value;

			MIPSAOutHeader(uint16_t magic = 0)
				: AOutHeader(magic)
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

		protected:
			void DumpFields(const COFFFormat& coff, Dumper::Dumper& dump, Dumper::Region& header_region) const override;
		};

		/**
		 * @brief ECOFF optional header
		 *
		 * TODO: untested
		 */
		class ECOFFAOutHeader : public OptionalHeader
		{
		public:
			static constexpr uint16_t OMAGIC = 0x0107;
			static constexpr uint16_t NMAGIC = 0x0108;
			static constexpr uint16_t ZMAGIC = 0x010B;
			/**
			 * @brief Type of executable
			 */
			uint16_t magic = 0;
			static constexpr uint16_t SYM_STAMP = 0x030D;
			/**
			 * @brief Object file version stamp (ECOFF name: vstamp)
			 */
			uint16_t version_stamp = SYM_STAMP;
			/**
			 * @brief Revision build of system tools (ECOFF name: bldrev)
			 */
			uint16_t build_revision = 0;

			/**
			 * @brief Size of code segment (ECOFF name: tsize)
			 */
			uint64_t code_size = 0;
			/**
			 * @brief Size of data segment (ECOFF name: dsize)
			 */
			uint64_t data_size = 0;
			/**
			 * @brief Size of bss segment (ECOFF name: bsize)
			 */
			uint64_t bss_size = 0;
			/**
			 * @brief Virtual address of execution start (ECOFF name: entry)
			 */
			uint64_t entry_address = 0;
			/**
			 * @brief Base address for code segment (ECOFF name: text_start)
			 */
			uint64_t code_address = 0;
			/**
			 * @brief Base address for data segment (ECOFF name: data_start)
			 */
			uint64_t data_address = 0;
			/**
			 * @brief Base address for bss segment (ECOFF name: bss_start)
			 */
			uint64_t bss_address = 0;
			/** @brief unused (ECOFF name: gprmask) */
			uint32_t gpr_mask = 0;
			/** @brief unused (ECOFF name: fprmask) */
			uint32_t fpr_mask = 0;
			/** @brief Initial global pointer value (ECOFF name: gp_value) */
			uint64_t global_pointer = 0;

			ECOFFAOutHeader(uint16_t magic = 0)
				: magic(magic)
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const override;
		};

		/**
		 * @brief XCOFF optional header
		 *
		 * TODO: untested
		 */
		class XCOFFAOutHeader : public OptionalHeader
		{
		public:
			/** @brief False for XCOFF32, true for XCOFF64 */
			bool is64 = false;
			/**
			 * @brief Type of executable (XCOFF name: mflag)
			 */
			uint16_t magic = 0;
			/**
			 * @brief Object file version stamp (XCOFF name: vstamp)
			 */
			uint16_t version_stamp = 0;
			/**
			 * @brief Size of code segment (XCOFF name: tsize)
			 */
			uint64_t code_size = 0;
			/**
			 * @brief Size of data segment (XCOFF name: dsize)
			 */
			uint64_t data_size = 0;
			/**
			 * @brief Size of bss segment (XCOFF name: bsize)
			 */
			uint64_t bss_size = 0;
			/**
			 * @brief Virtual address of execution start (XCOFF name: entry)
			 */
			uint64_t entry_address = 0;
			/**
			 * @brief Base address for code segment (XCOFF name: text_start)
			 */
			uint64_t code_address = 0;
			/**
			 * @brief Base address for data segment (XCOFF name: data_start)
			 */
			uint64_t data_address = 0;
			/**
			 * @brief Address of TOC (XCOFF name: toc)
			 */
			uint64_t toc_address = 0;
			/**
			 * @brief Section number for entry point (XCOFF name: snentry)
			 */
			uint16_t entry_section = 0;
			/**
			 * @brief Section number for text (XCOFF name: sntext)
			 */
			uint16_t code_section = 0;
			/**
			 * @brief Section number for data (XCOFF name: sndata)
			 */
			uint16_t data_section = 0;
			/**
			 * @brief Section number for TOC (XCOFF name: sntoc)
			 */
			uint16_t toc_section = 0;
			/**
			 * @brief Section number for loader data (XCOFF name: snloader)
			 */
			uint16_t loader_section = 0;
			/**
			 * @brief Section number for bss (XCOFF name: snbss)
			 */
			uint16_t bss_section = 0;
			/**
			 * @brief Maximum alignment of text (XCOFF name: algntext)
			 */
			uint16_t code_align = 0;
			/**
			 * @brief Maximum alignment of data (XCOFF name: algndata)
			 */
			uint16_t data_align = 0;
			/**
			 * @brief Module type (XCOFF name: modtype)
			 */
			uint16_t module_type = 0;
			/**
			 * @brief CPU flags (XCOFF name: cpuflag)
			 */
			uint8_t cpu_flags = 0;
			/**
			 * @brief CPU type (XCOFF name: cputype)
			 */
			uint8_t cpu_type = 0;
			/**
			 * @brief Maximum stack size (XCOFF name: maxstack)
			 */
			uint64_t maximum_stack_size = 0;
			/**
			 * @brief Maximum data size (XCOFF name: maxdata)
			 */
			uint64_t maximum_data_size = 0;
			/**
			 * @brief Reserved for debugger (XCOFF name: debugger)
			 */
			uint32_t debugger_data = 0;
			/**
			 * @brief Requested code page size (XCOFF name: textpsize)
			 */
			uint8_t code_page_size = 0;
			/**
			 * @brief Requested data page size (XCOFF name: datapsize)
			 */
			uint8_t text_page_size = 0;
			/**
			 * @brief Requested stack page size (XCOFF name: stackpsize)
			 */
			uint8_t stack_page_size = 0;
			/**
			 * @brief Flags
			 */
			uint8_t flags = 0;
			/**
			 * @brief Section number for tdata (XCOFF name: sntdata)
			 */
			uint16_t tdata_section = 0;
			/**
			 * @brief Section number for tbss (XCOFF name: sntbss)
			 */
			uint16_t tbss_section = 0;
			/**
			 * @brief XCOFF64 specific flags (XCOFF64 name: x64flags)
			 */
			uint32_t xcoff64_flags = 0;
#if 0
			/**
			 * @brief Shared memory page size (XCOFF64 name: shmpsize)
			 */
			uint8_t shared_memory_page = 0;
#endif

			XCOFFAOutHeader(bool is64, uint16_t magic = 0)
				: is64(is64), magic(magic)
			{
			}

			uint32_t GetSize() const override;

			void ReadFile(Linker::Reader& rd) override;

			void WriteFile(Linker::Writer& wr) const override;

			offset_t CalculateValues(COFFFormat& coff) override;

			void Dump(const COFFFormat& coff, Dumper::Dumper& dump) const override;
		};

		void Clear() override;

		/** @brief Loads the specified 16-bit integer in the specified byte order into the COFF header signature */
		void AssignMagicValue(uint16_t value, ::EndianType as_endian_type);
		/** @brief Loads the specified 16-bit integer in the currently set byte order into the COFF header signature */
		void AssignMagicValue(uint16_t value);
		/** @brief Loads the currently set CPU value in the currently set byte order into the COFF header signature */
		void AssignMagicValue();

		COFFVariantType coff_variant = AnyCOFFVariant;

		/**
		 * @brief The CPU type, reflected by the first 16-bit word of a COFF file
		 *
		 * The byte order has to be determined heuristically.
		 */
		cpu cpu_type = CPU_UNKNOWN;

		/**
		 * @brief The byte order
		 */
		::EndianType endiantype = ::UndefinedEndian;

		bool DetectCpuType(::EndianType expected);

		void DetectCpuType();

		void ReadFile(Linker::Reader& rd) override;

	protected:
		void ReadCOFFHeader(Linker::Reader& rd);
		void ReadOptionalHeader(Linker::Reader& rd);
		void ReadRestOfFile(Linker::Reader& rd);

	public:
		offset_t ImageSize() const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

	protected:
		offset_t WriteFileContents(Linker::Writer& wr) const;

	public:
		void Dump(Dumper::Dumper& dump) const override;

		/* * * Reader members * * */

		void SetupOptions(std::shared_ptr<Linker::OutputFormat> format) override;

		bool option_segmentation = false;

		bool FormatRequiresDataStreamFix() const override;

	private:
		/* symbols */
		std::string segment_prefix();

		std::string segment_of_prefix();

		/**
		 * @brief For Z8000 short segmented addresses
		 */
		std::string segmented_address_prefix();

#if 0
		// TODO: can this be used?
		std::string segment_difference_prefix();
#endif

		enum
		{
			/* section number */
			N_UNDEF = 0,
			N_ABS = 0xFFFF,
			N_DEBUG = 0xFFFE,

			/* storage class */
			C_EXT = 2,
			C_STAT = 3,
			C_LABEL = 6,
		};

	public:
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		/* * * Writer members * * */

		class COFFOptionCollector : public Linker::OptionCollector
		{
		public:
			Linker::Option<std::string> stub{"stub", "Filename for stub that gets prepended to executable"};
			// TODO: make stack size a parameter (for FlexOS)

			COFFOptionCollector()
			{
				InitializeFields(stub);
			}
		};

		// for DJGPP binaries
		mutable Microsoft::MZSimpleStubWriter stub;

		/**
		 * @brief Represents the type of target system, which will determine the CPU type and several other fields
		 */
		enum format_type
		{
			/**
			 * @brief An unspecified value, probably will not work
			 */
			GENERIC,
			/**
			 * @brief DJGPP COFF executable
			 */
			DJGPP,
			/**
			 * @brief Concurrent DOS 68K executable (untested but confident)
			 */
			CDOS68K,
			/**
			 * @brief FlexOS 386 executable
			 */
			CDOS386,
			/**
			 * @brief Windows Portable Executable (used only by PE)
			 */
			WINDOWS,
		};
		/**
		 * @brief A representation of the format to generate
		 */
		format_type type = GENERIC;

		/**
		 * @brief Suppress relocation generation, only relevant for Concurrent DOS 68K, since the other target formats do not store relocations
		 */
		bool option_no_relocation = false;

		/**
		 * @brief Size of MZ stub, only used for DJGPP COFF executables
		 */
		uint32_t stub_size = 0;

		/**
		 * @brief Concurrent DOS 68K and FlexOS 386: The stack segment, not stored as part of any section
		 */
		std::shared_ptr<Linker::Segment> stack;
		/**
		 * @brief Entry address, gets stored in optional header later
		 */
		uint32_t entry_address = 0; /* TODO */
		/**
		 * @brief Concurrent DOS 68K: Offset to relocations
		 */
		uint32_t relocations_offset = 0;

		COFFFormat(format_type type = GENERIC, COFFVariantType coff_variant = COFF, EndianType endiantype = ::UndefinedEndian)
			: coff_variant(coff_variant), endiantype(endiantype), type(type)
		{
		}

		~COFFFormat()
		{
			Clear();
		}

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		static std::vector<Linker::OptionDescription<void> *> ParameterNames;
		std::vector<Linker::OptionDescription<void> *> GetLinkerScriptParameterNames() override;

		std::shared_ptr<Linker::OptionCollector> GetOptions() override;

		void SetOptions(std::map<std::string, std::string>& options) override;

		/** @brief COFF file header flags, most of these are obsolete, we only use them as precombined flag sets */
		enum
		{
			/** @brief F_RELFLG */
			FLAG_NO_RELOCATIONS = 0x0001,
			/** @brief F_EXEC */
			FLAG_EXECUTABLE = 0x0002,
			/** @brief F_LNNO */
			FLAG_NO_LINE_NUMBERS = 0x0004,
			/** @brief F_LSYMS */
			FLAG_NO_SYMBOLS = 0x0008,
			/** @brief F_AR16WR */
			FLAG_PDP11_ENDIAN = 0x0080,
			/** @brief F_AR32WR */
			FLAG_32BIT_LITTLE_ENDIAN = 0x0100,
			/** @brief F_AR32W */
			FLAG_32BIT_BIG_ENDIAN = 0x0200,

			OMAGIC = 0x0107,
			NMAGIC = 0x0108,
			/**
			 * @brief Stored as the magic of the a.out header
			 */
			ZMAGIC = 0x010B,
			/**
			 * @brief Magic number required by FlexOS 386
			 */
			MAGIC_FLEXOS386 = 0x01C0,
		};

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		/** @brief Return the segment stored inside the section, note that this only works for binary generation */
		std::shared_ptr<Linker::Segment> GetSegment(std::shared_ptr<Section>& section);

		std::shared_ptr<Linker::Segment> GetCodeSegment();

		std::shared_ptr<Linker::Segment> GetDataSegment();

		std::shared_ptr<Linker::Segment> GetBssSegment();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) const override;
	};

}

#endif /* COFF_H */
