#ifndef XPEXP_H
#define XPEXP_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

namespace Ergo
{
	/**
	 * @brief Ergo OS/286 and OS/386 "XP" .exp file (Ergo was formerly A.I. Architects, then Eclipse)
	 */
	class XPFormat : public virtual Linker::OutputFormat, public Linker::LinkerManager
	{
	public:
		std::string stub_file; // TODO

		void ReadFile(Linker::Reader& in) override;
		void WriteFile(Linker::Writer& out) override;
		/* TODO */

		std::string GetDefaultExtension(Linker::Module& module, std::string filename) override;
	};
}

#endif /* XPEXP_H */
