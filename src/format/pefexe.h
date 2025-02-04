#ifndef PEFEXE_H
#define PEFEXE_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/segment_manager.h"

/* TODO: unimplemented */

namespace Apple
{
	/**
	 * @brief PowerPC Classic Mac OS "PEF" file format
	 */
	class PEFFormat : public virtual Linker::SegmentManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */
	};
}

#endif /* PEFEXE_H */
