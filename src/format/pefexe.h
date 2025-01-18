#ifndef PEFEXE_H
#define PEFEXE_H

#include "../common.h"
#include "../linker/linker.h"

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
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* PEFEXE_H */
