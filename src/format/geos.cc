
#include "geos.h"

/* TODO: unimplemented */

using namespace GEOS;

void GeodeFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

void GeodeFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */
}

std::string GeodeFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".geo";
}

