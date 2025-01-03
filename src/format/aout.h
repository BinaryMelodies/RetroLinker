#ifndef AOUT_H
#define AOUT_H

#include "mzexe.h"
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

/* TODO: UNIX v1 a.out
	magic
	header_size + text_size + data_size
	symtab_size
	relocations_size
	bss_size
	0

TODO: PDP-11 a.out
*/

namespace AOut
{
	/**
	 * @brief UNIX/Linux a.out binary file format
	 *
	 * Introduced in the earliest versions of UNIX, it underwent several incompatible updates, including being extended to 32 bits.
	 * At the time of writing, almost all current UNIX platforms use the COFF or ELF format.
	 *
	 * Versions:
	 * - PDP-7 version (unimplemented)
	 * - UNIX v1 version (unimplemented)
	 * - 16-bit UNIX version (unimplemented)
	 * - 32-bit UNIX version (first introduced on the VAX)
	 *
	 * Variants:
	 * - OMAGIC (old magic), the code segment is writable
	 * - NMAGIC (new magic), the code segment is pure and can be shared (read-only)
	 * - ZMAGIC, demand paged
	 * - QMAGIC (TODO)
	 *
	 * Platforms:
	 * - early Linux (unimplemented) (obsolete)
	 * - early DJGPP before version 1.11
	 * - early PDOS/386 and PD-Windows (http://pdos.sourceforge.net/) (obsolete)
	 */
	class AOutFormat : public virtual Linker::InputFormat, public virtual Linker::OutputFormat, public Linker::LinkerManager, protected Microsoft::MZSimpleStubWriter
	{
	public:
		/* * * General members * * */

		AOutFormat()
			: code(nullptr), data(nullptr), bss(nullptr)
		{
		}

		~AOutFormat()
		{
			if(code)
			{
				delete code;
			}
			if(data)
			{
				delete data;
			}
			if(bss)
			{
				delete bss;
			}
		}

		::EndianType endiantype;
		unsigned word_size;

		enum magic_type
		{
			OMAGIC = 0x0107,
			NMAGIC = 0x0108,
			ZMAGIC = 0x010B,
			QMAGIC = 0x00CC,
		} magic;

		enum cpu_type
		{
			UNKNOWN = 0x00,
			M68010  = 0x01,
			M68020  = 0x02,
			SPARC   = 0x03,
			I80386  = 0x64,
			ARM     = 0x67, /* according to BFD */
			MIPS1   = 0x97,
			MIPS2   = 0x98,
			PDP11   = 0xFF, /* not a real magic number */
		} cpu;

		::EndianType GetEndianType() const;

		unsigned GetWordSize() const;

		uint32_t code_size;
		uint32_t data_size;
		uint32_t bss_size;
		uint32_t symbol_table_size;
		uint32_t entry_address;
		uint32_t code_relocation_size;
		uint32_t data_relocation_size;
		std::map<uint32_t, uint32_t> code_relocations, data_relocations; /* only used by PDOS386 OMAGIC */

		Linker::Writable * code, * data, * bss;

	private:
		bool AttemptFetchMagic(uint8_t signature[4]);

		bool AttemptReadFile(Linker::Reader& rd, uint8_t signature[4], offset_t image_size);

	public:
		class Symbol
		{
		public:
			std::string name;
			uint16_t unknown;
			uint16_t name_offset;
			uint16_t type;
			uint16_t value;
		};

		std::vector<Symbol> symbols;

		void ReadFile(Linker::Reader& rd) override;

		void WriteFile(Linker::Writer& wr) override;

		/* * * Reader * * */

	private:
		void GenerateModule(Linker::Module& module);

	public:
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;

		/* * * Writer * * */

		enum system_type
		{
			UNIX, /* also Linux */ /* TODO */
			DJGPP1, /* early DJGPP */
			PDOS386, /* http://pdos.sourceforge.net/ */
		} system;

		static AOutFormat * CreateWriter(system_type system, magic_type magic);

		static AOutFormat * CreateWriter(system_type system);

		/**
		 * @brief Default magic number associated with the system
		 *
		 * This information is based on studying the executables included with that software.
		 * PDOS/386 uses OMAGIC, whereas DJGPP uses ZMAGIC.
		 */
		static magic_type GetDefaultMagic(system_type system);

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(Linker::Segment * segment) override;

		void CreateDefaultSegments();

		Script::List * GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		Linker::Segment * GetCodeSegment();

		Linker::Segment * GetDataSegment();

		Linker::Segment * GetBssSegment();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using LinkerManager::SetLinkScript;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;

		std::string GetDefaultExtension(Linker::Module& module) override;
	};

}

#endif /* AOUT_H */
