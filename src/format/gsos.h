#ifndef GSOS_H
#define GSOS_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

namespace Apple
{
	/**
	 * @brief Apple GS/OS OMF file format
	 *
	 * The file format was originally invented for ORCA/M as an object format, later adopted for the Apple GS/OS operating system.
	 * It had multiple versions, including a few early versions, version 1, 2 and 2.1.
	 */
	class OMFFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* GSOS_H */
