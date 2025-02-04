
#include "geos.h"
#include "../linker/location.h"

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

void GeodeFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Geode format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

std::string GeodeFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".geo";
}

