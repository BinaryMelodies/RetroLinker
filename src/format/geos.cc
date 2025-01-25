
#include "geos.h"

/* TODO: unimplemented */

using namespace GEOS;

void GeodeFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t GeodeFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

std::string GeodeFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".geo";
}

