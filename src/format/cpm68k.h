#ifndef CPM68K_H
#define CPM68K_H

#include <map>
#include <set>
#include <string>
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"
#include "../dumper/dumper.h"

namespace DigitalResearch
{
	/**
	 * @brief The native executable format for the Motorola 68000 port of CP/M
	 *
	 * This format was also adopted on the following systems:
	 * - CP/M-68K .68k contiguous and non-contiguous files
	 * - GEMDOS .prg and gem .app files (contiguous only)
	 * - Atari TOS .prg, .app, .tos, .ttp, .gtp files (contiguous only)
	 * - Concurrent DOS 68K .68k contiguous files with normal or crunched relocations
	 * - Human68k .z files with no relocations
	 */
	class CPM68KFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		/* * * General members * * */

		/**
		 * @brief Represents the magic number at the beginning of the executable file
		 */
		enum magic_type
		{
			/**
			 * @brief Contiguous executables (magic value 0x601A in big endian) must load the code, data, bss segments consecutively
			 */
			MAGIC_CONTIGUOUS = 1,
			/**
			 * @brief Non-contiguous executables (magic value 0x601B in big endian) can load the code, data, bss segments at non-consecutive addresses, only used by CP/M-68K
			 */
			MAGIC_NONCONTIGUOUS,
			/**
			 * @brief Contiguous executables with crunched relocations (magic value 0x601C in big endian), only used by Concurrent DOS 68K
			 */
			MAGIC_CRUNCHED, /* Concurrent DOS 68K only */
		};

		/** @brief The magic number at the beginning of the executable file, one of 0x601A (contiguous), 0x601B (non-contiguous), 0x601C (contiguous with crunched relocations) */
		char signature[2];

		/**
		 * @brief Size of the code/text segment
		 */
		uint32_t code_size;
		/**
		 * @brief Size of the initialized data segment
		 */
		uint32_t data_size;
		/**
		 * @brief Size of the uninitialized data (bss) segment. Human68k includes the stack in it
		 */
		uint32_t bss_size;
		/**
		 * @brief Size of the symbol table
		 */
		uint32_t symbol_table_size;
		/**
		 * @brief Size of the stack segment. Only used by Concurrent DOS 68K
		 */
		uint32_t stack_size;

		/**
		 * @brief Load address of the code/text segment. Not used by GEMDOS which stores the program flags at this offset
		 */
		uint32_t code_address;
		/**
		 * @brief Program flags, used by GEMDOS
		 */
		uint32_t program_flags; /* TODO: make parameter */

		/**
		 * @brief Set to a non-0 value when relocations are suppressed. Typically this can be 1, but Human68k specifically expects a 0xFFFF, according to documentation
		 */
		uint16_t relocations_suppressed;
		/**
		 * @brief Load address of the initialized data segment. Only relevant for non-contiguous executables (CP/M-68K), otherwise it should follow the code/text segment directy
		 */
		uint32_t data_address;
		/**
		 * @brief Load address of the uninitialized data (bss) segment. Only relevant for non-contiguous executables (CP/M-68K), otherwise it should follow the initialized data segment directy
		 */
		uint32_t bss_address;
		/**
		 * @brief Size of entire file, not used for generation
		 */
		offset_t file_size;

		/**
		 * @brief Storage for code segment
		 */
		Linker::Writable * code;
		/**
		 * @brief Storage for data segment
		 */
		Linker::Writable * data;

		/* filled in automatically */
		struct Relocation
		{
			/**
			 * @brief Size of value to relocate
			 */
			size_t size;
			/**
			 * @brief Segment value, as required by CP/M-68K, they take the value that is stored in file: 1 for data, 2 for text, 3 for bss
			 */
			unsigned segment;
			operator size_t() const;
		};
		/**
		 * @brief Relocations, not used for Human68k
		 */
		std::map<uint32_t, Relocation> relocations;

		magic_type GetSignature() const;

		void SetSignature(magic_type magic);

		/**
		 * @brief The system which will load the executable. Different systems have different relocation formats and expectations as to what segments should be present
		 */
		enum system_type
		{
			/**
			 * @brief Unknown system: use GEMDOS with no relocations
			 */
			SYSTEM_UNKNOWN,
			/**
			 * @brief Digital Research CP/M-68K, uses CP/M-68K relocations
			 */
			SYSTEM_CPM68K,
			/**
			 * @brief Digital Research GEMDOS, only contiguous, relocations always present, header is an unusual 0x1E bytes long and most entries are reserved
			 */
			SYSTEM_GEMDOS_EARLY, /* TODO: parametrize this type */
			/**
			 * @brief Digital Research GEMDOS, Atari TOS, only contiguous, text load address field replaced by program fields
			 */
			SYSTEM_GEMDOS,
			/**
			 * @brief Sharp Corporation & Hudson Soft Human68k .z executable, only contiguous, no relocations or symbol table
			 */
			SYSTEM_HUMAN68K,
			/**
			 * @brief Digital Research Concurrent DOS 68K, non-contiguous not allowed, but relocations can be in CP/M-68K format or crunched format, depending on magic number. Header contains separate field for stack size
			 */
			SYSTEM_CDOS68K
		} system;

		void Initialize() override;

		void Clear() override;

		CPM68KFormat(system_type system = SYSTEM_UNKNOWN, magic_type magic = MAGIC_CONTIGUOUS)
		{
			Initialize();
			SetSignature(magic);
			this->system = system;
		}

		void ReadFile(Linker::Reader& rd) override;

		template <typename SizeType>
			static void CDOS68K_WriteRelocations(Linker::Writer& wr, std::map<uint32_t, SizeType> relocations)
		{
			/* TODO: test */
			offset_t last_relocation = 0;
			for(auto it : relocations)
			{
				offset_t difference = it.first - last_relocation;
				uint8_t highbit = it.second/*.size*/ == 2 ? 0x80 : 0x00;
				if(difference != 0 && difference <= 0x7C)
				{
					wr.WriteWord(1, highbit | difference);
				}
				else if(difference < 0x100)
				{
					wr.WriteWord(1, highbit | 0x7D);
					wr.WriteWord(1, difference);
				}
				else if(difference < 0x10000)
				{
					wr.WriteWord(1, highbit | 0x7E);
					wr.WriteWord(2, difference);
				}
				else
				{
					wr.WriteWord(1, highbit | 0x7F);
					wr.WriteWord(4, difference);
				}
			}
		}

		void WriteFile(Linker::Writer& wr) override;

		void Dump(Dumper::Dumper& dump) override;

		void CalculateValues() override;

		/* * * Writer members * * */

		/**
		 * @brief Makes sure no relocations are placed into the output file
		 */
		bool option_no_relocation;

		/** @brief Segment to collect bss */
		Linker::Segment * bss_segment;
		/** @brief Segment to collect stack (Concurrent DOS 68K only) */
		Linker::Segment * stack_segment;

		/** @brief Return code segment (if it exists) */
		Linker::Segment * CodeSegment();

		/** @brief Return data segment (if it exists) */
		Linker::Segment * DataSegment();

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(Linker::Segment * segment) override;

		void CreateDefaultSegments();

		Script::List * GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

}

#endif /* CPM68K_H */
