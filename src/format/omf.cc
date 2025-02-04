
#include "omf.h"

/* TODO: unimplemented */

using namespace OMF;

void OMFFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t OMFFormat::WriteFile(Linker::Writer& wr) const
{
	/* TODO */

	return offset_t(-1);
}

void OMFFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("Intel OMF format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void OMFFormat::GenerateModule(Linker::Module& module) const
{
	/* TODO */
}

