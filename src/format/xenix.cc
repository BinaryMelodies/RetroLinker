
#include "xenix.h"

/* TODO: unimplemented */

using namespace Xenix;

void BOutFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t BOutFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void BOutFormat::Dump(Dumper::Dumper& dump)
{
	// TODO: set encoding to other for non-x86?
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("b.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

void XOutFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t XOutFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void XOutFormat::Dump(Dumper::Dumper& dump)
{
	// TODO: set encoding to other for non-x86?
	dump.SetEncoding(Dumper::Block::encoding_cp437);

	dump.SetTitle("x.out format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

