
#include "pmode.h"

/* TODO: unimplemented */

using namespace PMODE;

void PMW1Format::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void PMW1Format::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

std::string PMW1Format::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

