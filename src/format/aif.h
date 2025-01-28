#ifndef AIF_H
#define AIF_H

#include "../common.h"
#include "../linker/linker_manager.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace ARM
{
	/**
	 * @brief ARM Image Format
	 */
	class AIFFormat : public virtual Linker::LinkerManager
	{
	public:
		static constexpr uint32_t ARM_NOP = 0xE1A00000;
		static constexpr uint32_t ARM_BL = 0xEB000000;
		static constexpr uint32_t ARM_BL_OP = ARM_BL >> 24;

		::EndianType endiantype = ::LittleEndian;

		bool relocatable = false;
		uint32_t relocation_code = 0; // relative to image start

		bool has_zero_init = false;
		uint32_t zero_init_code = 0; // relative to image start

		bool executable = true;
		uint32_t entry = 0; // relative to code start

		static constexpr uint32_t ARM_SWI_0x11 = 0xEF000011;
		uint32_t exit_instruction = ARM_SWI_0x11;

		uint32_t text_size = 0;
		uint32_t data_size = 0;
		uint32_t debug_size = 0;
		uint32_t bss_size = 0;
		enum debug_type : uint32_t
		{
			NoDebugData = 0,
			LowLevelDebugData = 1,
			SourceLevelDebugData = 2,
			BothDebugData = 3,
		};
		debug_type image_debut_type = NoDebugData;
		uint32_t image_base = 0;
		uint32_t workspace = 0;
		enum address_mode_type : uint32_t
		{
			OldAifHeader = 0,
			AifHeader26 = 26,
			AifHeader32 = 32,
			SeparateCodeData = 0x00000100,
		};
		address_mode_type address_mode = AifHeader32;
		uint32_t data_base = 0;

		void Clear() override;
		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* AIF_H */
