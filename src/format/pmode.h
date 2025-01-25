#ifndef PMODE_H
#define PMODE_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace PMODE
{
	/**
	 * @brief PMODE/W linear executable format (https://github.com/amindlost/pmodew/blob/main/docs/pmw1fmt.txt)
	 */
	class PMW1Format : public virtual Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		offset_t WriteFile(Linker::Writer& wr) override;
		/* TODO */

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* PMODE_H */
