#ifndef PEEXE_H
#define PEEXE_H

#include "../common.h"
#include "../linker/linker_manager.h"
#include "coff.h"
#include "mzexe.h"

/* TODO: unimplemented */

namespace Microsoft
{
	/**
	 * @brief Microsoft PE .EXE portable executable file format
	 */
	class PEFormat : public COFF::COFFFormat, protected Microsoft::MZStubWriter
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* PEEXE_H */
