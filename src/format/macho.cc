
#include "macho.h"

using namespace MachO;

void MachOFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t MachOFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void MachOFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Mach-O format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

