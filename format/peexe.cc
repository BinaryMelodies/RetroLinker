
#include "peexe.h"

/* TODO: unimplemented */

using namespace Microsoft;

void PEFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void PEFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

