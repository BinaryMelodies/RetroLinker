#ifndef AS86OBJ_H
#define AS86OBJ_H

#include "../common.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: unimplemented */

/* as86 object format (input only) */
namespace AS86Obj
{
	/**
	 * @brief Output format for as86, used as an output by Bruce's C compiler from the dev86 package
	 */
	class AS86ObjFormat : public virtual Linker::InputFormat
	{
	public:
		void ReadFile(Linker::Reader& in) override;
		void WriteFile(Linker::Writer& out) override;
		void ProduceModule(Linker::Module& module, Linker::Reader& rd) override;
		/* TODO */
	};
}

#endif /* AS86OBJ_H */
