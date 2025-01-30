#ifndef HUEXE_H
#define HUEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/linker_manager.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace X68000
{
	/**
	 * @brief Human68k "HU" .X file
	 */
	class HUFormat : public virtual Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		enum load_mode_type
		{
			MODE_NORMAL,
			MODE_SMALLEST,
			MODE_HIGHEST,
		};
		load_mode_type load_mode = MODE_NORMAL; /* TODO: make parameter */
		uint32_t entry_address = 0;
		bool option_no_relocation = false; /* TODO: make parameter */

		/* filled in automatically */
		std::shared_ptr<Linker::Segment> code, data, bss;
		uint32_t relocation_size = 0;
		std::map<uint32_t, unsigned char> relocations;

		void OnNewSegment(std::shared_ptr<Linker::Segment> segment) override;

		void CreateDefaultSegments();

		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

		void Link(Linker::Module& module);

		void ProcessModule(Linker::Module& module) override;

		void CalculateValues() override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;

		void GenerateFile(std::string filename, Linker::Module& module) override;

		using Linker::OutputFormat::GetDefaultExtension;
		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

}

#endif /* HUEXE_H */
