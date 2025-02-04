#ifndef XENIX_H
#define XENIX_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/reader.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* b.out and x.out file formats */
namespace Xenix
{
	/**
	 * @brief Xenix b.out executable
	 */
	class BOutFormat : public virtual Linker::SegmentManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */
	};

	/**
	 * @brief Xenix x.out executable
	 */
	class XOutFormat : public virtual Linker::SegmentManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) override;
		void Dump(Dumper::Dumper& dump) override;
		/* TODO */
	};
}

#endif /* XENIX_H */
