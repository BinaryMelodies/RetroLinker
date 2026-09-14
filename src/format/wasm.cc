
#include "wasm.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

using namespace Wasm;

void WebAssemblyFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = ::LittleEndian;
	rd.Seek(0);
	// TODO
}

offset_t WebAssemblyFormat::WriteFile(Linker::Writer& wr) const
{
	wr.endiantype = ::LittleEndian;
	wr.Seek(0);
	// TODO
	return ImageSize();
}

offset_t WebAssemblyFormat::ImageSize() const
{
	// TODO
	return offset_t(-1);
}

void WebAssemblyFormat::Dump(Dumper::Dumper& dump) const
{
	dump.SetEncoding(Dumper::Block::encoding_default);

	dump.SetTitle("WebAssembly module format");
	Dumper::Region file_region("File", file_offset, ImageSize(), 8);
	file_region.Display(dump);

	// TODO
}

