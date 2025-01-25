#ifndef OMF_H
#define OMF_H

#include "../common.h"
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
		void WriteFile(Linker::Writer& wr) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		/* TODO */
	};
}

#endif /* OMF_H */
