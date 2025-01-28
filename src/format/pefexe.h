#ifndef PEFEXE_H
#define PEFEXE_H

#include "../common.h"
#include "../linker/linker_manager.h"

/* TODO: unimplemented */

namespace Apple
{
	/**
	 * @brief PowerPC Classic Mac OS "PEF" file format
	 */
	class PEFFormat : public virtual Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* PEFEXE_H */
