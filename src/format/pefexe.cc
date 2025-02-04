
#include "pefexe.h"
#include "../linker/location.h"

/* TODO: unimplemented */

using namespace Apple;

void PEFFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t PEFFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void PEFFormat::Dump(Dumper::Dumper& dump)
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PEF format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

