
#include "peexe.h"

/* TODO: unimplemented */

using namespace Microsoft;

void PEFormat::ReadFile(Linker::Reader& rd)
{
	/* TODO */
}

offset_t PEFormat::WriteFile(Linker::Writer& wr)
{
	/* TODO */

	return offset_t(-1);
}

void PEFormat::Dump(Dumper::Dumper& dump)
{
	// TODO: Windows encoding
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("PE format");
	Dumper::Region file_region("File", file_offset, 0 /* TODO: file size */, 8);
	file_region.Display(dump);

	// TODO
}

std::string PEFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ".exe";
}

