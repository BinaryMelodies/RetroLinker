#ifndef MINIX_H
#define MINIX_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/linker_manager.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace MINIX
{
	/**
	 * @brief MINIX/ELKS a.out file format
	 *
	 * This is the native executable format for MINX and ELKS (Linux for 8086)
	 */
	class MINIXFormat : public virtual Linker::LinkerManager
	{
	public:
		/* TODO: incorporate relocations and far code segment from around ELKS 0.8.0 */

		void ReadFile(Linker::Reader& rd) override;

		bool FormatIs16bit() const override;

		enum format_type
		{
			FormatCombined = 0x10,
			FormatSeparate = 0x20,
			UnmappedZeroPage = 0x01,
		};
		format_type format = format_type(0);

		enum cpu_type
		{
			/* TODO: make support for 68K? */
			I86 = 0x04,
			M68K = 0x0B,
			NS16K = 0x0C,
			I386 = 0x10,
			SPARC = 0x17,
		};
		cpu_type cpu = cpu_type(0);

		uint8_t header_size = 0x20;
		uint16_t format_version = 0;

		static ::EndianType GetEndianType(cpu_type cpu);

		::EndianType GetEndianType() const;

		static constexpr size_t PAGE_SIZE = 0x1000;

		explicit MINIXFormat()
		{
		}

		MINIXFormat(format_type format, int version = -1)
			: format(format), format_version(version)
		{
		}

		MINIXFormat(format_type format, cpu_type cpu, int version = -1)
			: format(format), cpu(cpu), format_version(version)
		{
		}

		uint32_t bss_size = 0;
		uint32_t total_memory = 0; /* TODO: parametrize */
		uint16_t heap_size = 0, stack_size = 0;
		uint32_t code_relocation_base = 0;
		uint32_t data_relocation_base = 0;

		bool enable_relocations = false;
		bool enable_symbols = false;

		struct Relocation
		{
			static constexpr uint16_t S_ABS = uint16_t(-1);
			static constexpr uint16_t S_TEXT = uint16_t(-2);
			static constexpr uint16_t S_DATA = uint16_t(-3);
			static constexpr uint16_t S_BSS = uint16_t(-4);
			/* ELKS extension */
			static constexpr uint16_t S_FTEXT = uint16_t(-5);

			static constexpr uint16_t R_ABBS = 0;
			static constexpr uint16_t R_RELLBYTE = 2;
			static constexpr uint16_t R_PCRBYTE = 3;
			static constexpr uint16_t R_RELWORD = 4;
			static constexpr uint16_t R_PCRWORD = 5;
			static constexpr uint16_t R_RELLONG = 6;
			static constexpr uint16_t R_PCRLONG = 7;
			static constexpr uint16_t R_REL3BYTE = 8;
			static constexpr uint16_t R_KBRANCHE = 9;
			/* ELKS extension */
			static constexpr uint16_t R_SEGWORD = 80;

			uint32_t address = 0;
			uint16_t symbol = 0;
			uint16_t type = 0;

			static Relocation Read(Linker::Reader& rd);
			void Write(Linker::Writer& wr);
		};
		std::vector<Relocation> code_relocations, data_relocations, far_code_relocations;

		struct Symbol
		{
			static constexpr uint8_t N_SECT = 0x07; // mask

			static constexpr uint8_t N_UNDF = 0x00;
			static constexpr uint8_t N_ABS = 0x01;
			static constexpr uint8_t N_TEXT = 0x02;
			static constexpr uint8_t N_DATA = 0x03;
			static constexpr uint8_t N_BSS = 0x04;
			static constexpr uint8_t N_COMM = 0x05;

			static constexpr uint8_t N_CLASS = 0xF8; // mask

			static constexpr uint8_t S_NULL = 0x00;
			static constexpr uint8_t S_EXT = 0x10; // external
			static constexpr uint8_t S_STAT = 0x18; // static

			std::string name;
			int32_t value;
			uint8_t sclass;
			uint8_t numaux; // not used by MINIX/ELKS
			uint16_t type; // not used by MINIX/ELKS

			static Symbol Read(Linker::Reader& rd);
			void Write(Linker::Writer& wr);
		};
		std::vector<Symbol> symbols;

		/* generated */
		std::shared_ptr<Linker::Image> code, data, far_code;
		std::shared_ptr<Linker::Segment> bss, heap, stack;
		uint32_t entry_address = 0;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options) override;
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module) override;
	};

}

#endif /* MINIX_H */
