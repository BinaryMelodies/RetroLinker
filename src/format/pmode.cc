
#include "pmode.h"

/* TODO: unimplemented */

using namespace PMODE;

void PMW1Format::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t PMW1Format::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

std::string PMW1Format::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

