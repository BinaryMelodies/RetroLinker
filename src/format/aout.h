#ifndef AOUT_H
#define AOUT_H

#include "mzexe.h"
#include "../common.h"
#include "../linker/linker_manager.h"
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
	class AOutFormat : public virtual Linker::InputFormat, public virtual Linker::LinkerManager, protected Microsoft::MZSimpleStubWriter
	{
	public:
		/* * * General members * * */
		::EndianType endiantype;
		unsigned word_size;

		enum magic_type
		{
			OMAGIC = 0x0107,
			NMAGIC = 0x0108,
			ZMAGIC = 0x010B,
			QMAGIC = 0x00CC,
		};
		magic_type magic = magic_type(0);

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
		};
		cpu_type cpu = UNKNOWN;

		::EndianType GetEndianType() const;

		unsigned GetWordSize() const;

		uint32_t code_size = 0;
		uint32_t data_size = 0;
		uint32_t bss_size = 0;
		uint32_t symbol_table_size = 0;
		uint32_t entry_address = 0;
		uint32_t code_relocation_size = 0;
		uint32_t data_relocation_size = 0;
		std::map<uint32_t, uint32_t> code_relocations, data_relocations; /* only used by PDOS386 OMAGIC */

		std::shared_ptr<Linker::Image> code, data, bss;

	private:
		bool AttemptFetchMagic(uint8_t signature[4]);

		bool AttemptReadFile(Linker::Reader& rd, uint8_t signature[4], offset_t image_size);

	public:
		class Symbol
		{
		public:
			std::string name;
			uint16_t unknown = 0;
			uint16_t name_offset = 0;
			uint16_t type = 0;
			uint16_t value = 0;
		};

		std::vector<Symbol> symbols;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;

		/* * * Reader * * */

		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		/* * * Writer * * */

		enum system_type
		{
			UNIX, /* also Linux */ /* TODO */
			DJGPP1, /* early DJGPP */
			PDOS386, /* http://pdos.sourceforge.net/ */
		};
		system_type system = system_type(0);

		static std::shared_ptr<AOutFormat> CreateWriter(system_type system, magic_type magic);

		static std::shared_ptr<AOutFormat> CreateWriter(system_type system);

		/**
		 * @brief Default magic number associated with the system
		 *
		 * This information is based on studying the executables included with that software.
		 * PDOS/386 uses OMAGIC, whereas DJGPP uses ZMAGIC.
		 */
		static magic_type GetDefaultMagic(system_type system);

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		std::shared_ptr<Linker::Segment> GetCodeSegment();

		std::shared_ptr<Linker::Segment> GetDataSegment();

		std::shared_ptr<Linker::Segment> GetBssSegment();

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};

}

#endif /* AOUT_H */
