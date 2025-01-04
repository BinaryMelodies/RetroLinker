#ifndef HUNK_H
#define HUNK_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace Amiga
{
	/**
	 * @brief AmigaOS/TRIPOS Hunk files
	 *
	 * Introduced for the TRIPOS system and then adopted for AmigaOS, a hunk file stores the binary executable for an Amiga application.
	 */
	class HunkFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		enum flags
		{
			/**
			 * @brief Section to be stored in fast memory
			 */
			FastMemory = Linker::Section::CustomFlag,
			/**
			 * @brief Section to be stored in chip memory
			 */
			ChipMemory = Linker::Section::CustomFlag << 1,
		};

		unsigned FormatAdditionalSectionFlags(std::string section_name) const override;

		void ReadFile(Linker::Reader& rd) override;

		enum
		{
			HUNK_CODE = 0x3E9,
			HUNK_DATA = 0x3EA,
			HUNK_BSS = 0x3EB,
			HUNK_RELOC32 = 0x3EC,
			HUNK_END = 0x3F2,
			HUNK_HEADER = 0x3F3,
			HUNK_PPC_CODE = 0x4E9,
		};
		uint32_t cpu = HUNK_CODE;

		class Hunk
		{
		public:
			uint32_t hunk_type;
			enum flag_type
			{
				LoadAny      = 0x00000000,
				LoadPublic   = 0x00000001, /* default, not stored */
				LoadChipMem  = 0x00000002,
				LoadFastMem  = 0x00000004,
				LoadLocalMem = 0x00000008,
				Load24BitDma = 0x00000010,
				LoadClear    = 0x00010000,
			};
			flag_type flags;
			std::shared_ptr<Linker::Segment> image;

			Hunk(uint32_t hunk_type, std::string name = "image", unsigned flags = LoadAny)
				: hunk_type(hunk_type), flags((flag_type)flags), image(std::make_shared<Linker::Segment>(name))
			{
			}

			Hunk(uint32_t hunk_type, std::shared_ptr<Linker::Segment> segment, unsigned flags = LoadAny)
				: hunk_type(hunk_type), flags((flag_type)flags), image(segment)
			{
			}

			std::map<uint32_t, std::vector<uint32_t> > relocations;

			uint32_t GetSizeField();

			bool RequiresAdditionalFlags();

			uint32_t GetAdditionalFlags();
		};

		std::vector<Hunk> hunks;
		std::map<std::shared_ptr<Linker::Segment>, size_t> segment_index; /* makes it easier to lookup the indices of segments */

		using LinkerManager::SetLinkScript;

		void SetOptions(std::map<std::string, std::string>& options) override;

		void AddHunk(const Hunk& hunk);

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		void WriteFile(Linker::Writer& wr) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		std::string GetDefaultExtension(Linker::Module& module) override;

	};

}

#endif /* HUNK_H */
