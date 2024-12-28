#ifndef O65_H
#define O65_H

#include "../common.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* o65 object format (input only) */
namespace O65
{
	/**
	 * @brief Output format for the 6502 assembler xa
	 */
	class O65Format : public virtual Linker::InputFormat
	{
	public:
		void ReadFile(Linker::Reader& in) override;
		void WriteFile(Linker::Writer& out) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		/* TODO */
	};
}

#endif /* O65_H */
