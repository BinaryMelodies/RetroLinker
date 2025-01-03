#ifndef MINIX_H
#define MINIX_H

#include "../common.h"
#include "../linker/linker.h"
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
	class MINIXFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
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
		} format;

		enum cpu_type
		{
			/* TODO: extend for 68K? */
			I86 = 0x04,
			M68K = 0x0B,
			NS16K = 0x0C,
			I386 = 0x10,
			SPARC = 0x17,
		} cpu;

		static ::EndianType GetEndianType(cpu_type cpu);

		::EndianType GetEndianType() const;

		static constexpr size_t PAGE_SIZE = 0x1000;

		MINIXFormat(format_type format, cpu_type cpu = cpu_type(0))
			: format(format), cpu(cpu), code_base_address(0), heap_top_address(0)
		{
		}

		uint32_t code_base_address; /* TODO: parametrize */
		//uint32_t data_base_address; /* TODO: parametrize */
		uint32_t heap_top_address; /* TODO: parametrize */

		/* generated */
		Linker::Segment * code, * data, * bss;
		uint32_t entry_address;

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void OnNewSegment(Linker::Segment * segment) override;

		void CreateDefaultSegments();

		Script::List * GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void WriteFile(Linker::Writer& wr) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module) override;
	};

}

#endif /* MINIX_H */
