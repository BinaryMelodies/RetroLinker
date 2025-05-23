#ifndef OMF_H
#define OMF_H

#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/format.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* Intel Object Module format (input only) */

namespace OMF
{
	/**
	 * @brief Intel Relocatable Object Module format, used by various 16/32-bit DOS based compilers and linkers, including 16-bit Microsoft compilers, Borland and Watcom compilers
	 */
	class OMFFormat : public virtual Linker::InputFormat
	{
	public:
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
		/* TODO */
	};
}

#endif /* OMF_H */
