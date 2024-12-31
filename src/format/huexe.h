#ifndef HUEXE_H
#define HUEXE_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace X68000
{
	/**
	 * @brief Human68k "HU" .X file
	 */
	class HUFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;

		HUFormat()
			: load_mode(MODE_NORMAL), entry_address(0), option_no_relocation(false)
		{
		}

		enum
		{
			MODE_NORMAL,
			MODE_SMALLEST,
			MODE_HIGHEST,
		} load_mode; /* TODO: make parameter */
		uint32_t entry_address;
		bool option_no_relocation; /* TODO: make parameter */

		/* filled in automatically */
		Linker::Segment * code, * data, * bss;
		uint32_t relocation_size;
		std::map<uint32_t, unsigned char> relocations;

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

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};

}

#endif /* HUEXE_H */
