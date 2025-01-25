
#include "peexe.h"

/* TODO: unimplemented */

using namespace Microsoft;

void PEFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t PEFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

