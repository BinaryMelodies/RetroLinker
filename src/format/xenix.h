#ifndef XENIX_H
#define XENIX_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* b.out and x.out file formats */
namespace Xenix
{
	/**
	 * @brief Xenix b.out executable
	 */
	class BOutFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};

	/**
	 * @brief Xenix x.out executable
	 */
	class XOutFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* XENIX_H */
