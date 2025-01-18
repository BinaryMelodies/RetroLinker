#ifndef MACHO_H
#define MACHO_H

#include <sstream>
#include <vector>
#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/reader.h"

namespace MachO
{
	/**
	 * @brief Mach/NeXTSTEP/Mac OS X (macOS) file format
	 *
	 * Originally developed for the Mach kernel, it has been adopted by other UNIX systems based on the Mach kernel, including NeXTSTEP and macOS.
	 */
	class MachOFormat : public virtual Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* MACHO_H */
