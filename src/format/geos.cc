
#include "geos.h"
#include "../linker/location.h"

/* TODO: unimplemented */

using namespace GEOS;

bool GeodeFormat::FormatSupportsSegmentation() const
{
	/* TODO */
	return true;
}

bool GeodeFormat::FormatIs16bit() const
{
	return true;
}

bool GeodeFormat::FormatIsProtectedMode() const
{
	return false;
}

void GeodeFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t GeodeFormat::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void GeodeFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("Geode format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

std::string GeodeFormat::GetDefaultExtension(Linker::Module& module, std::string filename) const
{
	return filename + ".geo";
}

